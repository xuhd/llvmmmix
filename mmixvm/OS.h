#pragma once

#include <stdint.h>
#include "Engine.h"
#include "MmixDef.h"

namespace MmixLlvm {
	struct OS {
		virtual void handleTrap(Engine& e) = 0;
		virtual ~OS() = 0;
	};
};