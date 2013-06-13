#pragma once

#include <stdint.h>

namespace MmixLlvm {
	struct MemAccessor {
		virtual uint8_t readByte(uint64_t ref) = 0;

		virtual uint16_t readWyde(uint64_t ref) = 0;

		virtual uint32_t readTetra(uint64_t ref) = 0;

		virtual uint64_t readOcta(uint64_t ref) = 0;

		virtual void writeByte(uint64_t ref, uint8_t arg) = 0;

		virtual void writeWyde(uint64_t ref, uint16_t arg) = 0;

		virtual void writeTetra(uint64_t ref, uint32_t arg) = 0;

		virtual void writeOcta(uint64_t ref, uint64_t arg) = 0;

		virtual ~MemAccessor() = 0;
	};
};