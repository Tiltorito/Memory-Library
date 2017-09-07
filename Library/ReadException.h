#pragma once

#include "ReadWriteException.h"

class ReadException : public ReadWriteException {
public:
	ReadException(const std::string& msg, DWORD pid, LPVOID address, DWORD code) : ReadWriteException(msg, pid, address, code) {}
};