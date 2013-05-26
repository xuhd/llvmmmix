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
	template<int Pow2> class EmitS
	{
		const static int64_t LoBound = ~0i64 << ((1 << Pow2) * 8 - 1);
		const static int64_t HiBound = -1i64 - LoBound;
		static Value* makeA(LLVMContext& ctx, IRBuilder<>& builder, Value* yVal, Value* zVal);
		static Value* adjustEndianness(IRBuilder<>& builder, Value* val);
		static Value* createStoreCast(LLVMContext& ctx, IRBuilder<>& builder, Value* val, int isSigned);
	public:
		static void emit(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
		static void emitu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
		static void emitht(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
		static void emitco(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
	};

	void EmitS<3>::emit(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
	{
		emitu(vctx, xarg, yarg, zarg, immediate);
	}

	template<int Pow2> void EmitS<Pow2>::emit(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
	{
		LLVMContext& ctx = *vctx.Ctx;
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(vctx.Entry);
		RegistersMap& regMap = *vctx.RegMap;
		Value *registers = (*vctx.Module).getGlobalVariable("Registers");
		Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
		Value* loBoundCk = builder.CreateICmpSGE(xVal, builder.getInt64(LoBound));
		Value* hiBoundCk = builder.CreateICmpSLE(xVal, builder.getInt64(HiBound));
		BasicBlock *boundsChecked = BasicBlock::Create(ctx, genUniq("block"), vctx.Function);
		BasicBlock *boundsCheckFailed = BasicBlock::Create(ctx, genUniq("block"), vctx.Function);
		BasicBlock *exit0 = BasicBlock::Create(ctx, genUniq("block"), vctx.Function);
		builder.CreateCondBr(builder.CreateAnd(loBoundCk, hiBoundCk), boundsChecked, boundsCheckFailed);
		builder.SetInsertPoint(boundsChecked);
		Value* valToStore = createStoreCast(ctx, builder, xVal, true);
		Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : emitRegisterLoad(ctx, builder, registers, regMap, zarg);
		Value* theA = makeA(ctx, builder, yVal, zVal);
		emitStoreMem(ctx, *vctx.Module, *vctx.Function, builder, theA, adjustEndianness(builder, valToStore), exit0);
		builder.SetInsertPoint(boundsCheckFailed);
		Function* h = (*vctx.Module).getFunction("HandleOverflow");
		builder.CreateCall(h);
		builder.CreateBr(exit0);
		builder.SetInsertPoint(exit0);
		builder.CreateBr(vctx.Exit);
	}

	template<int Pow2> void EmitS<Pow2>::emitu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
	{
		LLVMContext& ctx = *vctx.Ctx;
		RegistersMap& regMap = *vctx.RegMap; 
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(vctx.Entry);
		Value *registers = (*vctx.Module).getGlobalVariable("Registers");
		Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
		Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : emitRegisterLoad(ctx, builder, registers, regMap, zarg);
		Value* theA = makeA(ctx, builder, yVal, zVal);
		Value* valToStore = createStoreCast(ctx, builder, xVal, false);
		BasicBlock *exit0 = BasicBlock::Create(ctx, genUniq("block"), vctx.Function);
		emitStoreMem(ctx, *vctx.Module, *vctx.Function, builder, theA, adjustEndianness(builder, valToStore), exit0);
		builder.SetInsertPoint(exit0);
		builder.CreateBr(vctx.Exit);
	}

	template<> void EmitS<2>::emitht(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
	{
		LLVMContext& ctx = *vctx.Ctx;
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(vctx.Entry);
		RegistersMap& regMap = *vctx.RegMap;
		Value *registers = (*vctx.Module).getGlobalVariable("Registers");
		Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
		Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : emitRegisterLoad(ctx, builder, registers, regMap, zarg);
		Value* theA = makeA(ctx, builder, yVal, zVal);
		Value* valToStore = createStoreCast(ctx, builder, builder.CreateLShr(xVal, builder.getInt64(32)), false);
		BasicBlock *exit0 = BasicBlock::Create(ctx, genUniq("block"), vctx.Function);
		emitStoreMem(ctx, *vctx.Module, *vctx.Function, builder, theA, adjustEndianness(builder, valToStore), exit0);
		builder.SetInsertPoint(exit0);
		builder.CreateBr(vctx.Exit);
	}

	template<> void EmitS<3>::emitco(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
	{
		LLVMContext& ctx = *vctx.Ctx;
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(vctx.Entry);
		Value *registers = (*vctx.Module).getGlobalVariable("Registers");
		RegistersMap& regMap = *vctx.RegMap;
		Value* xVal = builder.getInt64(xarg);
		Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : emitRegisterLoad(ctx, builder, registers, regMap, zarg);
		Value* theA = makeA(ctx, builder, yVal, zVal);
		Value* valToStore = createStoreCast(ctx, builder, xVal, false);
		BasicBlock *exit0 = BasicBlock::Create(ctx, genUniq("block"), vctx.Function);
		emitStoreMem(ctx, *vctx.Module, *vctx.Function, builder, theA, adjustEndianness(builder, valToStore), exit0);
		builder.SetInsertPoint(exit0);
		builder.CreateBr(vctx.Exit);
	}

	template<> Value* EmitS<0>::makeA(LLVMContext& ctx, IRBuilder<>& builder, Value* yVal, Value* zVal)
	{
		return builder.CreateAdd(yVal, zVal);
	}

	template<int Pow2> Value* EmitS<Pow2>::makeA(LLVMContext& ctx, IRBuilder<>& builder, Value* yVal, Value* zVal)
	{
		return builder.CreateAnd(builder.CreateNot(builder.getInt64((1 << Pow2) - 1)), builder.CreateAdd(yVal, zVal));
	}

	template<> Value* EmitS<3>::adjustEndianness(IRBuilder<>& builder, Value* val)
	{
		return emitAdjust64Endianness(builder, val);
	}

	template<> Value* EmitS<2>::adjustEndianness(IRBuilder<>& builder, Value* val)
	{
		return emitAdjust32Endianness(builder, val);
	}

	template<> Value* EmitS<1>::adjustEndianness(IRBuilder<>& builder, Value* val)
	{
		return emitAdjust16Endianness(builder, val);
	}

	template<> Value* EmitS<0>::adjustEndianness(IRBuilder<>& builder, Value* val)
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

void MmixLlvm::Private::emitSto(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<3>::emit(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitStoi(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<3>::emit(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitStt(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<2>::emit(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitStti(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<2>::emit(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitStw(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<1>::emit(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitStwi(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<1>::emit(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitStb(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<0>::emit(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitStbi(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<0>::emit(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitSttu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<2>::emitu(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitSttui(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<2>::emitu(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitStwu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<1>::emitu(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitStwui(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<1>::emitu(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitStbu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<0>::emitu(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitStbui(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<0>::emitu(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitStht(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<2>::emitht(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitSthti(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<2>::emitht(vctx, xarg, yarg, zarg, true);
}

void MmixLlvm::Private::emitStco(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<3>::emitco(vctx, xarg, yarg, zarg, false);
}

void MmixLlvm::Private::emitStcoi(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	EmitS<3>::emitco(vctx, xarg, yarg, zarg, true);
}
