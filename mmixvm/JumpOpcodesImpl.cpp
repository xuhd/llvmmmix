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
using MmixLlvm::MXByte;
using MmixLlvm::MXTetra;
using MmixLlvm::MXOcta;

namespace {
	template<class Cond> 
	struct EmitBranch {
		static void emit(VerticeContext& vctx, IRBuilder<>& builder, 
			MXByte xarg, MXTetra yzarg, bool backward);
	};

	template<class Cond> void EmitBranch<Cond>::emit(VerticeContext& vctx,
		IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward)
	{
		LLVMContext& ctx = vctx.getLctx();
		BasicBlock *condTrueBlock = BasicBlock::Create(ctx, genUniq("cond_true"), &vctx.getFunction());
		Value* xarg0 = vctx.getRegister(xarg);
		Value* cond0 = typename Cond::emitCond(builder, xarg0);
		builder.CreateCondBr(cond0, condTrueBlock, vctx.getOCExit());
		builder.SetInsertPoint(condTrueBlock);
		MXOcta target;
		if (!backward)
			target = vctx.getXPtr() + (yzarg << 2);
		else 
			target = vctx.getXPtr() - (yzarg << 2);
		emitLeaveVerticeViaJump(vctx, builder, target);
	}
};


void MmixLlvm::Private::emitJmp(VerticeContext& vctx, IRBuilder<>& builder,
	MXTetra xyzarg, bool backward)
{
	MXOcta target;
	if (!backward)
		target = vctx.getXPtr() + (xyzarg << 2);
	else 
		target = vctx.getXPtr() - (xyzarg << 2);
	emitLeaveVerticeViaJump(vctx, builder, target);
}

void MmixLlvm::Private::emitBn(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSLT(arg, builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, builder, xarg, yzarg, backward);
}

void MmixLlvm::Private::emitBz(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpEQ(arg, builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, builder, xarg, yzarg, backward);
}

void MmixLlvm::Private::emitBp(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSGT(arg, builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, builder, xarg, yzarg, backward);
}

void MmixLlvm::Private::emitBod(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpNE(builder.CreateAnd(arg, builder.getInt64(1)), builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, builder, xarg, yzarg, backward);
}

void MmixLlvm::Private::emitBnn(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSGE(arg, builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, builder, xarg, yzarg, backward);
}

void MmixLlvm::Private::emitBnz(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpNE(arg, builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, builder, xarg, yzarg, backward);
}

void MmixLlvm::Private::emitBnp(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSLE(arg, builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, builder, xarg, yzarg, backward);
}

void MmixLlvm::Private::emitBev(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward) {
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpEQ(builder.CreateAnd(arg, builder.getInt64(1)), builder.getInt64(0));
		}
	};
	EmitBranch<Cond>::emit(vctx, builder, xarg, yzarg, backward);
}
