#pragma once

#include <stdint.h>

namespace MmixLlvm {
	struct MemAccessor {
		virtual MXByte readByte(MXOcta ref) = 0;

		virtual MXWyde readWyde(MXOcta ref) = 0;

		virtual MXTetra readTetra(MXOcta ref) = 0;

		virtual MXOcta readOcta(MXOcta ref) = 0;

		virtual void writeByte(MXOcta ref, MXByte arg) = 0;

		virtual void writeWyde(MXOcta ref, MXWyde arg) = 0;

		virtual void writeTetra(MXOcta ref, MXTetra arg) = 0;

		virtual void writeOcta(MXOcta ref, MXOcta arg) = 0;

		virtual ~MemAccessor() = 0;
	};
};