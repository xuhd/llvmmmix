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
using MmixLlvm::Private::RegisterRecord;
using MmixLlvm::Private::RegistersMap;
using MmixLlvm::MXByte;

namespace {
	template<int Pow2> class EmitL
	{
		static Value* emitFetchMem(VerticeContext& vctx, IRBuilder<>& builder, Value* theA);
		static Value* makeA(VerticeContext& vctx, IRBuilder<>& builder, Value* yVal, Value* zVal);
		static Value* emitLoad(VerticeContext& vctx, IRBuilder<>& builder, Value* iref, bool isSigned);
	public:
		static void emit(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned, bool immediate);
		static void emitht(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
	};

	template<int Pow2> void EmitL<Pow2>::emit(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned, bool immediate)
	{
		LLVMContext& ctx = *vctx.Ctx;
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(vctx.Entry);
		Value* yVal = emitRegisterLoad(vctx, builder, yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : emitRegisterLoad(vctx, builder, zarg);
		Value* theA = makeA(vctx, builder, yVal, zVal);
		Value* readVal = emitFetchMem(vctx, builder, theA);
		Value* result = emitLoad(vctx, builder, readVal, isSigned);
		builder.CreateBr(vctx.Exit);
		addRegisterToCache(vctx, xarg, result, true);
	}

	template<> void EmitL<2>::emitht(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate) 
	{
		LLVMContext& ctx = *vctx.Ctx;
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(vctx.Entry);
		Value* yVal = emitRegisterLoad(vctx, builder, yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : emitRegisterLoad(vctx, builder, zarg);
		Value* theA = makeA(vctx, builder, yVal, zVal);
		Value* readVal = emitFetchMem(vctx, builder, theA);
		Value* result = builder.CreateShl(emitLoad(vctx, builder, readVal, false), builder.getInt64(32));
		builder.CreateBr(vctx.Exit);
		addRegisterToCache(vctx, xarg, result, true);
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
		return MmixLlvm::Private::emitFetchMem(*vctx.Ctx, *vctx.Module, *vctx.Function, builder, theA, Type::getInt64Ty(*vctx.Ctx));
	}

	template<> Value* EmitL<2>::emitFetchMem(VerticeContext& vctx,IRBuilder<>& builder, Value* theA)
	{
		return MmixLlvm::Private::emitFetchMem(*vctx.Ctx, *vctx.Module, *vctx.Function, builder, theA, Type::getInt32Ty(*vctx.Ctx));
	}

	template<> Value* EmitL<1>::emitFetchMem(VerticeContext& vctx, IRBuilder<>& builder, Value* theA)
	{
		return MmixLlvm::Private::emitFetchMem(*vctx.Ctx, *vctx.Module, *vctx.Function, builder, theA, Type::getInt16Ty(*vctx.Ctx));
	}

	template<> Value* EmitL<0>::emitFetchMem(VerticeContext& vctx, IRBuilder<>& builder, Value* theA)
	{
		return MmixLlvm::Private::emitFetchMem(*vctx.Ctx, *vctx.Module, *vctx.Function, builder, theA, Type::getInt8Ty(*vctx.Ctx));
	}

	template<> Value* EmitL<3>::emitLoad(VerticeContext& vctx, IRBuilder<>& builder, Value* readVal, bool isSigned)
	{
		return builder.CreateIntCast(emitAdjust64Endianness(vctx, builder, readVal), Type::getInt64Ty(*vctx.Ctx), isSigned);
	}

	template<> Value* EmitL<2>::emitLoad(VerticeContext& vctx, IRBuilder<>& builder, Value* readVal, bool isSigned)
	{
		return builder.CreateIntCast(emitAdjust32Endianness(vctx, builder, readVal), Type::getInt64Ty(*vctx.Ctx), isSigned);
	}

	template<> Value* EmitL<1>::emitLoad(VerticeContext& vctx, IRBuilder<>& builder, Value* readVal, bool isSigned)
	{
		return builder.CreateIntCast(emitAdjust16Endianness(vctx, builder, readVal), Type::getInt64Ty(*vctx.Ctx), isSigned);
	}

	template<> Value* EmitL<0>::emitLoad(VerticeContext& vctx, IRBuilder<>& builder, Value* readVal, bool isSigned)
	{
		return builder.CreateIntCast(readVal, Type::getInt64Ty(*vctx.Ctx), isSigned);
	}
};

void MmixLlvm::Private::emitLdo(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitL<3>::emit(vctx, xarg, yarg, zarg, false, false);
}

void MmixLlvm::Private::emitLdoi(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitL<3>::emit(vctx, xarg, yarg, zarg, false, true);
}

void MmixLlvm::Private::emitLdt(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned)
{
	EmitL<2>::emit(vctx, xarg, yarg, zarg, false, false);
}

void MmixLlvm::Private::emitLdti(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned)
{
	EmitL<2>::emit(vctx, xarg, yarg, zarg, false, true);
}

void MmixLlvm::Private::emitLdw(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned)
{
	EmitL<1>::emit(vctx, xarg, yarg, zarg, false, false);
}

void MmixLlvm::Private::emitLdwi(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned)
{
	EmitL<1>::emit(vctx, xarg, yarg, zarg, false, true);
}

void MmixLlvm::Private::emitLdb(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned)
{
	EmitL<0>::emit(vctx, xarg, yarg, zarg, false, false);
}

void MmixLlvm::Private::emitLdbi(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned)
{
	EmitL<0>::emit(vctx, xarg, yarg, zarg, false, true);
}

void MmixLlvm::Private::emitLdht(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitL<2>::emitht(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitLdhti(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	EmitL<2>::emitht(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitGet(VerticeContext& vctx, MXByte xarg, MXByte zarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* val = emitSpecialRegisterLoad(vctx, builder, (MmixLlvm::SpecialReg)zarg);
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, val, true);
}
		
void MmixLlvm::Private::emitPut(VerticeContext& vctx, MXByte xarg, MXByte zarg, bool immediate)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* val = immediate ? builder.getInt64(zarg) : emitRegisterLoad(vctx, builder, zarg);
	builder.CreateBr(vctx.Exit);
	addSpecialRegisterToCache(vctx, xarg, val, true);
}

void MmixLlvm::Private::emitGeta(VerticeContext& vctx, MXByte xarg, MXWyde yzarg, bool backward) {
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* m;
	if (!backward)
		m = builder.getInt64(vctx.XPtr + (yzarg << 2));
	else
		m = builder.getInt64(vctx.XPtr - (yzarg << 2));
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, m, true);
}