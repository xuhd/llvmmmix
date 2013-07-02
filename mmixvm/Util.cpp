#include "stdafx.h"
#include <windows.h>
#include "Util.h"
#include "Engine.h"
#include "MemAccess.h"
#include "RegAccess.h"
#include "OS.h"
#include "VerticeContext.h"
#include "MmixDef.h"

using llvm::Twine;

using namespace MmixLlvm::Util;

using MmixLlvm::MXByte;
using MmixLlvm::MXWyde;
using MmixLlvm::MXTetra;
using MmixLlvm::MXOcta;

namespace {
	int Id = 0;
}

Twine MmixLlvm::Util::genUniq(const llvm::Twine& prefix) {
	return llvm::Twine(prefix) + (llvm::Twine(++Id));
}

MXWyde MmixLlvm::Util::adjust16Endianness(llvm::ArrayRef<MXByte>& ref) {
	return ((MXWyde)ref[0] << 8) | (MXWyde)ref[1];
}

MXTetra MmixLlvm::Util::adjust32Endianness(llvm::ArrayRef<MXByte>& ref) {
	return ((MXTetra)ref[0] << 24) | ((MXTetra)ref[1] << 16) 
		 | ((MXTetra)ref[2] << 8)  | (MXTetra)ref[3];
}

MXOcta MmixLlvm::Util::adjust64Endianness(llvm::ArrayRef<MXByte>& ref) {
	return ((MXOcta)ref[0] << 56) | ((MXOcta)ref[1] << 48)
		 | ((MXOcta)ref[2] << 40) | ((MXOcta)ref[3] << 32)
	     | ((MXOcta)ref[4] << 24) | ((MXOcta)ref[5] << 16)
	     | ((MXOcta)ref[6] << 8)  |  (MXOcta)ref[7];
}

std::vector<MXByte> MmixLlvm::Util::stringToBytes(const std::wstring& arg, DWORD cp) {
	int size = ::WideCharToMultiByte(cp, 0, arg.c_str(), -1, NULL, 0, NULL, NULL);
	std::vector<MXByte> retVal(size);
	::WideCharToMultiByte(cp, 0, arg.c_str(), -1, (LPSTR)&retVal[0], retVal.size(), NULL, NULL);
	return retVal;
}

std::wstring MmixLlvm::Util::fromUTF8(const std::vector<MXByte>& arg) {
	int cc = ::MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)&arg[0], arg.size(), NULL, 0);
	std::vector<wchar_t> tmp(cc);
	::MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)&arg[0], arg.size(), &tmp[0], cc);
	return std::wstring(tmp.begin(), tmp.end());
}

MmixLlvm::MemAccessor::~MemAccessor() { }

MmixLlvm::RegAccessor::~RegAccessor() { }

MmixLlvm::Engine::~Engine() { }

MmixLlvm::OS::~OS() { }

MmixLlvm::Private::VerticeContext::~VerticeContext() { }