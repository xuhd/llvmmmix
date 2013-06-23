#include "stdafx.h"
#include "Util.h"
#include "MmixEmitPvt.h"
#include "MmixDef.h"

using llvm::LLVMContext;
using llvm::Module;
using llvm::Type;
using llvm::PointerType;
using llvm::Value;
using llvm::PHINode;
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

namespace {
	template<int Pow2> class EmitS
	{
		const static int64_t LoBound = ~0i64 << ((1 << Pow2) * 8 - 1);
		const static int64_t HiBound = -1i64 - LoBound;
		static Value* makeA(LLVMContext& ctx, IRBuilder<>& builder, Value* yVal, Value* zVal);
		static Value* adjustEndianness(VerticeContext& vctx, IRBuilder<>& builder, Value* val);
		static Value* createStoreCast(LLVMContext& ctx, IRBuilder<>& builder, Value* val, int isSigned);
	public:
		static void emit(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		static void emitu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		static void emitht(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		static void emitco(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
	};

	void EmitS<3>::emit(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
	{
		emitu(vctx, xarg, yarg, zarg, immediate);
	}

	template<int Pow2> void EmitS<Pow2>::emit(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
	{
		IRBuilder<> builder(vctx.getLctx());
		builder.SetInsertPoint(vctx.getOCEntry());
		Value* xVal = vctx.getRegister( xarg);
		Value* loBoundCk = builder.CreateICmpSGE(xVal, builder.getInt64(LoBound));
		Value* hiBoundCk = builder.CreateICmpSLE(xVal, builder.getInt64(HiBound));
		BasicBlock *success = BasicBlock::Create(vctx.getLctx(), genUniq("success"), &vctx.getFunction());
		BasicBlock *overflow = BasicBlock::Create(vctx.getLctx(), genUniq("overflow"), &vctx.getFunction());
		BasicBlock *setOverflowFlag = BasicBlock::Create(vctx.getLctx(), genUniq("set_overflow_flag"), &vctx.getFunction());
		BasicBlock *exitViaOverflowTrip = BasicBlock::Create(vctx.getLctx(), genUniq("exit_via_overflow_trip"), &vctx.getFunction());
		BasicBlock *epilogue = BasicBlock::Create(vctx.getLctx(), genUniq("epilogue"), &vctx.getFunction());
		Value* initRaVal = vctx.getSpRegister(MmixLlvm::rA);
		Value* yVal = vctx.getRegister( yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : vctx.getRegister( zarg);
		Value* theA = makeA(vctx.getLctx(), builder, yVal, zVal);
		builder.CreateCondBr(builder.CreateAnd(loBoundCk, hiBoundCk), success, overflow);
		builder.SetInsertPoint(success);
		Value* valToStore = createStoreCast(vctx.getLctx(), builder, xVal, true);
		emitStoreMem(vctx.getLctx(), vctx.getModule(), vctx.getFunction(), builder, theA, adjustEndianness(vctx, builder, valToStore));
		builder.CreateBr(epilogue);
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
		emitLeaveVerticeViaTrip(vctx, builder, theA, xVal, getArithTripVector(MmixLlvm::V));
		builder.SetInsertPoint(epilogue);
		PHINode* ra = builder.CreatePHI(Type::getInt64Ty(vctx.getLctx()), 0);
		ra->addIncoming(initRaVal, success);
		ra->addIncoming(overflowRaVal, setOverflowFlag);
		vctx.assignSpRegister(MmixLlvm::rA, ra);
		builder.CreateBr(vctx.getOCExit());
		//addSpecialRegisterToCache(vctx, MmixLlvm::rA, ra, true);
	}

	template<int Pow2> void EmitS<Pow2>::emitu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
	{
		IRBuilder<> builder(vctx.getLctx());
		builder.SetInsertPoint(vctx.getOCEntry());
		Value* xVal = vctx.getRegister( xarg);
		Value* yVal = vctx.getRegister( yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : vctx.getRegister( zarg);
		Value* theA = makeA(vctx.getLctx(), builder, yVal, zVal);
		Value* valToStore = createStoreCast(vctx.getLctx(), builder, xVal, false);
		emitStoreMem(vctx.getLctx(), vctx.getModule(), vctx.getFunction(), builder, theA, adjustEndianness(vctx, builder, valToStore));
		builder.CreateBr(vctx.getOCExit());
	}

	template<> void EmitS<2>::emitht(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
	{
		IRBuilder<> builder(vctx.getLctx());
		builder.SetInsertPoint(vctx.getOCEntry());
		Value* xVal = vctx.getRegister( xarg);
		Value* yVal = vctx.getRegister( yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : vctx.getRegister( zarg);
		Value* theA = makeA(vctx.getLctx(), builder, yVal, zVal);
		Value* valToStore = createStoreCast(vctx.getLctx(), builder, builder.CreateLShr(xVal, builder.getInt64(32)), false);
		emitStoreMem(vctx.getLctx(), vctx.getModule(), vctx.getFunction(), builder, theA, adjustEndianness(vctx, builder, valToStore));
		builder.CreateBr(vctx.getOCExit());
	}

	template<> void EmitS<3>::emitco(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
	{
		IRBuilder<> builder(vctx.getLctx());
		builder.SetInsertPoint(vctx.getOCEntry());
		Value* xVal = builder.getInt64(xarg);
		Value* yVal = vctx.getRegister( yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : vctx.getRegister( zarg);
		Value* theA = makeA(vctx.getLctx(), builder, yVal, zVal);
		Value* valToStore = createStoreCast(vctx.getLctx(), builder, xVal, false);
		emitStoreMem(vctx.getLctx(), vctx.getModule(), vctx.getFunction(), builder, theA, adjustEndianness(vctx, builder, valToStore));
		builder.CreateBr(vctx.getOCExit());
	}

	template<> Value* EmitS<0>::makeA(LLVMContext& ctx, IRBuilder<>& builder, Value* yVal, Value* zVal)
	{
		return builder.CreateAdd(yVal, zVal);
	}

	template<int Pow2> Value* EmitS<Pow2>::makeA(LLVMContext& ctx, IRBuilder<>& builder, Value* yVal, Value* zVal)
	{
		return builder.CreateAnd(builder.CreateNot(builder.getInt64((1 << Pow2) - 1)), builder.CreateAdd(yVal, zVal));
	}

	template<> Value* EmitS<3>::adjustEndianness(VerticeContext& vctx, IRBuilder<>& builder, Value* val)
	{
		return emitAdjust64Endianness(vctx, builder, val);
	}

	template<> Value* EmitS<2>::adjustEndianness(VerticeContext& vctx, IRBuilder<>& builder, Value* val)
	{
		return emitAdjust32Endianness(vctx, builder, val);
	}

	template<> Value* EmitS<1>::adjustEndianness(VerticeContext& vctx, IRBuilder<>& builder, Value* val)
	{
		return emitAdjust16Endianness(vctx, builder, val);
	}

	template<> Value* EmitS<0>::adjustEndianness(VerticeContext& vctx, IRBuilder<>& builder, Value* val)
	{
		return val;
	}

	template<> Value* EmitS<3>::createStoreCast(LLVMContext& ctx, IRBuilder<>& builder, Value* val, int isSigned)
	{
		return val;
	}

	template<> Value* EmitS<2>::createStoreCast(LLVMContext& ctx, IRBuilder<>& builder, Value* val, int isSigned)
	{
		return builder.CreateIntCast(val, Type::getInt32Ty(ctx), isSigned);
	}

	template<> Value* EmitS<1>::createStoreCast(LLVMContext& ctx, IRBuilder<>& builder, Value* val, int isSigned)
	{
		return builder.CreateIntCast(val, Type::getInt16Ty(ctx), isSigned);
	}

	template<> Value* EmitS<0>::createStoreCast(LLVMContext& ctx, IRBuilder<>& builder, Value* val, int isSigned)
	{
		return builder.CreateIntCast(val, Type::getInt8Ty(ctx), isSigned);
	}
};

void MmixLlvm::Private::emitSto(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<3>::emit(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitStoi(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<3>::emit(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitStt(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<2>::emit(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitStti(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<2>::emit(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitStw(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<1>::emit(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitStwi(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<1>::emit(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitStb(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<0>::emit(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitStbi(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<0>::emit(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitSttu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<2>::emitu(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitSttui(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<2>::emitu(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitStwu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<1>::emitu(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitStwui(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<1>::emitu(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitStbu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<0>::emitu(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitStbui(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<0>::emitu(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitStht(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<2>::emitht(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitSthti(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<2>::emitht(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitStco(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<3>::emitco(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitStcoi(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitS<3>::emitco(vctx, xarg, yarg, zarg, true);
}
