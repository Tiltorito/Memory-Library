#pragma once

#include <iostream>
#include <exception>
#include <Windows.h>


class ReadWriteException : public std::runtime_error {
public:
	ReadWriteException(const std::string& msg, DWORD pid, LPVOID address, DWORD code) : std::runtime_error(msg), pid(pid), address(address), code(code) {}
	
	virtual DWORD getPID() const {
		return pid;
	}

	virtual LPVOID getAddress() const {
		return address;
	}

	virtual DWORD errorCode() const {
		return code;
	}
	
private:
	DWORD pid;
	LPVOID address;
	DWORD code;
};