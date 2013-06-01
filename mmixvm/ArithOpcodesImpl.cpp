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
using llvm::Intrinsic::ID;

using namespace MmixLlvm::Util;
using namespace MmixLlvm::Private;
using MmixLlvm::Private::RegisterRecord;
using MmixLlvm::Private::RegistersMap;

namespace {
	void emitSetArithFlag(VerticeContext& vctx, IRBuilder<>& builder, Value* raIref, MmixLlvm::ArithFlag flag, bool val) {
		RegistersMap& sregMap = *vctx.SpecialRegMap;
		Value* ra = builder.CreateLoad(raIref);
		Value* newVal;
		if (val)
			newVal = builder.CreateOr(ra, builder.getInt64(flag));
		else
			newVal = builder.CreateAnd(ra, builder.CreateNot(builder.getInt64(flag)));
		builder.CreateStore(newVal, raIref);
	}

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
		Value* iref = builder.CreateAlloca(Type::getInt64Ty(ctx));
		builder.CreateStore(emitRegisterLoad(vctx, builder, xarg), iref);
		Value* arithFlagsIref = builder.CreateAlloca(Type::getInt64Ty(ctx));
		builder.CreateStore(emitSpecialRegisterLoad(vctx, builder, MmixLlvm::SpecialReg::rA), arithFlagsIref);
		Value* overflowFlag = builder.CreateExtractValue(resStruct, ArrayRef<uint32_t>(idx, idx + 1), "overflowFlag");
		builder.CreateCondBr(overflowFlag, overflow, success);
		builder.SetInsertPoint(success);
		idx[0] = 0;
		Value* addResult = builder.CreateExtractValue(resStruct, ArrayRef<uint32_t>(idx, idx + 1), "arith" + Twine(yarg) + Twine(zarg));
		builder.CreateStore(addResult, iref);
		builder.CreateBr(epilogue);
		builder.SetInsertPoint(overflow);
		Value* overflowFlagCondition = emitQueryArithFlag(vctx, builder, MmixLlvm::ArithFlag::O);
		builder.CreateCondBr(overflowFlagCondition, exitViaTrip, setOverflowFlag);
		builder.SetInsertPoint(setOverflowFlag);
		emitSetArithFlag(vctx, builder, arithFlagsIref, MmixLlvm::ArithFlag::O, true);
		builder.CreateBr(epilogue);
		builder.SetInsertPoint(exitViaTrip);
		emitLeaveVerticeViaTrip(vctx, builder, args[0], args[1], getArithTripVector(MmixLlvm::ArithFlag::O));
		builder.SetInsertPoint(epilogue);
		Value* result = builder.CreateLoad(iref, false);
		Value* ra = builder.CreateLoad(arithFlagsIref, false);
		builder.CreateBr(vctx.Exit);
		RegisterRecord r0;
		r0.value = result;
		r0.changed = true;
		regMap[xarg] = r0;
		r0.value = ra;
		r0.changed = true;
		sregMap[MmixLlvm::SpecialReg::rA] = r0;
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
	BasicBlock *dividerOkay1 = BasicBlock::Create(ctx, genUniq("divider_okay_1"), vctx.Function);
	BasicBlock *dividerOkay2 = BasicBlock::Create(ctx, genUniq("divider_okay_2"), vctx.Function);
	BasicBlock *overflow = BasicBlock::Create(ctx, genUniq("overflow"), vctx.Function);
	BasicBlock *divByZero = BasicBlock::Create(ctx, genUniq("div_by_zero"), vctx.Function);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("epilogue"), vctx.Function);
	Value *iprodref = builder.CreateAlloca(Type::getInt64Ty(ctx));
	Value *iremref = builder.CreateAlloca(Type::getInt64Ty(ctx));
	builder.CreateStore(emitRegisterLoad(vctx, builder, xarg), iprodref);
	builder.CreateStore(emitSpecialRegisterLoad(vctx, builder, rR), iremref);
	Value* overflowFlag = builder.CreateAnd(builder.CreateICmpEQ(yarg0, builder.getInt64(~0i64)), 
		builder.CreateICmpEQ(zarg0, builder.getInt64(-1i64)));
	builder.CreateCondBr(overflowFlag, overflow, dividerOkay1);
	builder.SetInsertPoint(dividerOkay1);
	Value* divideByZeroFlag = builder.CreateICmpEQ(zarg0, builder.getInt64(0i64));
	builder.CreateCondBr(divideByZeroFlag, divByZero, dividerOkay2);
	builder.SetInsertPoint(dividerOkay2);
	builder.CreateStore(builder.CreateSDiv(yarg0, zarg0), iprodref);
	builder.CreateStore(builder.CreateSRem(yarg0, zarg0), iremref);
	builder.CreateBr(epilogue);
	builder.SetInsertPoint(overflow);
	builder.CreateBr(epilogue);
	builder.SetInsertPoint(divByZero);	
	builder.CreateBr(epilogue);
	builder.SetInsertPoint(epilogue);
	Value* prodResult = builder.CreateLoad(iprodref, false);
	Value* remResult = builder.CreateLoad(iremref, false);
	builder.CreateBr(vctx.Exit);
	RegisterRecord r0, r1;
	r0.value = prodResult;
	r0.changed = true;
	regMap[xarg] = r0;
	r1.value = remResult;
	r1.changed = true;
	specialRegMap[rR] = r1;
}