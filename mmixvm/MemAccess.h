#pragma once

#include <stdint.h>

namespace MmixLlvm {
	struct MemAccessor {
		virtual uint8_t readByte(uint64_t ref) = 0;

		virtual uint16_t readWyde(uint64_t ref) = 0;

		virtual uint32_t readTetra(uint64_t ref) = 0;

		virtual uint64_t readOcta(uint64_t ref) = 0;

		virtual ~MemAccessor() = 0;
	};
};