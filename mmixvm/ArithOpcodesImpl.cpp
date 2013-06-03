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
	void emitIntrinsicOp(VerticeContext& vctx, ID id, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate) {
		LLVMContext& ctx = *vctx.Ctx;
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
		Value* newRaVal = builder.CreateOr(initRaVal, builder.getInt64(MmixLlvm::V));
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

namespace {
	template<int Pow2> class EmitAU
	{
		static Value* emitAdd(IRBuilder<>& builder, Value* yarg, Value* zarg, Twine& label);
	public:
		static void emit(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
	};

	Value* EmitAU<0>::emitAdd(IRBuilder<>& builder, Value* yarg, Value* zarg, Twine& label) {
		return builder.CreateAdd(yarg, zarg, label);
	}

	template<int Pow2> Value* EmitAU<Pow2>::emitAdd(IRBuilder<>& builder, Value* yarg, Value* zarg, Twine& label) {
		return builder.CreateAdd(builder.CreateShl(yarg, builder.getInt64(Pow2)), zarg, label);
	}

	template<int Pow2> void EmitAU<Pow2>::emit(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate) {
		LLVMContext& ctx = *vctx.Ctx;
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(vctx.Entry);
		Value* yarg0 = emitRegisterLoad(vctx, builder, yarg); 
		Value* zarg0 = immediate ? builder.getInt64(zarg) : emitRegisterLoad(vctx, builder, zarg);
		Twine label = Twine(1<<Pow2) + (immediate ? Twine("addui") : Twine("addu")) + Twine(yarg) + Twine(zarg);
		Value* result = emitAdd(builder, yarg0, zarg0, label);
		builder.CreateBr(vctx.Exit);
		addRegisterToCache(vctx, xarg, result, true);
	}
};

void MmixLlvm::Private::emitAddu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	EmitAU<0>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emit2Addu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	EmitAU<1>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emit4Addu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	EmitAU<2>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emit8Addu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	EmitAU<3>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emit16Addu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	EmitAU<4>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitSubu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* yarg0 = emitRegisterLoad(vctx, builder, yarg); 
	Value* zarg0 = immediate ? builder.getInt64(zarg) : emitRegisterLoad(vctx, builder, zarg);
	Twine label = (immediate ? Twine("subu") : Twine("subui")) + Twine(yarg) + Twine(zarg);
	Value* result = builder.CreateSub(yarg0, zarg0, label);
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitMulu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* loProd = builder.CreateAlloca(Type::getInt64Ty(ctx));
	Value* hiProd = builder.CreateAlloca(Type::getInt64Ty(ctx));
	Value* callParams[] = {
		emitRegisterLoad(vctx, builder, yarg),
		immediate ? builder.getInt64(zarg) : emitRegisterLoad(vctx, builder, zarg),
		hiProd,
		loProd
	};
	Twine label = (immediate ? Twine("mulu") : Twine("mului")) + Twine(yarg) + Twine(zarg);
	builder.CreateCall((*vctx.Module).getFunction("MuluImpl"), ArrayRef<Value*>(callParams, callParams + 4));
	Value* xval0 = builder.CreateLoad(loProd, false, label);
	Value* rh = builder.CreateLoad(hiProd);
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, xval0, true);
	addSpecialRegisterToCache(vctx, MmixLlvm::rH, rh, true);
}

void MmixLlvm::Private::emitDivu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	BasicBlock *simpleCase = BasicBlock::Create(ctx, genUniq("simple_case"), vctx.Function);
	BasicBlock *fullCase = BasicBlock::Create(ctx, genUniq("full_case"), vctx.Function);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("epilogue"), vctx.Function);
	Value* yreg = emitRegisterLoad(vctx, builder, yarg);
	Value* rd = emitSpecialRegisterLoad(vctx, builder, MmixLlvm::rD);
	Value* zreg = immediate ? builder.getInt64(zarg) : emitRegisterLoad(vctx, builder, zarg);
	Value* rdGreaterThanZreg = builder.CreateICmpUGE(rd, zreg);
	builder.CreateCondBr(rdGreaterThanZreg, simpleCase, fullCase);
	builder.SetInsertPoint(simpleCase);
	builder.CreateBr(epilogue);
	builder.SetInsertPoint(fullCase);
	Value* quotPtr = builder.CreateAlloca(Type::getInt64Ty(ctx));
	Value* remPtr = builder.CreateAlloca(Type::getInt64Ty(ctx));
	Value* callParams[] = {
		rd, yreg, zreg, quotPtr, remPtr
	};
	Twine labelq = (immediate ? Twine("divu_q") : Twine("divui_q")) + Twine(yarg) + Twine(zarg);
	Twine labelr = (immediate ? Twine("divu_r") : Twine("divui_r")) + Twine(yarg) + Twine(zarg);
	builder.CreateCall((*vctx.Module).getFunction("DivuImpl"), ArrayRef<Value*>(callParams, callParams + 5));
	Value* quotient = builder.CreateLoad(quotPtr, labelq);
	Value* remainder = builder.CreateLoad(remPtr, labelr);
	builder.CreateBr(epilogue);
	builder.SetInsertPoint(epilogue);
	PHINode* quotResult = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	(*quotResult).addIncoming(quotient, fullCase);
	(*quotResult).addIncoming(rd, simpleCase);
	PHINode* remResult = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	(*remResult).addIncoming(remainder, fullCase);
	(*remResult).addIncoming(yreg, simpleCase);
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, quotResult, true);
	addSpecialRegisterToCache(vctx, MmixLlvm::rR, remResult, true);
}
