#pragma once

#include <stdint.h>
#include "Engine.h"
#include "MmixDef.h"

namespace MmixLlvm {
	struct OS {
		virtual void loadExecutable(Engine& e) = 0;
		virtual void handleTrap(Engine& e, uint64_t vector) = 0;
		virtual ~OS() = 0;
	};
};