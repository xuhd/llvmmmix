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
		static BasicBlock* emit(LLVMContext& ctx, Module& m, Function& f, 
			BasicBlock* entry, RegistersMap& regMap, 
			uint8_t xarg, uint8_t yarg, uint8_t zarg,
			bool isSigned, bool immediate);
		static BasicBlock* emitht(LLVMContext& ctx, Module& m, Function& f, 
			BasicBlock* entry, RegistersMap& regMap, 
			uint8_t xarg, uint8_t yarg, uint8_t zarg,
			bool immediate);
	};

	template<int Pow2> 
		BasicBlock* EmitL<Pow2>::emit(LLVMContext& ctx, Module& m, Function& f, 
			BasicBlock* entry, RegistersMap& regMap, 
			uint8_t xarg, uint8_t yarg, uint8_t zarg,
			bool isSigned, bool immediate)
	{
		BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
		BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(entryBlock);
		Value *registers = m.getGlobalVariable("Registers");
		Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : emitRegisterLoad(ctx, builder, registers, regMap, zarg);
		Value* theA = makeA(ctx, builder, yVal, zVal);
		Value* iref = emitFetchMem(ctx, m, f, builder, theA, epilogue);
		builder.SetInsertPoint(epilogue);
		Value* result = emitLoad(ctx, builder, iref, isSigned);
		RegisterRecord r0;
		r0.value = result;
		r0.changed = true;
		regMap[xarg] = r0;
		return epilogue;
	}

	template<> 
		BasicBlock* EmitL<2>::emitht(LLVMContext& ctx, Module& m, Function& f, 
			BasicBlock* entry, RegistersMap& regMap, 
			uint8_t xarg, uint8_t yarg, uint8_t zarg,
			bool immediate)
	{
		BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
		BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(entryBlock);
		Value *registers = m.getGlobalVariable("Registers");
		Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : emitRegisterLoad(ctx, builder, registers, regMap, zarg);
		Value* theA = makeA(ctx, builder, yVal, zVal);
		Value* iref = emitFetchMem(ctx, m, f, builder, theA, epilogue);
		builder.SetInsertPoint(epilogue);
		Value* result = builder.CreateShl(emitLoad(ctx, builder, iref, false), builder.getInt64(32));
		RegisterRecord r0;
		r0.value = result;
		r0.changed = true;
		regMap[xarg] = r0;
		return epilogue;
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

BasicBlock* MmixLlvm::Private::emitLdo(LLVMContext& ctx, Module& m, Function& f, 
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitL<3>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, false, false);
}

BasicBlock* MmixLlvm::Private::emitLdoi(LLVMContext& ctx, Module& m, Function& f, 
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitL<3>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, false, true);
}

BasicBlock* MmixLlvm::Private::emitLdt(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	return EmitL<2>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, false, false);
}

BasicBlock* MmixLlvm::Private::emitLdti(LLVMContext& ctx, Module& m, Function& f, 
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	return EmitL<2>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, false, true);
}

BasicBlock* MmixLlvm::Private::emitLdw(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	return EmitL<1>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, false, false);
}

BasicBlock* MmixLlvm::Private::emitLdwi(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	return EmitL<1>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, false, true);
}

BasicBlock* MmixLlvm::Private::emitLdb(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	return EmitL<0>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, false, false);
}

BasicBlock* MmixLlvm::Private::emitLdbi(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	return EmitL<0>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, false, true);
}

BasicBlock* MmixLlvm::Private::emitLdht(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitL<2>::emitht(ctx, m, f, entry, regMap, xarg, yarg, zarg, false);
}

BasicBlock* MmixLlvm::Private::emitLdhti(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitL<2>::emitht(ctx, m, f, entry, regMap, xarg, yarg, zarg, true);
}
