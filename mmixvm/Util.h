#pragma once

#include <stdint.h>
#include <windows.h>
#include <llvm/ADT/Twine.h>

namespace MmixLlvm {
	namespace Util {
		llvm::Twine genUniq(const llvm::Twine& prefix);

		uint16_t adjust16Endianness(llvm::ArrayRef<uint8_t>& ref);

		uint32_t adjust32Endianness(llvm::ArrayRef<uint8_t>& ref);

		uint64_t adjust64Endianness(llvm::ArrayRef<uint8_t>& ref);

		std::vector<uint8_t> stringToBytes(const std::wstring& arg, DWORD cp);

		std::wstring fromUTF8(const std::vector<uint8_t>& arg);
	}
}