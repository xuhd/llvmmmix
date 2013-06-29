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
using MmixLlvm::MXTetra;

namespace {
	void emitIntrinsicOp(VerticeContext& vctx, IRBuilder<>& builder,
		ID id, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate) 
	{
		LLVMContext& ctx = vctx.getLctx();
		Type* intrinsicArgs[] = { Type::getInt64Ty(ctx) };
		Function* intrinsic = vctx.getIntrinsic(id, ArrayRef<Type*>(intrinsicArgs, intrinsicArgs + 1));
		Value* args[] = {
			vctx.getRegister(yarg), 
			immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg)
		};
		Value* resStruct = builder.CreateCall(intrinsic, ArrayRef<Value*>(args, args + 2));
		BasicBlock *success = vctx.makeBlock("success");
		BasicBlock *overflow = vctx.makeBlock("overflow");
		BasicBlock *setOverflowFlag = vctx.makeBlock("set_overflow_flag");
		BasicBlock *exitViaTrip = vctx.makeBlock("exit_via_trip");
		BasicBlock *epilogue = vctx.makeBlock("epilogue");
		MXTetra idx[1];
		idx[0] = 1;
		Value* initRaVal = vctx.getSpRegister(MmixLlvm::rA);
		Value* overflowFlag = builder.CreateExtractValue(resStruct, ArrayRef<MXTetra>(idx, idx + 1), "overflowFlag");
		builder.CreateCondBr(overflowFlag, overflow, success);
		builder.SetInsertPoint(success);
		idx[0] = 0;
		Value* addResult = builder.CreateExtractValue(resStruct, ArrayRef<MXTetra>(idx, idx + 1), "arith" + Twine(yarg) + Twine(zarg));
		builder.CreateBr(epilogue);
		builder.SetInsertPoint(overflow);
		Value* initXRegVal = vctx.getRegister( xarg);
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
		result->addIncoming(addResult, success);
		result->addIncoming(initXRegVal, setOverflowFlag);
		PHINode* ra = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
		ra->addIncoming(initRaVal, success);
		ra->addIncoming(newRaVal, setOverflowFlag);
		vctx.assignRegister(xarg, result);
		vctx.assignSpRegister(MmixLlvm::rA, ra);
		builder.CreateBr(vctx.getOCExit());
	}
};

void MmixLlvm::Private::emitAdd(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	emitIntrinsicOp(vctx, builder, llvm::Intrinsic::sadd_with_overflow, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitSub(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	emitIntrinsicOp(vctx, builder, llvm::Intrinsic::ssub_with_overflow, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitMul(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	emitIntrinsicOp(vctx, builder, llvm::Intrinsic::smul_with_overflow, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitDiv(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	LLVMContext& ctx = vctx.getLctx();
	Value* yarg0 = vctx.getRegister(yarg); 
	Value* zarg0 = immediate ? builder.getInt64(zarg) : vctx.getRegister( zarg);
	BasicBlock *success = vctx.makeBlock("success");
	BasicBlock *keepPrecondOnError = vctx.makeBlock("keep_precond_on_error");
	BasicBlock *overflow = vctx.makeBlock("overflow");
	BasicBlock *divByZero = vctx.makeBlock("div_by_zero");
	BasicBlock *setOverflowFlag = vctx.makeBlock("set_overflow_flag");
	BasicBlock *setDivideByZeroFlag = vctx.makeBlock("set_divide_by_zero_flag");
	BasicBlock *exitViaOverflowTrip = vctx.makeBlock("exit_via_overflow_trip");
	BasicBlock *exitViaDivideByZeroTrip = vctx.makeBlock("exit_via_divide_by_zero_trip");
	BasicBlock *epilogue = vctx.makeBlock("epilogue");
	Value* initRaVal = vctx.getSpRegister(rA);
	Value* overflowFlag = builder.CreateAnd(builder.CreateICmpEQ(yarg0, builder.getInt64(~0i64)), 
		builder.CreateICmpEQ(zarg0, builder.getInt64(-1i64)));
	Value* divideByZeroFlag = builder.CreateICmpEQ(zarg0, builder.getInt64(0i64));
	builder.CreateCondBr(builder.CreateOr(overflowFlag, divideByZeroFlag), keepPrecondOnError, success);
	builder.SetInsertPoint(success);
	Value* quotient = builder.CreateSDiv(yarg0, zarg0);
	Value* remainder = builder.CreateSRem(yarg0, zarg0);
	builder.CreateBr(epilogue);
	builder.SetInsertPoint(keepPrecondOnError);
	Value* initXRegVal = vctx.getRegister(xarg);
	Value* initRrVal = vctx.getSpRegister(rR);
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
	quotResult->addIncoming(quotient, success);
	quotResult->addIncoming(initXRegVal, setOverflowFlag);
	quotResult->addIncoming(initXRegVal, setDivideByZeroFlag);
	PHINode* remResult = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	remResult->addIncoming(remainder, success);
	remResult->addIncoming(initRrVal, setOverflowFlag);
	remResult->addIncoming(initRrVal, setDivideByZeroFlag);
	PHINode* newRa = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	newRa->addIncoming(initRaVal, success);
	newRa->addIncoming(overflowRaVal, setOverflowFlag);
	newRa->addIncoming(divideByZeroRaVal, setDivideByZeroFlag);
	vctx.assignRegister(xarg, quotResult);
	vctx.assignSpRegister(MmixLlvm::rA, newRa);
	vctx.assignSpRegister(MmixLlvm::rR, remResult);
	builder.CreateBr(vctx.getOCExit());
}

namespace {
	template<int Pow2> class EmitAU
	{
		static Value* emitAdd(IRBuilder<>& builder, Value* yarg, Value* zarg, Twine& label);
	public:
		static void emit(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
	};

	Value* EmitAU<0>::emitAdd(IRBuilder<>& builder, Value* yarg, Value* zarg, Twine& label) {
		return builder.CreateAdd(yarg, zarg, label);
	}

	template<int Pow2> Value* EmitAU<Pow2>::emitAdd(IRBuilder<>& builder, Value* yarg, Value* zarg, Twine& label) {
		return builder.CreateAdd(builder.CreateShl(yarg, builder.getInt64(Pow2)), zarg, label);
	}

	template<int Pow2> void EmitAU<Pow2>::emit(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate) {
		Value* yarg0 = vctx.getRegister(yarg); 
		Value* zarg0 = immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg);
		Twine label = Twine(1<<Pow2) + (immediate ? Twine("addui") : Twine("addu")) + Twine(yarg) + Twine(zarg);
		Value* result = emitAdd(builder, yarg0, zarg0, label);
		vctx.assignRegister(xarg, result);
		builder.CreateBr(vctx.getOCExit());
	}
};

void MmixLlvm::Private::emitAddu(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	EmitAU<0>::emit(vctx, builder, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emit2Addu(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	EmitAU<1>::emit(vctx, builder, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emit4Addu(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	EmitAU<2>::emit(vctx, builder, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emit8Addu(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	EmitAU<3>::emit(vctx, builder, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emit16Addu(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	EmitAU<4>::emit(vctx, builder, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitSubu(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	Value* yarg0 = vctx.getRegister(yarg); 
	Value* zarg0 = immediate ? builder.getInt64(zarg) : vctx.getRegister( zarg);
	Twine label = (immediate ? Twine("subu") : Twine("subui")) + Twine(yarg) + Twine(zarg);
	Value* result = builder.CreateSub(yarg0, zarg0, label);
	vctx.assignRegister(xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitMulu(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	LLVMContext& ctx = vctx.getLctx();
	Value* loProd = builder.CreateAlloca(Type::getInt64Ty(ctx));
	Value* hiProd = builder.CreateAlloca(Type::getInt64Ty(ctx));
	Value* callParams[] = {
		vctx.getRegister( yarg),
		immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg),
		hiProd,
		loProd
	};
	Twine label = (immediate ? Twine("mulu") : Twine("mului")) + Twine(yarg) + Twine(zarg);
	builder.CreateCall(vctx.getModuleFunction("MuluImpl"), ArrayRef<Value*>(callParams, callParams + 4));
	Value* xval0 = builder.CreateLoad(loProd, false, label);
	Value* rh = builder.CreateLoad(hiProd);
	vctx.assignRegister(xarg, xval0);
	vctx.assignSpRegister(MmixLlvm::rH, rh);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitDivu(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	LLVMContext& ctx = vctx.getLctx();
	BasicBlock *simpleCase = vctx.makeBlock("simple_case");
	BasicBlock *fullCase = vctx.makeBlock("full_case");
	BasicBlock *epilogue = vctx.makeBlock("epilogue");
	Value* yreg = vctx.getRegister( yarg);
	Value* rd = vctx.getSpRegister(MmixLlvm::rD);
	Value* zreg = immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg);
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
	builder.CreateCall(vctx.getModuleFunction("DivuImpl"), ArrayRef<Value*>(callParams, callParams + 5));
	Value* quotient = builder.CreateLoad(quotPtr, labelq);
	Value* remainder = builder.CreateLoad(remPtr, labelr);
	builder.CreateBr(epilogue);
	builder.SetInsertPoint(epilogue);
	PHINode* quotResult = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	quotResult->addIncoming(quotient, fullCase);
	quotResult->addIncoming(rd, simpleCase);
	PHINode* remResult = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	remResult->addIncoming(remainder, fullCase);
	remResult->addIncoming(yreg, simpleCase);
	vctx.assignRegister(xarg, quotResult);
	vctx.assignSpRegister(MmixLlvm::rR, remResult);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitNeg(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate) 
{
	Value* yarg0 = builder.getInt64(yarg);
	Value* zarg0 =  immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg);
	Value* xval0 = builder.CreateSub(yarg0, zarg0);
	vctx.assignRegister(xarg, xval0);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitNegu(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	Value* yarg0 = builder.getInt64(yarg);
	Value* zarg0 =  immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg);
	Value* xval0 = builder.CreateSub(yarg0, zarg0);
	vctx.assignRegister(xarg, xval0);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitSr(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	LLVMContext& ctx = vctx.getLctx();
	BasicBlock *yPositiveBlock = vctx.makeBlock("y_positive");
	BasicBlock *yNegativeBlock = vctx.makeBlock("y_negative");
	BasicBlock *yDefBlock = vctx.makeBlock("y_def");
	BasicBlock *epilogue = vctx.makeBlock("epilogue");
	Value* yarg0 = vctx.getRegister(yarg);
	Value* zarg0 =  immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg);
	Value* yIsPositive = builder.CreateICmpSGE(yarg0, builder.getInt64(0));
	builder.CreateCondBr(yIsPositive, yPositiveBlock, yNegativeBlock);
	builder.SetInsertPoint(yPositiveBlock);
	builder.CreateBr(yDefBlock);
	builder.SetInsertPoint(yNegativeBlock);
	Value* mask0 = builder.CreateShl(builder.getInt64(~0ui64), builder.CreateSub(builder.getInt64(64), zarg0));
	builder.CreateBr(yDefBlock);
	builder.SetInsertPoint(yDefBlock);
	PHINode* mask = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	mask->addIncoming(builder.getInt64(0), yPositiveBlock);
	mask->addIncoming(mask0, yNegativeBlock);
	Value* result = builder.CreateOr(builder.CreateLShr(yarg0, zarg0), mask);
	vctx.assignRegister(xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitSru(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	Value* yarg0 = vctx.getRegister( yarg);
	Value* zarg0 =  immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg);
	Value* result = builder.CreateLShr(yarg0, zarg0);
	vctx.assignRegister(xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitSl(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	LLVMContext& ctx = vctx.getLctx();	
	BasicBlock *maskBlock0 = vctx.makeBlock("mask_block_0");
	BasicBlock *maskBlock1 = vctx.makeBlock("mask_block_1");
	BasicBlock *maskDef = vctx.makeBlock("mask_def");
	BasicBlock *yPositive = vctx.makeBlock("y_positive");
	BasicBlock *yNegative = vctx.makeBlock("y_negative");
	BasicBlock *yDef = vctx.makeBlock("y_def");
	BasicBlock *successBlock = vctx.makeBlock("success");
	BasicBlock *overflowBlock = vctx.makeBlock("overflow");
	BasicBlock *setOverflowFlag = vctx.makeBlock("set_overflow_flag");
	BasicBlock *exitViaOverflowTrip = vctx.makeBlock("exit_via_overflow_trip");
	BasicBlock *epilogue = vctx.makeBlock("epilogue");
	Value* yarg0 = vctx.getRegister( yarg);
	Value* zarg0 = immediate ? builder.getInt64(zarg) : vctx.getRegister( zarg);
	Value* b0 = builder.CreateICmpULT(zarg0, builder.getInt64(63));
	builder.CreateCondBr(b0, maskBlock0, maskBlock1);
	builder.SetInsertPoint(maskBlock0);
	/*long.MaxValue << LONG_WITDH - (a + 1)*/
	Value* mask0 = builder.CreateShl(builder.getInt64(-1i64), 
		builder.CreateSub(builder.getInt64(64), builder.CreateAdd(zarg0, builder.getInt64(1)))); 
	builder.CreateBr(maskDef);
	builder.SetInsertPoint(maskBlock1);
	/*long.MaxValue << LONG_WITDH - (a + 1)*/
	Value* mask1 = builder.getInt64(-1i64); 
	builder.CreateBr(maskDef);
	builder.SetInsertPoint(maskDef);
	PHINode* mask = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	mask->addIncoming(mask0, maskBlock0);
	mask->addIncoming(mask1, maskBlock1);
	Value* b1 = builder.CreateICmpSGE(yarg0, builder.getInt64(0));
	/*(val >= 0 ? (ulong)val : ~(ulong)val)*/
	builder.CreateCondBr(b1, yPositive, yNegative);
	builder.SetInsertPoint(yPositive);
	builder.CreateBr(yDef);
	builder.SetInsertPoint(yNegative);
	Value* notYarg0 = builder.CreateNot(yarg0);
	builder.CreateBr(yDef);
	builder.SetInsertPoint(yDef);
	PHINode* y0 = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	y0->addIncoming(yarg0, yPositive);
	y0->addIncoming(notYarg0, yNegative);
	Value* initRaVal = vctx.getSpRegister(rA);
	Value* overflow = builder.CreateICmpNE(builder.CreateAnd(y0, mask), builder.getInt64(0));
	builder.CreateCondBr(overflow, overflowBlock, successBlock);
	builder.SetInsertPoint(successBlock);
	Value* result = builder.CreateShl(yarg0, zarg0);
	builder.CreateBr(epilogue);
	builder.SetInsertPoint(overflowBlock);
	Value* initXRegVal = vctx.getRegister(xarg);
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
	builder.SetInsertPoint(epilogue);
	PHINode* shlResult = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	shlResult->addIncoming(result, successBlock);
	shlResult->addIncoming(initXRegVal, setOverflowFlag);
	PHINode* newRa = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
	newRa->addIncoming(initRaVal, successBlock);
	newRa->addIncoming(overflowRaVal, setOverflowFlag);
	vctx.assignRegister(xarg, shlResult);
	vctx.assignSpRegister(MmixLlvm::rA, newRa);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitSlu(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	Value* yarg0 = vctx.getRegister(yarg);
	Value* zarg0 =  immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg);
	Value* result = builder.CreateShl(yarg0, zarg0);
	vctx.assignRegister(xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitCmp(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	LLVMContext& ctx = vctx.getLctx();		
	Value* yarg0 = vctx.getRegister(yarg);
	Value* zarg0 = immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg);
	Value* val0 = builder.CreateIntCast(builder.CreateICmpSGT(yarg0, zarg0), Type::getInt64Ty(ctx), false);
	Value* val1 = builder.CreateIntCast(builder.CreateICmpSLT(yarg0, zarg0), Type::getInt64Ty(ctx), false);
	Value* result = builder.CreateSub(val0, val1);
	vctx.assignRegister(xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitCmpu(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	LLVMContext& ctx = vctx.getLctx();	
	Value* yarg0 = vctx.getRegister(yarg);
	Value* zarg0 = immediate ? builder.getInt64(zarg) : vctx.getRegister( zarg);
	Value* val0 = builder.CreateIntCast(builder.CreateICmpUGT(yarg0, zarg0), Type::getInt64Ty(ctx), false);
	Value* val1 = builder.CreateIntCast(builder.CreateICmpULT(yarg0, zarg0), Type::getInt64Ty(ctx), false);
	Value* result = builder.CreateSub(val0, val1);
	vctx.assignRegister(xarg, result);
	builder.CreateBr(vctx.getOCExit());
}
