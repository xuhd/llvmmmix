#include "stdafx.h"
#include "Util.h"
#include "MmixEmitPvt.h"
#include "MmixDef.h"

using llvm::LLVMContext;
using llvm::Module;
using llvm::Type;
using llvm::PointerType;
using llvm::IntegerType;
using llvm::Value;
using llvm::Function;
using llvm::BasicBlock;
using llvm::IRBuilder;
using llvm::ArrayRef;
using llvm::SwitchInst;
using llvm::Twine;
using llvm::PHINode; 
using llvm::Intrinsic::ID;

using namespace MmixLlvm::Util;
using namespace MmixLlvm::Private;
using MmixLlvm::Private::RegisterRecord;
using MmixLlvm::Private::RegistersMap;
using MmixLlvm::MXByte;

namespace {
	template<typename BitOp> 
		struct EmitCond {
			static void emit(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		};

		template<class BitOp> void EmitCond<BitOp>::emit(VerticeContext& vctx, 
			MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
		{
			
			IRBuilder<> builder(vctx.getLctx());
			builder.SetInsertPoint(vctx.getOCEntry());
			Value* yarg0 = vctx.getRegister( yarg);
			Value* zarg0 =  immediate ? builder.getInt64(zarg) : vctx.getRegister( zarg);
			Value* result = typename BitOp::emitBitOp(vctx, builder, yarg0, zarg0);
			vctx.assignRegister(xarg, result);
			builder.CreateBr(vctx.getOCExit());
		}
};

void MmixLlvm::Private::emitAnd(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateAnd(yarg, zarg);
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitOr(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateOr(yarg, zarg);
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitXor(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateXor(yarg, zarg);
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitAndn(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateAnd(yarg, builder.CreateNot(zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitOrn(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateOr(yarg, builder.CreateNot(zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitNand(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateNot(builder.CreateAnd(yarg, zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitNor(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateNot(builder.CreateOr(yarg, zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitNxor(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateNot(builder.CreateXor(yarg, zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitMux(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			Value* mreg = vctx.getSpRegister(MmixLlvm::rM);
			Value* l = builder.CreateAnd(yarg, mreg);
			Value* r = builder.CreateAnd(zarg, builder.CreateNot(mreg));
			return builder.CreateOr(l, r);
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitSadd(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	LLVMContext& ctx = vctx.getLctx();	
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.getOCEntry());
	BasicBlock *loopEntry = BasicBlock::Create(ctx, genUniq("loop_entry"), &vctx.getFunction());
	BasicBlock *loop = BasicBlock::Create(ctx, genUniq("loop"), &vctx.getFunction());
	Value* yarg0 = vctx.getRegister( yarg);
	Value* zarg0 =  immediate ? builder.getInt64(zarg) : vctx.getRegister( zarg);
	Value* arg0 = builder.CreateAnd(yarg0, builder.CreateNot(zarg0));
	builder.CreateBr(loopEntry);
	builder.SetInsertPoint(loopEntry);
	PHINode* counter = builder.CreatePHI(Type::getInt32Ty(ctx), 0);
	counter->addIncoming(builder.getInt32(0), vctx.getOCEntry());
	PHINode* var = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	var->addIncoming(arg0, vctx.getOCEntry());
	PHINode* result = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	result->addIncoming(builder.getInt64(0), vctx.getOCEntry());
	Value* cond = builder.CreateICmpULT(counter, builder.getInt32(64));
	builder.CreateCondBr(cond, loop, vctx.getOCExit());
	builder.SetInsertPoint(loop);
	Value* nextResult = builder.CreateAdd(result, builder.CreateAnd(var, builder.getInt64(1)));
	Value* nextCounter = builder.CreateAdd(counter, builder.getInt32(1));
	Value* nextVar = builder.CreateLShr(var, builder.getInt64(1));
	counter->addIncoming(nextCounter, loop);
	var->addIncoming(nextVar, loop);
	result->addIncoming(nextResult, loop);
	vctx.assignRegister(xarg, result);
	builder.CreateBr(loopEntry);
}