#pragma once

#include <iostream>
#include <exception>
#include <Windows.h>

class AttachException : public std::runtime_error {
public:
	AttachException(const std::string& msg) : AttachException(msg, 0, 0) {}
	AttachException(const std::string& msg, DWORD pid, DWORD code) : std::runtime_error(msg), msg(msg), code(code) {}
	
	virtual const char* what() {
		return msg.c_str();
	}

	virtual DWORD errorCode() const {
		return code;
	}

	virtual DWORD getPID() {
		return pid;
	}

private:
	std::string msg;
	DWORD pid;
	DWORD code;


};