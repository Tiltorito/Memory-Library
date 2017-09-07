#pragma once

#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <cstdarg>
#include "AttachException.h"
#include "WriteException.h"
#include "ReadException.h"
#include "stdafx.h"

namespace Memory {
#define null NULL

	typedef struct {
		DWORD baseAddress;	// The base address of the module
		DWORD size;			// The size of the module in bytes
	} MODULE_INFO, *PMODULE_INFO;



	/*
	*	Trying to take a handler in the specified window and then 
	*	return the process id which made the window, the process must 
	*	have a window.
	*	
	*	@param  the name of the window.
	*	@return the pid of the process which made the window or null if not found
	*/
	DWORD getPID(LPCTSTR windowName) {
		HWND window = FindWindow(null, windowName);											// Try to take a handler to the window
		DWORD PID = null;
		if (window == null) {				
			_tprintf(TEXT("Error fetching the PID errorCode: %d\n"), GetLastError());
		}
		else {
			GetWindowThreadProcessId(window, &PID);											// Fetch the PID
		}

		return PID;
	}



	/*
	*	Doing linear search on all processes, trying to find the 
	*	specified process name. (Does not need a window Name).
	*
	*	@param  the name of the process to be searched.
	*	@return the PID of the process if found or 0 otherwise.
	*/
	DWORD getPIDex(LPCTSTR processName) {
		PROCESSENTRY32 entry;

		entry.dwSize = sizeof(PROCESSENTRY32);
		std::wstring bin;
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, null);

		if (Process32First(snapshot, &entry)) {
			bin = entry.szExeFile;

			if (bin.find(processName) != std::wstring::npos) {
				CloseHandle(snapshot);
				return entry.th32ProcessID;
			}

			while (Process32Next(snapshot, &entry)) {
				bin = entry.szExeFile;

				if (bin.find(processName) != std::wstring::npos) {
					CloseHandle(snapshot);
					return entry.th32ProcessID;
				}
			}
		}

		_tprintf(TEXT("Error fetching the PID errorCode: %d\n"), GetLastError());
		CloseHandle(snapshot);
		return 0;
	}


	/*
	*	Returns the baseAddress of a module.
	*	The module must be mapped inside the process specified by the @param PID,
	*	if not the function return NULL.
	*	
	*	@param  the name of the module.
	*	@param  the PID of the process.
	*	@return the baseAddress of the module casted on T type (relative to process VM).
	*/
	template<typename T>
	T getModuleBaseAddress(LPCTSTR moduleName, DWORD PID) {
		PMODULEENTRY32 entry = getModule(moduleName, PID);
		if (entry) {
			T baseAdd = (T)entry->modBaseAddr;
			free(entry);
			return baseAdd;
		}

		return null;
	}



	namespace {

		/*
		*	Doing linear search on all modules mapped inside the process specified by @param PID,
		*	until it finds a module with the same name as @param moduleName.
		*	It returns a pointer to a module entry,
		*	the a pointer to a module entry is allocated via malloc.
		*	The module must be mapped inside the process specified by the @param PID,
		*	if not the the pointer is NULL. The pointer should be freed.
		*
		*	@param  the name of the module.
		*	@param  the PID of the process
		*	@return A pointer to a module entry. It points to MODULEEENTRY32 struct allocated via malloc,
		*			remember to free the pointer.
		*
		*/
		PMODULEENTRY32 getModule(LPCTSTR moduleName, DWORD PID) {
			HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, PID);

			PMODULEENTRY32 entry = (PMODULEENTRY32)malloc(sizeof(MODULEENTRY32));
			entry->dwSize = sizeof(MODULEENTRY32);
			std::wstring bin;

			if (Module32First(snapshot, entry)) {
				bin = entry->szModule;
				if (bin == moduleName) {
					CloseHandle(snapshot);
					return entry;

				}

				while (Module32Next(snapshot, entry)) {
					bin = entry->szModule;
					if (bin == moduleName) {
						CloseHandle(snapshot);
						return entry;
					}
				}
			}
			
			_tprintf(TEXT("Error fetching module address errorCode: %d\n"), GetLastError());
			CloseHandle(snapshot);
			return null;
		}
	}

	/*
	*	Finds and return the moduleInformation about the specified module. The module
	*	must be mapped inside the process (specified by the @param pid).
	*
	*	@param  the name of the module.
	*	@param  the pid of the process which module is mapped inside.
	*	@param  a pointer to a MODULE_INFO structure that will used as buffer for the result.
	*	@return TRUE if the function succeed and FALSE otherwise.
	*/
	BOOL getModuleInformation(LPCTSTR moduleName, DWORD pid, PMODULE_INFO moduleInfo) {
		PMODULEENTRY32 entry = getModule(moduleName, pid);

		if (entry) {
			moduleInfo->baseAddress = (DWORD)entry->modBaseAddr;
			moduleInfo->size = entry->modBaseSize;
			free(entry);
			return TRUE;
		}

		return FALSE;
	}


	/*
	*	Open an existing process with ALL_ACCESS right. The HANDLE is not inheritable
	*
	*	@param  the pid of the process.
	*	@return a open HANDLE to the process. Remember is not inheritable.
	*	@throws AttachException if HANDLE value is invalid.
	*/
	HANDLE openProcess(DWORD PID) {
		HANDLE process = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, false, PID);

		if (process == null) {
			throw AttachException("Failed to open the process", PID, GetLastError());
		}

		return process;
	}



	/*
	*	Writes data to an area of memory in a specified process. 
	*	The entire area to be written to must be accessible or the operation fails.
	*
	*	@param<T> the data type of the value to be written.
	*	@param    a HANDLE to the process with at least PROCESS_VM_WRITE right.
	*	@param    pointer to the base address in the specified process to which data is written.
	*	@param    the value to be written.
	*	@return   TRUE if write operation succeed.
	*	@throws   WriteException if write operation failed.
	*/
	template<typename T>
	BOOL writeMemory(HANDLE process, LPVOID address, T value) {
		if (!WriteProcessMemory(process, address, &value, sizeof(T), null)) {
			throw WriteException("Failed to write on process memory", GetProcessId(process), address, GetLastError());
		}

		return TRUE;
	}


	/*
	*	{@link writeMemory<T>(HANDLE, LPVOID, T valye)}
	*/
	template<typename T>
	BOOL writeMemory(HANDLE process, DWORD address, T value) {
		return writeMemory<T>(process, (LPVOID)address, value);
	}



	/*
	*	Reads data from an area of memory in a specified process.
	*	The entire area to be read must be accessible or the operation fails.
	*
	*	@param<T> the type of the value to be read.
	*	@param    a HANDLE to the process with at least PROCESS_VM_READ.
	*	@param    a pointer to the base address of the area to be read.
	*	@return   the data which the function read if the operation succeed.
	*	@throws   ReadException if the operation failed
	*/
	template<typename T>
	T readMemory(HANDLE process, LPVOID address) {
		T value;
		if (!ReadProcessMemory(process, address, &value, sizeof(T), null)) {
			throw ReadException("Failed to read the process memory", GetProcessId(process), address, GetLastError());
		}

		return value;
	}


	/*
	* {@link readMemory<T>(HANDLE, LPVOID)}
	*/
	template<typename T>
	T readMemory(HANDLE process, DWORD address) {
		return readMemory<T>(process, (LPVOID)address);
	}


	/*
	*	Changes the protection on the specified area in VM of the process.
	*
	*   @param<T> the sizeof(T) specifize the length of the area which the protection will change.
	*	@param  a HANDLE to the process.
	*	@param  a pointer to the base address in the address space of the process to change the protection.
	*	@param  the new protection.
	*	@return the old protection in this area.
	*/
	template<typename T>
	DWORD protectMemory(HANDLE process, LPVOID address, DWORD protection) {
		DWORD oldProtect;
		VirtualProtectEx(process, address, sizeof(T), protection, &oldProtect);
		return oldProtect;
	}


	/*
	*	Doing a linear search inside the specified process until it finds the specified pattern.
	*	The process must have at least PROCESS_VM_WRITE at least. Keep in mind that the function is slow.
	*
	*	@param<T> T specifize the return type of the function.
	*	@param    the process.
	*	@param    the startAddress for the search.
	*	@param    the length of the search. The function will search from startAddress to startAddress + length - 1.
	*	@param    the mask for the pattern.
	*	@param    the number of parameters that ... contains.
	*	@param    (...) The bytes of the pattern.
		@return   the baseAddress of the first pattern found.
	*/
	template<typename T>
	T patternScan(HANDLE process, DWORD startAddress, DWORD length, LPCSTR mask, int count, ...) {
		va_list ap;
		va_start(ap, count);
		std::vector<BYTE> signature;
		
		for (int i = 0; i < count; i++) {
			BYTE byte = va_arg(ap, BYTE);
			signature.push_back(byte);
		}

		return patternScanEx<T>(process, startAddress, length, mask, signature);
	}



	template<typename T>
	T patternScanEx(HANDLE process, DWORD startAddress, DWORD length, LPCSTR mask, std::vector<BYTE>& signature) {

		if (strlen(mask) != signature.size() || length <= 0) {
			return (T)null;
		}

		for (DWORD i = 0; i < length; i++) {
			if (patternMatches(process, startAddress + i, mask, signature)) {
				return (T)(startAddress + i);
			}
		}

		return (T)null;

	}

	namespace {

		/*
		*	Says when a area in the specified process matches the signature.
		*
		*	@param  a HANDLE to the process.
		*	@param  the baseAddress that the function will try to match.
		*	@param  the mask of the pattern.
		*	@param  a vector which contains the signature of the pattern.
		*	@return TRUE if the signature of the pattern matches the BYTES in the area in the memory specified by the @param address.
		*/
		BOOL patternMatches(HANDLE process, DWORD address, LPCSTR mask, std::vector<BYTE>& signature) {

			for (DWORD i = 0; i < signature.size(); i++) {
				if (mask[i] == 'x' && readMemory<BYTE>(process, address + i) != signature[i]) {
					return FALSE;
				}
			}

			return TRUE;
		}

	}

}