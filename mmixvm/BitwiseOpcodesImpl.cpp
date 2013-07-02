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
//using MmixLlvm::Private::RegisterRecord;
//using MmixLlvm::Private::RegistersMap;
using MmixLlvm::MXByte;

namespace {
	template<typename BitOp> 
		struct EmitCond {
			static void emit(VerticeContext& vctx, IRBuilder<>& builder,
				MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		};

		template<class BitOp> void EmitCond<BitOp>::emit(VerticeContext& vctx, 
			IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
		{
			Value* yarg0 = vctx.getRegister( yarg);
			Value* zarg0 =  immediate ? builder.getInt64(zarg) : vctx.getRegister( zarg);
			Value* result = typename BitOp::emitBitOp(vctx, builder, yarg0, zarg0);
			assignRegister(vctx, builder, xarg, result);
			builder.CreateBr(vctx.getOCExit());
		}
};

void MmixLlvm::Private::emitAnd(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateAnd(yarg, zarg);
		}
	};
	EmitCond<Impl>::emit(vctx, builder, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitOr(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateOr(yarg, zarg);
		}
	};
	EmitCond<Impl>::emit(vctx, builder, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitXor(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateXor(yarg, zarg);
		}
	};
	EmitCond<Impl>::emit(vctx, builder, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitAndn(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateAnd(yarg, builder.CreateNot(zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, builder, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitOrn(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateOr(yarg, builder.CreateNot(zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, builder, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitNand(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateNot(builder.CreateAnd(yarg, zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, builder, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitNor(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateNot(builder.CreateOr(yarg, zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, builder, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitNxor(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			return builder.CreateNot(builder.CreateXor(yarg, zarg));
		}
	};
	EmitCond<Impl>::emit(vctx, builder, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitMux(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Impl {
		static Value* emitBitOp(VerticeContext& vctx, IRBuilder<>& builder, Value* yarg, Value* zarg) {
			Value* mreg = vctx.getSpRegister(MmixLlvm::rM);
			Value* l = builder.CreateAnd(yarg, mreg);
			Value* r = builder.CreateAnd(zarg, builder.CreateNot(mreg));
			return builder.CreateOr(l, r);
		}
	};
	EmitCond<Impl>::emit(vctx, builder, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitSadd(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	LLVMContext& ctx = vctx.getLctx();	
	BasicBlock *loopEntry = vctx.makeBlock("loop_entry");
	BasicBlock *loop = vctx.makeBlock("loop");
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
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(loopEntry);
}

/*
entry:			
	br loop0;
loop0:
	retVal = phi [0 : entry], [newRetVal : loop0_cont];
	i = phi [0 : entry], [newI : loop0_cont];
	b = ((z >> (i << 3)) & 0xFF);
	br loop1
loop1:
	j = phi [0 : loop0], [newJ : loop1_cont];
	cond0 = (b & (1 << j)) != 0);
	if cond0 br doOr else br skip;
doOr:
	q1 = retVal bitor ((y >> (j << 3)) & 0xFF) << (i << 3);
	br loop1_cont;
skip:
	br loop1_cont;
loop1_cont:
	newRetVal = phi [doOr: q1],[skip: retVal];
	newJ = j + 1;
	if newJ < 8 br loop1 else br loop0_cont;
loop0_cont:
	newI = i + 1;
	if newI < 8 br loop0 else br epilogue;
epilogue: ret retVal*/
void MmixLlvm::Private::emitMor(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	Value* callParams[] = {
		vctx.getRegister( yarg),
		immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg),
	};
	Twine label = (immediate ? Twine("mor") : Twine("mori")) + Twine(yarg) + Twine(zarg);
	Value* result = builder.CreateCall(vctx.getModuleFunction("MorImpl"), ArrayRef<Value*>(callParams, callParams + 2));
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitMxor(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	Value* callParams[] = {
		vctx.getRegister( yarg),
		immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg),
	};
	Twine label = (immediate ? Twine("mxor") : Twine("mxori")) + Twine(yarg) + Twine(zarg);
	Value* result = builder.CreateCall(vctx.getModuleFunction("MxorImpl"), ArrayRef<Value*>(callParams, callParams + 2));
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

/*
	LLVMContext& ctx = vctx.getLctx();	
	BasicBlock *loop0 = BasicBlock::Create(ctx, genUniq("loop0"), &vctx.getFunction());
	BasicBlock *loop1 = BasicBlock::Create(ctx, genUniq("loop1"), &vctx.getFunction());
	BasicBlock *doOr = BasicBlock::Create(ctx, genUniq("do_or"), &vctx.getFunction());
	BasicBlock *skip = BasicBlock::Create(ctx, genUniq("skip"), &vctx.getFunction());
	BasicBlock *loop1Cont = BasicBlock::Create(ctx, genUniq("loop1_cont"), &vctx.getFunction());
	BasicBlock *loop0Cont = BasicBlock::Create(ctx, genUniq("loop0_cont"), &vctx.getFunction());
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("epilogue"), &vctx.getFunction());
	Value* yarg0 = vctx.getRegister( yarg);
	Value* zarg0 =  immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg);
	builder.CreateBr(loop0);
	builder.SetInsertPoint(loop0);
	PHINode* retVal = builder.CreatePHI(builder.getInt64Ty(), 2);
	retVal->addIncoming(builder.getInt64(0LL), vctx.getOCEntry());
	PHINode* i = builder.CreatePHI(builder.getInt64Ty(), 2);
	i->addIncoming(builder.getInt64(0LL), vctx.getOCEntry());
	// b = ((z >> (i << 3)) & 0xFF);
	Value* b =  builder.CreateAnd(
		builder.CreateLShr(zarg0,
			builder.CreateShl(i,
				builder.getInt64(3))),
			builder.getInt64(0xFF));
	builder.CreateBr(loop1);
	builder.SetInsertPoint(loop1);
	PHINode* j = builder.CreatePHI(builder.getInt64Ty(), 2);
	j->addIncoming(builder.getInt64(0LL), loop0);
	// cond0 = (b & (1 << j)) != 0);
	Value* cond = builder.CreateICmpNE(
		builder.CreateAnd(b, 
			builder.CreateShl(builder.getInt64(1LL), j)),
		builder.getInt64(0LL));
	builder.CreateCondBr(cond, doOr, skip);
	builder.SetInsertPoint(doOr);
	//retVal bitor ((y >> (j << 3)) & 0xFF) << (i << 3)
	Value* q1 = builder.CreateOr(retVal,
		builder.CreateShl(
			builder.CreateAnd(
				builder.CreateLShr(yarg0, 
					builder.CreateShl(j, builder.getInt64(3LL))),
				builder.getInt64(0xFF)),
			builder.CreateShl(i, builder.getInt64(3LL))));
	builder.CreateBr(loop1Cont);
	builder.SetInsertPoint(skip);
	builder.CreateBr(loop1Cont);
	builder.SetInsertPoint(loop1Cont);
	PHINode* newRetVal = builder.CreatePHI(builder.getInt64Ty(), 2);
	newRetVal->addIncoming(q1, doOr);
	newRetVal->addIncoming(retVal, skip);
	retVal->addIncoming(newRetVal, loop0Cont);
	Value* newJ = builder.CreateAdd(j, builder.getInt64(1LL));
	j->addIncoming(newJ, loop1Cont);
	builder.CreateCondBr(builder.CreateICmpULT(newJ, builder.getInt64(8LL)), loop1, loop0Cont);
	builder.SetInsertPoint(loop0Cont);
	Value* newI = builder.CreateAdd(i, builder.getInt64(1LL));
	i->addIncoming(newI, loop0Cont);
	builder.CreateCondBr(builder.CreateICmpULT(newI, builder.getInt64(8LL)), loop0, epilogue);
	builder.SetInsertPoint(epilogue);
	assignRegister(vctx, builder, xarg, newRetVal);
	builder.CreateBr(vctx.getOCExit());*/