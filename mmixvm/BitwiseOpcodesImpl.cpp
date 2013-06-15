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

namespace {
	template<typename BitOp> 
		struct EmitCond {
			static void emit(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
		};

		template<class BitOp> void EmitCond<BitOp>::emit(VerticeContext& vctx, 
			uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
		{
			LLVMContext& ctx = *vctx.Ctx;
			IRBuilder<> builder(ctx);
			builder.SetInsertPoint(vctx.Entry);
			Value* yarg0 = emitRegisterLoad(vctx, builder, yarg);
			Value* zarg0 =  immediate ? builder.getInt64(zarg) : emitRegisterLoad(vctx, builder, zarg);
			Value* result = typename BitOp::emitBitOp(vctx, builder, yarg0, zarg0);
			builder.CreateBr(vctx.Exit);
			addRegisterToCache(vctx, xarg, result, true);
		}
};

void MmixLlvm::Private::emitAnd(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateAnd(yarg, zarg);
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitOr(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateOr(yarg, zarg);
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitXor(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateXor(yarg, zarg);
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitAndn(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateAnd(yarg, builder.CreateNot(zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitOrn(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateOr(yarg, builder.CreateNot(zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitNand(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateNot(builder.CreateAnd(yarg, zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitNor(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateNot(builder.CreateOr(yarg, zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitNxor(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateNot(builder.CreateXor(yarg, zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitMux(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			Value* mreg = emitSpecialRegisterLoad(vctx, builder, MmixLlvm::rM);
			Value* l = builder.CreateAnd(yarg, mreg);
			Value* r = builder.CreateAnd(zarg, builder.CreateNot(mreg));
			return builder.CreateOr(l, r);
		}
	};
	EmitCond<Impl>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitSadd(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	BasicBlock *loopEntry = BasicBlock::Create(ctx, genUniq("loop_entry"), vctx.Function);
	BasicBlock *loop = BasicBlock::Create(ctx, genUniq("loop"), vctx.Function);
	Value* yarg0 = emitRegisterLoad(vctx, builder, yarg);
	Value* zarg0 =  immediate ? builder.getInt64(zarg) : emitRegisterLoad(vctx, builder, zarg);
	Value* arg0 = builder.CreateAnd(yarg0, builder.CreateNot(zarg0));
	builder.CreateBr(loopEntry);
	builder.SetInsertPoint(loopEntry);
	PHINode* counter = builder.CreatePHI(Type::getInt32Ty(ctx), 0);
	counter->addIncoming(builder.getInt32(0), vctx.Entry);
	PHINode* var = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	var->addIncoming(arg0, vctx.Entry);
	PHINode* result = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	result->addIncoming(builder.getInt64(0), vctx.Entry);
	Value* cond = builder.CreateICmpULT(counter, builder.getInt32(64));
	builder.CreateCondBr(cond, loop, vctx.Exit);
	builder.SetInsertPoint(loop);
	Value* nextResult = builder.CreateAdd(result, builder.CreateAnd(var, builder.getInt64(1)));
	Value* nextCounter = builder.CreateAdd(counter, builder.getInt32(1));
	Value* nextVar = builder.CreateLShr(var, builder.getInt64(1));
	counter->addIncoming(nextCounter, loop);
	var->addIncoming(nextVar, loop);
	result->addIncoming(nextResult, loop);
	builder.CreateBr(loopEntry);
	addRegisterToCache(vctx, xarg, result, true);
}