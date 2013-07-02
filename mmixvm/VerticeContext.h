#pragma once

#include <stdint.h>
#include <llvm/IR/Intrinsics.h>
#include <boost/shared_ptr.hpp>
#include "MmixEmit.h"
#include "MmixDef.h"

namespace MmixLlvm {
	namespace Private {
		struct VerticeContext {
			virtual void feedNewOpcode(MXOcta xptr, MXTetra opcode, bool term) = 0;

			virtual MXTetra getInstr() = 0;

			virtual MXOcta getXPtr() = 0;

			virtual llvm::LLVMContext& getLctx() = 0;

			virtual llvm::BasicBlock *getOCEntry() = 0;

			virtual llvm::BasicBlock *getOCExit() = 0;

			virtual llvm::BasicBlock *makeBlock(const llvm::Twine& prefix) = 0;

			virtual std::vector<llvm::Argument*> getVerticeArgs() = 0;

			virtual llvm::Value *getModuleVar(const char* varName) = 0;

			virtual llvm::Function* getModuleFunction(const char* funcName) = 0;

			virtual llvm::Function* getIntrinsic(llvm::Intrinsic::ID id, llvm::ArrayRef<llvm::Type*> argTypes) = 0;

			virtual llvm::Value *getRegister(MmixLlvm::MXByte reg) = 0;

			virtual llvm::Value *getSpRegister(MmixLlvm::SpecialReg reg) = 0;

			virtual void assignRegister(MmixLlvm::MXByte reg, llvm::Value* val) = 0;

			virtual void assignSpRegister(MmixLlvm::SpecialReg reg, llvm::Value* val) = 0;

			virtual std::vector<MXByte> getDirtyRegisters() = 0;

			virtual std::vector<SpecialReg> getDirtySpRegisters() = 0;

			virtual void markAllClean() = 0;

			virtual boost::shared_ptr<VerticeContext> makeBranch() = 0;

			virtual ~VerticeContext() = 0;
		};
	}
}