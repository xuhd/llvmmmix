#include "stdafx.h"
#include "Util.h"
#include "MmixEmitPvt.h"
#include "MmixDef.h"

using llvm::LLVMContext;
using llvm::Module;
using llvm::Type;
using llvm::PointerType;
using llvm::Value;
using llvm::Function;
using llvm::BasicBlock;
using llvm::IRBuilder;
using llvm::ArrayRef;
using llvm::SwitchInst;
using llvm::cast;

using namespace MmixLlvm::Util;
using namespace MmixLlvm::Private;
using MmixLlvm::Private::RegisterRecord;
using MmixLlvm::Private::RegistersMap;

namespace {
	template<class Cond> 
	struct EmitBranch {
		static void emit(VerticeContext& vctx, uint8_t xarg, uint32_t yzarg, bool backward);
	};

	template<class Cond> void EmitBranch<Cond>::emit(VerticeContext& vctx,
		uint8_t xarg, uint32_t yzarg, bool backward)
	{
		LLVMContext& ctx = *vctx.Ctx;
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(vctx.Entry);
		BasicBlock *condTrueBlock = BasicBlock::Create(ctx, genUniq("cond_true"), vctx.Function);
		BasicBlock *condFalseBlock = BasicBlock::Create(ctx, genUniq("cond_false"), vctx.Function);
		BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("epilogue"), vctx.Function);
		Value* xarg0 = emitRegisterLoad(vctx, builder, xarg);
		Value* cond0 = typename Cond::emitCond(builder, xarg0);
		builder.CreateCondBr(cond0, condTrueBlock, vctx.Exit);
		builder.SetInsertPoint(condTrueBlock);
		uint64_t target;
		if (!backward)
			target = vctx.XPtr + (yzarg << 2);
		else 
			target = vctx.XPtr - (yzarg << 2);
		emitLeaveVerticeViaJump(vctx, builder, target);
	}
};


void MmixLlvm::Private::emitJmp(VerticeContext& vctx, uint32_t xyzarg, bool backward) {
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	uint64_t target;
	if (!backward)
		target = vctx.XPtr + (xyzarg << 2);
	else 
		target = vctx.XPtr - (xyzarg << 2);
	emitLeaveVerticeViaJump(vctx, builder, target);
}

void MmixLlvm::Private::emitBn(VerticeContext& vctx, uint8_t xarg, uint32_t yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSLT(arg, builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, xarg, yzarg, backward);
}

void MmixLlvm::Private::emitBz(VerticeContext& vctx, uint8_t xarg, uint32_t yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpEQ(arg, builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, xarg, yzarg, backward);
}

void MmixLlvm::Private::emitBp(VerticeContext& vctx, uint8_t xarg, uint32_t yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSGT(arg, builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, xarg, yzarg, backward);
}

void MmixLlvm::Private::emitBod(VerticeContext& vctx, uint8_t xarg, uint32_t yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpNE(builder.CreateAnd(arg, builder.getInt64(1)), builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, xarg, yzarg, backward);
}

void MmixLlvm::Private::emitBnn(VerticeContext& vctx, uint8_t xarg, uint32_t yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSGE(arg, builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, xarg, yzarg, backward);
}

void MmixLlvm::Private::emitBnz(VerticeContext& vctx, uint8_t xarg, uint32_t yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpNE(arg, builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, xarg, yzarg, backward);
}

void MmixLlvm::Private::emitBnp(VerticeContext& vctx, uint8_t xarg, uint32_t yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSLE(arg, builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, xarg, yzarg, backward);
}

void MmixLlvm::Private::emitBev(VerticeContext& vctx, uint8_t xarg, uint32_t yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpEQ(builder.CreateAnd(arg, builder.getInt64(1)), builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, xarg, yzarg, backward);
}
