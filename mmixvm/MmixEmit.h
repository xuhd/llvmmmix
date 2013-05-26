#pragma once

#include <stdint.h>
#include <vector>
#include <boost/tuple/tuple.hpp>
#include <llvm/IR/IRBuilder.h>
#include "MemAccess.h"

namespace MmixLlvm {
	typedef std::vector<uint64_t> EdgeList;

	boost::tuple<llvm::Function*, EdgeList> emitSimpleVertice(llvm::LLVMContext& ctx, llvm::Module& m, MmixLlvm::MemAccessor& ma, uint64_t xPtr);
};