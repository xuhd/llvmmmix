#pragma once

#include <stdint.h>
#include "RegAccess.h"
#include "MemAccess.h"

namespace MmixLlvm {
	struct Engine : public MemAccessor, public RegAccessor {
		virtual void run(MXOcta xref) = 0;
		virtual void halt() = 0;
		virtual ~Engine() = 0;
	};
};
