#pragma once

#include "ReadWriteException.h"

class WriteException : public ReadWriteException {
public:
	WriteException(const std::string& msg, DWORD pid, LPVOID address, DWORD code) : ReadWriteException(msg, pid, address, code) {}
};