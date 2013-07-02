#include "stdafx.h"
#include "Util.h"
#include "MmixEmitPvt.h"
#include "MmixDef.h"

using llvm::LLVMContext;
using llvm::Module;
using llvm::Type;
using llvm::PointerType;
using llvm::Value;
using llvm::Argument;
using llvm::Function;
using llvm::BasicBlock;
using llvm::IRBuilder;
using llvm::ArrayRef;
using llvm::SwitchInst;
using llvm::cast;

using namespace MmixLlvm::Util;
using namespace MmixLlvm::Private;
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
		BasicBlock *condTrueBlock = vctx.makeBlock("cond_true");
		Value* xarg0 = vctx.getRegister(xarg);
		Value* cond0 = typename Cond::emitCond(builder, xarg0);
		builder.CreateCondBr(cond0, condTrueBlock, vctx.getOCExit());
		builder.SetInsertPoint(condTrueBlock);
		MXOcta target;
		if (!backward)
			target = vctx.getXPtr() + ((MXOcta)yzarg << 2);
		else 
			target = vctx.getXPtr() - ((MXOcta)yzarg << 2);
		emitLeaveVerticeViaJump(vctx, builder, target);
	}
};


void MmixLlvm::Private::emitJmp(VerticeContext& vctx, IRBuilder<>& builder,
	MXTetra xyzarg, bool backward)
{
	MXOcta target;
	if (!backward)
		target = vctx.getXPtr() + ((MXOcta)xyzarg << 2);
	else 
		target = vctx.getXPtr() - ((MXOcta)xyzarg << 2);
	emitLeaveVerticeViaJump(vctx, builder, target);
}

void MmixLlvm::Private::emitGo(VerticeContext& vctx, llvm::IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	Value* yarg0 = vctx.getRegister(yarg); 
	Value* zarg0 = immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg);
	assignRegister(vctx, builder, xarg, builder.getInt64(vctx.getXPtr() + 4));
	Value* target = builder.CreateAnd(builder.getInt64(~7LL), builder.CreateAdd(yarg0, zarg0));
	emitLeaveVerticeViaIndirectJump(vctx, builder, target);
}

void MmixLlvm::Private::emitPushgo(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	Value* yarg0 = vctx.getRegister(yarg); 
	Value* zarg0 = immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg);
	vctx.assignSpRegister(MmixLlvm::rJ, builder.getInt64(vctx.getXPtr() + 4));
	Value* target = builder.CreateAnd(builder.getInt64(~7LL), builder.CreateAdd(yarg0, zarg0));
	emitPushRegsAndLeaveVerticeViaIndirectJump(vctx, xarg, builder, target);
}


void MmixLlvm::Private::emitPushj(VerticeContext& vctx, llvm::IRBuilder<>& builder,
	MXByte xarg, MXTetra yzarg, bool backward) 
{
	MXOcta target;
	if (!backward)
		target = vctx.getXPtr() + ((MXOcta)yzarg << 2);
	else 
		target = vctx.getXPtr() - ((MXOcta)yzarg << 2);
	vctx.assignSpRegister(MmixLlvm::rJ, builder.getInt64(vctx.getXPtr() + 4));
	emitPushRegsAndLeaveVerticeViaJump(vctx, xarg, builder, target);
}

void MmixLlvm::Private::emitPop(VerticeContext& vctx, llvm::IRBuilder<>& builder,
	MXByte xarg, MXTetra yzarg)
{
	Value* target = builder.CreateAdd(vctx.getSpRegister(MmixLlvm::rJ), builder.getInt64((MXOcta)yzarg << 2));
	emitLeaveVerticeViaPop(vctx, builder, builder.getInt64(xarg), target);
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
