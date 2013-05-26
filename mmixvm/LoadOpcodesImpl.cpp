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

namespace {
	template<int Pow2> class EmitL
	{
		static Value* emitFetchMem(LLVMContext& ctx, Module& m, Function& f,
			IRBuilder<>& builder, Value* theA, BasicBlock* exit);
		static Value* makeA(LLVMContext& ctx, IRBuilder<>& builder, Value* yVal, Value* zVal);
		static Value* emitLoad(LLVMContext& ctx, IRBuilder<>& builder, Value* iref, bool isSigned);
	public:
		static void emit(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned, bool immediate);
		static void emitht(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
	};

	template<int Pow2> void EmitL<Pow2>::emit(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned, bool immediate)
	{
		LLVMContext& ctx = *vctx.Ctx;
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(vctx.Entry);
		RegistersMap& regMap = *vctx.RegMap;
		Value *registers = (*vctx.Module).getGlobalVariable("Registers");
		Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : emitRegisterLoad(ctx, builder, registers, regMap, zarg);
		Value* theA = makeA(ctx, builder, yVal, zVal);
		BasicBlock* fetchExit = BasicBlock::Create(ctx, genUniq("fetchExit"), vctx.Function);
		Value* iref = emitFetchMem(ctx, *vctx.Module, *vctx.Function, builder, theA, fetchExit);
		builder.SetInsertPoint(fetchExit);
		Value* result = emitLoad(ctx, builder, iref, isSigned);
		builder.CreateBr(vctx.Exit);
		RegisterRecord r0;
		r0.value = result;
		r0.changed = true;
		regMap[xarg] = r0;
	}

	template<> void EmitL<2>::emitht(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate) 
	{
		LLVMContext& ctx = *vctx.Ctx;
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(vctx.Entry);
		RegistersMap& regMap = *vctx.RegMap;
		Value *registers = (*vctx.Module).getGlobalVariable("Registers");
		Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : emitRegisterLoad(ctx, builder, registers, regMap, zarg);
		Value* theA = makeA(ctx, builder, yVal, zVal);
		BasicBlock* fetchExit = BasicBlock::Create(ctx, genUniq("fetchExit"), vctx.Function);
		Value* iref = emitFetchMem(ctx, *vctx.Module, *vctx.Function, builder, theA, fetchExit);
		builder.SetInsertPoint(fetchExit);
		Value* result = builder.CreateShl(emitLoad(ctx, builder, iref, false), builder.getInt64(32));
		builder.CreateBr(vctx.Exit);
		RegisterRecord r0;
		r0.value = result;
		r0.changed = true;
		regMap[xarg] = r0;
	}

	template<> Value* EmitL<0>::makeA(LLVMContext& ctx, IRBuilder<>& builder, Value* yVal, Value* zVal)
	{
		return builder.CreateAdd(yVal, zVal);
	}

	template<int Pow2> Value* EmitL<Pow2>::makeA(LLVMContext& ctx, IRBuilder<>& builder, Value* yVal, Value* zVal)
	{
		return builder.CreateAnd(builder.CreateNot(builder.getInt64((1 << Pow2) - 1)), builder.CreateAdd(yVal, zVal));
	}

	template<> Value* EmitL<3>::emitFetchMem(LLVMContext& ctx, Module& m, Function& f,
			IRBuilder<>& builder, Value* theA, BasicBlock* exit)
	{
		return MmixLlvm::Private::emitFetchMem(ctx, m, f, builder, theA, Type::getInt64Ty(ctx), exit);
	}

	template<> Value* EmitL<2>::emitFetchMem(LLVMContext& ctx, Module& m, Function& f,
			IRBuilder<>& builder, Value* theA, BasicBlock* exit)
	{
		return MmixLlvm::Private::emitFetchMem(ctx, m, f, builder, theA, Type::getInt32Ty(ctx), exit);
	}

	template<> Value* EmitL<1>::emitFetchMem(LLVMContext& ctx, Module& m, Function& f,
			IRBuilder<>& builder, Value* theA, BasicBlock* exit)
	{
		return MmixLlvm::Private::emitFetchMem(ctx, m, f, builder, theA, Type::getInt16Ty(ctx), exit);
	}

	template<> Value* EmitL<0>::emitFetchMem(LLVMContext& ctx, Module& m, Function& f,
			IRBuilder<>& builder, Value* theA, BasicBlock* exit)
	{
		return MmixLlvm::Private::emitFetchMem(ctx, m, f, builder, theA, Type::getInt8Ty(ctx), exit);
	}

	template<> Value* EmitL<3>::emitLoad(LLVMContext& ctx, IRBuilder<>& builder, Value* iref, bool isSigned)
	{
		return builder.CreateIntCast(emitAdjust64Endianness(builder, builder.CreateLoad(iref)), Type::getInt64Ty(ctx), isSigned);
	}

	template<> Value* EmitL<2>::emitLoad(LLVMContext& ctx, IRBuilder<>& builder, Value* iref, bool isSigned)
	{
		return builder.CreateIntCast(emitAdjust32Endianness(builder, builder.CreateLoad(iref)), Type::getInt64Ty(ctx), isSigned);
	}

	template<> Value* EmitL<1>::emitLoad(LLVMContext& ctx, IRBuilder<>& builder, Value* iref, bool isSigned)
	{
		return builder.CreateIntCast(emitAdjust16Endianness(builder, builder.CreateLoad(iref)), Type::getInt64Ty(ctx), isSigned);
	}

	template<> Value* EmitL<0>::emitLoad(LLVMContext& ctx, IRBuilder<>& builder, Value* iref, bool isSigned)
	{
		return builder.CreateIntCast(builder.CreateLoad(iref), Type::getInt64Ty(ctx), isSigned);
	}
};

void MmixLlvm::Private::emitLdo(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitL<3>::emit(vctx, xarg, yarg, zarg, false, false);
}

void MmixLlvm::Private::emitLdoi(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitL<3>::emit(vctx, xarg, yarg, zarg, false, true);
}

void MmixLlvm::Private::emitLdt(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	EmitL<2>::emit(vctx, xarg, yarg, zarg, false, false);
}

void MmixLlvm::Private::emitLdti(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	EmitL<2>::emit(vctx, xarg, yarg, zarg, false, true);
}

void MmixLlvm::Private::emitLdw(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	EmitL<1>::emit(vctx, xarg, yarg, zarg, false, false);
}

void MmixLlvm::Private::emitLdwi(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	EmitL<1>::emit(vctx, xarg, yarg, zarg, false, true);
}

void MmixLlvm::Private::emitLdb(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	EmitL<0>::emit(vctx, xarg, yarg, zarg, false, false);
}

void MmixLlvm::Private::emitLdbi(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	EmitL<0>::emit(vctx, xarg, yarg, zarg, false, true);
}

void MmixLlvm::Private::emitLdht(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitL<2>::emitht(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitLdhti(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitL<2>::emitht(vctx, xarg, yarg, zarg, true);
}
