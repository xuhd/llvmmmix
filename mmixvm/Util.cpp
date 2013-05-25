#include "stdafx.h"
#include "Util.h"

using llvm::Twine;

using namespace MmixLlvm::Util;

namespace {
	int Id = 0;
}

Twine MmixLlvm::Util::genUniq(const llvm::Twine& prefix) {
	return llvm::Twine(prefix) + (llvm::Twine(++Id));
}