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
using MmixLlvm::MXByte;

namespace {
	template<int Pow2> class EmitL
	{
		static Value* emitFetchMem(VerticeContext& vctx, IRBuilder<>& builder, Value* theA);
		static Value* makeA(VerticeContext& vctx, IRBuilder<>& builder, Value* yVal, Value* zVal);
		static Value* emitLoad(VerticeContext& vctx, IRBuilder<>& builder, Value* iref, bool isSigned);
	public:
		static void emit(VerticeContext& vctx, IRBuilder<>& builder, 
			MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned, bool immediate);
		static void emitht(VerticeContext& vctx, IRBuilder<>& builder,
			MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
	};

	template<int Pow2> void EmitL<Pow2>::emit(VerticeContext& vctx, IRBuilder<>& builder, 
		MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned, bool immediate)
	{
		Value* yVal = vctx.getRegister(yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : vctx.getRegister( zarg);
		Value* theA = makeA(vctx, builder, yVal, zVal);
		Value* readVal = emitFetchMem(vctx, builder, theA);
		Value* result = emitLoad(vctx, builder, readVal, isSigned);
		assignRegister(vctx, builder, xarg, result);
		builder.CreateBr(vctx.getOCExit());
	}

	template<> void EmitL<2>::emitht(VerticeContext& vctx, IRBuilder<>& builder,
		MXByte xarg, MXByte yarg, MXByte zarg, bool immediate) 
	{
		Value* yVal = vctx.getRegister(yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : vctx.getRegister( zarg);
		Value* theA = makeA(vctx, builder, yVal, zVal);
		Value* readVal = emitFetchMem(vctx, builder, theA);
		Value* result = builder.CreateShl(emitLoad(vctx, builder, readVal, false), builder.getInt64(32));
		assignRegister(vctx, builder, xarg, result);
		builder.CreateBr(vctx.getOCExit());
	}

	template<> Value* EmitL<0>::makeA(VerticeContext& vctx, IRBuilder<>& builder, Value* yVal, Value* zVal)
	{
		return builder.CreateAdd(yVal, zVal);
	}

	template<int Pow2> Value* EmitL<Pow2>::makeA(VerticeContext& vctx, IRBuilder<>& builder, Value* yVal, Value* zVal)
	{
		return builder.CreateAnd(builder.CreateNot(builder.getInt64((1 << Pow2) - 1)), builder.CreateAdd(yVal, zVal));
	}

	template<> Value* EmitL<3>::emitFetchMem(VerticeContext& vctx, IRBuilder<>& builder, Value* theA)
	{
		return MmixLlvm::Private::emitFetchMem(vctx, builder, theA, Type::getInt64Ty(vctx.getLctx()));
	}

	template<> Value* EmitL<2>::emitFetchMem(VerticeContext& vctx, IRBuilder<>& builder, Value* theA)
	{
		return MmixLlvm::Private::emitFetchMem(vctx, builder, theA, Type::getInt32Ty(vctx.getLctx()));
	}

	template<> Value* EmitL<1>::emitFetchMem(VerticeContext& vctx, IRBuilder<>& builder, Value* theA)
	{
		return MmixLlvm::Private::emitFetchMem(vctx, builder, theA, Type::getInt16Ty(vctx.getLctx()));
	}

	template<> Value* EmitL<0>::emitFetchMem(VerticeContext& vctx, IRBuilder<>& builder, Value* theA)
	{
		return MmixLlvm::Private::emitFetchMem(vctx, builder, theA, Type::getInt8Ty(vctx.getLctx()));
	}

	template<> Value* EmitL<3>::emitLoad(VerticeContext& vctx, IRBuilder<>& builder, Value* readVal, bool isSigned)
	{
		return builder.CreateIntCast(emitAdjust64Endianness(vctx, builder, readVal), Type::getInt64Ty(vctx.getLctx()), isSigned);
	}

	template<> Value* EmitL<2>::emitLoad(VerticeContext& vctx, IRBuilder<>& builder, Value* readVal, bool isSigned)
	{
		return builder.CreateIntCast(emitAdjust32Endianness(vctx, builder, readVal), Type::getInt64Ty(vctx.getLctx()), isSigned);
	}

	template<> Value* EmitL<1>::emitLoad(VerticeContext& vctx, IRBuilder<>& builder, Value* readVal, bool isSigned)
	{
		return builder.CreateIntCast(emitAdjust16Endianness(vctx, builder, readVal), Type::getInt64Ty(vctx.getLctx()), isSigned);
	}

	template<> Value* EmitL<0>::emitLoad(VerticeContext& vctx, IRBuilder<>& builder, Value* readVal, bool isSigned)
	{
		return builder.CreateIntCast(readVal, Type::getInt64Ty(vctx.getLctx()), isSigned);
	}
};

void MmixLlvm::Private::emitLdo(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitL<3>::emit(vctx, builder, xarg, yarg, zarg, false, false);
}

void MmixLlvm::Private::emitLdoi(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitL<3>::emit(vctx, builder, xarg, yarg, zarg, false, true);
}

void MmixLlvm::Private::emitLdt(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned)
{
	EmitL<2>::emit(vctx, builder, xarg, yarg, zarg, false, false);
}

void MmixLlvm::Private::emitLdti(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned)
{
	EmitL<2>::emit(vctx, builder, xarg, yarg, zarg, false, true);
}

void MmixLlvm::Private::emitLdw(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned)
{
	EmitL<1>::emit(vctx, builder, xarg, yarg, zarg, false, false);
}

void MmixLlvm::Private::emitLdwi(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned)
{
	EmitL<1>::emit(vctx, builder, xarg, yarg, zarg, false, true);
}

void MmixLlvm::Private::emitLdb(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned)
{
	EmitL<0>::emit(vctx, builder, xarg, yarg, zarg, false, false);
}

void MmixLlvm::Private::emitLdbi(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned)
{
	EmitL<0>::emit(vctx, builder, xarg, yarg, zarg, false, true);
}

void MmixLlvm::Private::emitLdht(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitL<2>::emitht(vctx, builder, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitLdhti(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitL<2>::emitht(vctx, builder, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitGet(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte zarg)
{
	Value* val = vctx.getSpRegister((MmixLlvm::SpecialReg)zarg);
	assignRegister(vctx, builder, xarg, val);
	builder.CreateBr(vctx.getOCExit());
}
		
void MmixLlvm::Private::emitPut(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXByte zarg, bool immediate)
{
	Value* val = immediate ? builder.getInt64(zarg) : vctx.getRegister(zarg);
	vctx.assignSpRegister((SpecialReg)xarg, val);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitGeta(VerticeContext& vctx, IRBuilder<>& builder,
	MXByte xarg, MXWyde yzarg, bool backward)
{
	Value* m;
	if (!backward)
		m = builder.getInt64(vctx.getXPtr() + (yzarg << 2));
	else
		m = builder.getInt64(vctx.getXPtr() - (yzarg << 2));
	assignRegister(vctx, builder, xarg, m);
	builder.CreateBr(vctx.getOCExit());
}