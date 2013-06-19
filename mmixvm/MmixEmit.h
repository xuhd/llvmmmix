#pragma once

#include <stdint.h>
#include <vector>
#include <boost/tuple/tuple.hpp>
#include <llvm/IR/IRBuilder.h>
#include "MemAccess.h"

namespace MmixLlvm {
	typedef std::vector<MXOcta> EdgeList;

	struct Vertice {
		EdgeList EdgeList;

		void (*Entry)(MXOcta* a,MXOcta* b);

		llvm::Function* Function;
	};

	void emitSimpleVertice(llvm::LLVMContext& ctx, llvm::Module& m, MmixLlvm::MemAccessor& ma, MXOcta xPtr, Vertice& out);
};