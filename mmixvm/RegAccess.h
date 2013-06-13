#pragma once

#include "MmixDef.h"
#include <stdint.h>

namespace MmixLlvm {
	struct RegAccessor {
		virtual uint64_t getReg(uint8_t reg) = 0;

		virtual void setReg(uint8_t reg, uint64_t value) = 0;

		virtual uint64_t getSpReg(MmixLlvm::SpecialReg sreg) = 0;

		virtual void setSpReg(MmixLlvm::SpecialReg reg, uint64_t value) = 0;

		virtual ~RegAccessor() = 0;
	};
};