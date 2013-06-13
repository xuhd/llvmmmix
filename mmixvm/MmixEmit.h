#pragma once

#include <stdint.h>
#include <vector>
#include <boost/tuple/tuple.hpp>
#include <llvm/IR/IRBuilder.h>
#include "MemAccess.h"

namespace MmixLlvm {
	typedef std::vector<uint64_t> EdgeList;

	struct Vertice {
		EdgeList EdgeList;

		llvm::Function* Function;
	};

	void emitSimpleVertice(llvm::LLVMContext& ctx, llvm::Module& m, MmixLlvm::MemAccessor& ma, uint64_t xPtr, Vertice& out);
};