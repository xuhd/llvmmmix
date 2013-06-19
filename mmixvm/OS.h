#pragma once

#include <stdint.h>
#include "Engine.h"
#include "MmixDef.h"

namespace MmixLlvm {
	struct OS {
		virtual void loadExecutable(Engine& e) = 0;
		virtual MXOcta handleTrap(Engine& e, MXOcta instr, MXOcta vector) = 0;
		virtual ~OS() = 0;
	};
};