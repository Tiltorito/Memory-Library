#pragma once

#include "Memory.h"

using namespace Memory;

class MemoryWrapper {
public:
	MemoryWrapper() : MemoryWrapper(null) {}
	MemoryWrapper(HANDLE process) : process(process) {} 
	~MemoryWrapper() {
		CloseHandle(process);
	}

	template<typename T>
	BOOL write(LPVOID address, T value) {
		return writeMemory<T>(process, address, value);
	}

	
	template<typename T>
	BOOL write(DWORD address, T value) {
		return writeMemory<T>(process, address, value);
	}

	template<typename T>
	T read(LPVOID address) {
		return readMemory<T>(process, address);
	}

	template<typename T>
	T read(DWORD address) {
		return readMemory<T>(process, address);
	}


	template<typename T>
	T patternScan(DWORD startAddress, DWORD length, LPCSTR mask, int count, ...) {
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
	DWORD protect(LPVOID address, DWORD protection) {
		return protectMemory(process, address, protection);
	}

	template<typename T>
	DWORD protect(DWORD address, DWORD protection) {
		return protect<T>((LPVOID)address, protection);
	}


	void setProcess(HANDLE process) {
		this->process = process;
	}

private:
	HANDLE process;
};

