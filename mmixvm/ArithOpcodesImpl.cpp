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
using llvm::Twine;
using llvm::PHINode; 
using llvm::Intrinsic::ID;

using namespace MmixLlvm::Util;
using namespace MmixLlvm::Private;
using MmixLlvm::Private::RegisterRecord;
using MmixLlvm::Private::RegistersMap;

namespace {
	void emitIntrinsicOp(VerticeContext& vctx, ID id, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate) {
		LLVMContext& ctx = *vctx.Ctx;
		RegistersMap& regMap = *vctx.RegMap;
		RegistersMap& sregMap = *vctx.SpecialRegMap;
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(vctx.Entry);
		Type* intrinsicArgs[] = { Type::getInt64Ty(ctx) };
		Function* intrinsic = llvm::Intrinsic::getDeclaration(vctx.Module,
			id, 
			ArrayRef<Type*>(intrinsicArgs, intrinsicArgs + 1));
		Value* args[] = {
			emitRegisterLoad(vctx, builder, yarg), 
			immediate ? builder.getInt64(zarg) : emitRegisterLoad(vctx, builder, zarg)
		};
		Value* resStruct = builder.CreateCall(intrinsic, ArrayRef<Value*>(args, args + 2));
		BasicBlock *success = BasicBlock::Create(ctx, genUniq("success"), vctx.Function);
		BasicBlock *overflow = BasicBlock::Create(ctx, genUniq("overflow"), vctx.Function);
		BasicBlock *setOverflowFlag = BasicBlock::Create(ctx, genUniq("set_overflow_flag"), vctx.Function);
		BasicBlock *exitViaTrip = BasicBlock::Create(ctx, genUniq("exit_via_trip"), vctx.Function);
		BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("epilogue"), vctx.Function);
		uint32_t idx[1];
		idx[0] = 1;
		Value* initRaVal = emitSpecialRegisterLoad(vctx, builder, MmixLlvm::rA);
		Value* overflowFlag = builder.CreateExtractValue(resStruct, ArrayRef<uint32_t>(idx, idx + 1), "overflowFlag");
		builder.CreateCondBr(overflowFlag, overflow, success);
		builder.SetInsertPoint(success);
		idx[0] = 0;
		Value* addResult = builder.CreateExtractValue(resStruct, ArrayRef<uint32_t>(idx, idx + 1), "arith" + Twine(yarg) + Twine(zarg));
		builder.CreateBr(epilogue);
		builder.SetInsertPoint(overflow);
		Value* initXRegVal = emitRegisterLoad(vctx, builder, xarg);
		Value* overflowFlagCondition = 
			builder.CreateICmpNE(
				builder.CreateAnd(initRaVal, builder.getInt64(MmixLlvm::V)),
				                  builder.getInt64(0));
		builder.CreateCondBr(overflowFlagCondition, exitViaTrip, setOverflowFlag);
		builder.SetInsertPoint(setOverflowFlag);
		Value* newRaVal = builder.CreateAnd(initRaVal, builder.CreateNot(builder.getInt64(MmixLlvm::V)));
		builder.CreateBr(epilogue);
		builder.SetInsertPoint(exitViaTrip);
		emitLeaveVerticeViaTrip(vctx, builder, args[0], args[1], getArithTripVector(MmixLlvm::V));
		builder.SetInsertPoint(epilogue);
		PHINode* result = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
		(*result).addIncoming(addResult, success);
		(*result).addIncoming(initXRegVal, setOverflowFlag);
		PHINode* ra = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
		(*ra).addIncoming(initRaVal, success);
		(*ra).addIncoming(newRaVal, setOverflowFlag);
		builder.CreateBr(vctx.Exit);
		addRegisterToCache(vctx, xarg, result, true);
		addSpecialRegisterToCache(vctx, MmixLlvm::rA, ra, true);
	}
};

void MmixLlvm::Private::emitAdd(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	emitIntrinsicOp(vctx, llvm::Intrinsic::sadd_with_overflow, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitSub(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	emitIntrinsicOp(vctx, llvm::Intrinsic::ssub_with_overflow, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitMul(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	emitIntrinsicOp(vctx, llvm::Intrinsic::smul_with_overflow, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitDiv(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	LLVMContext& ctx = *vctx.Ctx;
	RegistersMap& regMap = *vctx.RegMap;
	RegistersMap& specialRegMap = *vctx.SpecialRegMap;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* yarg0 = emitRegisterLoad(vctx, builder, yarg); 
	Value* zarg0 = immediate ? builder.getInt64(zarg) : emitRegisterLoad(vctx, builder, zarg);
	BasicBlock *success = BasicBlock::Create(ctx, genUniq("success"), vctx.Function);
	BasicBlock *keepPrecondOnError = BasicBlock::Create(ctx, genUniq("keep_precond_on_error"), vctx.Function);
	BasicBlock *overflow = BasicBlock::Create(ctx, genUniq("overflow"), vctx.Function);
	BasicBlock *divByZero = BasicBlock::Create(ctx, genUniq("div_by_zero"), vctx.Function);
	BasicBlock *setOverflowFlag = BasicBlock::Create(ctx, genUniq("set_overflow_flag"), vctx.Function);
	BasicBlock *setDivideByZeroFlag = BasicBlock::Create(ctx, genUniq("set_divide_by_zero_flag"), vctx.Function);
	BasicBlock *exitViaOverflowTrip = BasicBlock::Create(ctx, genUniq("exit_via_overflow_trip"), vctx.Function);
	BasicBlock *exitViaDivideByZeroTrip = BasicBlock::Create(ctx, genUniq("exit_via_divide_by_zero_trip"), vctx.Function);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("epilogue"), vctx.Function);
	Value* initRaVal = emitSpecialRegisterLoad(vctx, builder, rA);
	Value* overflowFlag = builder.CreateAnd(builder.CreateICmpEQ(yarg0, builder.getInt64(~0i64)), 
		builder.CreateICmpEQ(zarg0, builder.getInt64(-1i64)));
	Value* divideByZeroFlag = builder.CreateICmpEQ(zarg0, builder.getInt64(0i64));
	builder.CreateCondBr(builder.CreateOr(overflowFlag, divideByZeroFlag), keepPrecondOnError, success);
	builder.SetInsertPoint(success);
	Value* quotient = builder.CreateSDiv(yarg0, zarg0);
	Value* remainder = builder.CreateSRem(yarg0, zarg0);
	builder.CreateBr(epilogue);
	builder.SetInsertPoint(keepPrecondOnError);
	Value* initXRegVal = emitRegisterLoad(vctx, builder, xarg);
	Value* initRrVal = emitSpecialRegisterLoad(vctx, builder, rR);
	builder.CreateCondBr(divideByZeroFlag, divByZero, overflow);
	builder.SetInsertPoint(overflow);
	Value* overflowAlreadySet = 
			builder.CreateICmpNE(
				builder.CreateAnd(initRaVal, builder.getInt64(MmixLlvm::V)),
				                  builder.getInt64(0));
	builder.CreateCondBr(overflowAlreadySet, exitViaOverflowTrip, setOverflowFlag);
	builder.SetInsertPoint(setOverflowFlag);
	Value* overflowRaVal = builder.CreateOr(initRaVal, builder.getInt64(MmixLlvm::V));
	builder.CreateBr(epilogue);
	builder.SetInsertPoint(exitViaOverflowTrip);
	emitLeaveVerticeViaTrip(vctx, builder, yarg0, zarg0, getArithTripVector(MmixLlvm::V));
	builder.SetInsertPoint(divByZero);
	Value* divideByZeroAlreadySet = 
		builder.CreateICmpNE(
			builder.CreateAnd(initRaVal, builder.getInt64(MmixLlvm::D)),
				              builder.getInt64(0));
	builder.CreateCondBr(divideByZeroAlreadySet, exitViaDivideByZeroTrip, setDivideByZeroFlag);
	builder.SetInsertPoint(setDivideByZeroFlag);
	Value* divideByZeroRaVal = builder.CreateOr(initRaVal, builder.getInt64(MmixLlvm::D));
	builder.CreateBr(epilogue);
	builder.SetInsertPoint(exitViaDivideByZeroTrip);
	emitLeaveVerticeViaTrip(vctx, builder, yarg0, zarg0, getArithTripVector(MmixLlvm::D));
	builder.SetInsertPoint(epilogue);
	PHINode* quotResult = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	(*quotResult).addIncoming(quotient, success);
	(*quotResult).addIncoming(initXRegVal, setOverflowFlag);
	(*quotResult).addIncoming(initXRegVal, setDivideByZeroFlag);
	PHINode* remResult = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	(*remResult).addIncoming(remainder, success);
	(*remResult).addIncoming(initRrVal, setOverflowFlag);
	(*remResult).addIncoming(initRrVal, setDivideByZeroFlag);
	PHINode* newRa = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	(*newRa).addIncoming(initRaVal, success);
	(*newRa).addIncoming(overflowRaVal, setOverflowFlag);
	(*newRa).addIncoming(divideByZeroRaVal, setDivideByZeroFlag);
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, quotResult, true);
	addSpecialRegisterToCache(vctx, MmixLlvm::rA, newRa, true);
	addSpecialRegisterToCache(vctx, MmixLlvm::rR, remResult, true);
}