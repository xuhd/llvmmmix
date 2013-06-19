#pragma once

#include "MmixDef.h"
#include <stdint.h>

namespace MmixLlvm {
	struct RegAccessor {
		virtual MXOcta getReg(MXByte reg) = 0;

		virtual void setReg(MXByte reg, MXOcta value) = 0;

		virtual MXOcta getSpReg(MmixLlvm::SpecialReg sreg) = 0;

		virtual void setSpReg(MmixLlvm::SpecialReg reg, MXOcta value) = 0;

		virtual ~RegAccessor() = 0;
	};
};