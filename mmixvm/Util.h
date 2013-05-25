#pragma once

#include <stdint.h>
#include <llvm/ADT/Twine.h>

namespace MmixLlvm {
	namespace Util {
		llvm::Twine genUniq(const llvm::Twine& prefix);
	}
}