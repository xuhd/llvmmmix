#include "stdafx.h"
#include <windows.h>
#include "Util.h"
#include "Engine.h"
#include "MemAccess.h"
#include "RegAccess.h"
#include "OS.h"

using llvm::Twine;

using namespace MmixLlvm::Util;

namespace {
	int Id = 0;
}

Twine MmixLlvm::Util::genUniq(const llvm::Twine& prefix) {
	return llvm::Twine(prefix) + (llvm::Twine(++Id));
}

uint16_t MmixLlvm::Util::adjust16Endianness(llvm::ArrayRef<uint8_t>& ref) {
	return ((uint16_t)ref[0] << 8) | (uint16_t)ref[1];
}

uint32_t MmixLlvm::Util::adjust32Endianness(llvm::ArrayRef<uint8_t>& ref) {
	return ((uint32_t)ref[0] << 24) | ((uint32_t)ref[1] << 16) 
		 | ((uint32_t)ref[2] << 8)  | (uint32_t)ref[3];
}

uint64_t MmixLlvm::Util::adjust64Endianness(llvm::ArrayRef<uint8_t>& ref) {
	return ((uint64_t)ref[0] << 56) | ((uint64_t)ref[1] << 48)
		 | ((uint64_t)ref[2] << 40) | ((uint64_t)ref[3] << 32)
	     | ((uint64_t)ref[4] << 24) | ((uint64_t)ref[5] << 16)
	     | ((uint64_t)ref[6] << 8)  |  (uint64_t)ref[7];
}

std::vector<uint8_t> MmixLlvm::Util::stringToBytes(const std::wstring& arg, DWORD cp) {
	int size = ::WideCharToMultiByte(cp, 0, arg.c_str(), -1, NULL, 0, NULL, NULL);
	std::vector<uint8_t> retVal(size);
	::WideCharToMultiByte(cp, 0, arg.c_str(), -1, (LPSTR)&retVal[0], retVal.size(), NULL, NULL);
	return retVal;
}

std::wstring MmixLlvm::Util::fromUTF8(const std::vector<uint8_t>& arg) {
	int cc = ::MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)&arg[0], arg.size(), NULL, 0);
	std::vector<wchar_t> tmp(cc);
	::MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)&arg[0], arg.size(), &tmp[0], cc);
	return std::wstring(tmp.begin(), tmp.end());
}

MmixLlvm::MemAccessor::~MemAccessor() { }

MmixLlvm::RegAccessor::~RegAccessor() { }

MmixLlvm::Engine::~Engine() { }

MmixLlvm::OS::~OS() { }