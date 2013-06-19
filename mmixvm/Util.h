#pragma once

#include <stdint.h>
#include <windows.h>
#include <llvm/ADT/Twine.h>
#include "MmixDef.h"

namespace MmixLlvm {
	namespace Util {
		llvm::Twine genUniq(const llvm::Twine& prefix);

		MXWyde adjust16Endianness(llvm::ArrayRef<MmixLlvm::MXByte>& ref);

		MXTetra adjust32Endianness(llvm::ArrayRef<MmixLlvm::MXByte>& ref);

		MXOcta adjust64Endianness(llvm::ArrayRef<MmixLlvm::MXByte>& ref);

		std::vector<MmixLlvm::MXByte> stringToBytes(const std::wstring& arg, DWORD cp);

		std::wstring fromUTF8(const std::vector<MmixLlvm::MXByte>& arg);
	}
}