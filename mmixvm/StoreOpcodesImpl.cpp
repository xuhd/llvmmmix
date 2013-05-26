#include "stdafx.h"
#include "Util.h"
#include "MmixEmit.h"
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
using MmixLlvm::EdgeList;
using MmixLlvm::MemAccessor;
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
		static BasicBlock* emit(LLVMContext& ctx, Module& m, Function& f, BasicBlock* entry,
			RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
		static BasicBlock* emitu(LLVMContext& ctx, Module& m, Function& f, BasicBlock* entry,
			RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
		static BasicBlock* emitht(LLVMContext& ctx, Module& m, Function& f, BasicBlock* entry,
			RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
	};

	BasicBlock* EmitS<3>::emit(LLVMContext& ctx, Module& m, Function& f, BasicBlock* entry,
		RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
	{
		return emitu(ctx, m, f, entry, regMap, xarg, yarg, zarg, immediate);
	}

	template<int Pow2> BasicBlock* EmitS<Pow2>::emit(LLVMContext& ctx, Module& m, Function& f, BasicBlock* entry,
			RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
	{
		BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
		BasicBlock *boundsChecked = BasicBlock::Create(ctx, genUniq("block"), &f);
		BasicBlock *boundsCheckFailed = BasicBlock::Create(ctx, genUniq("block"), &f);
		BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(entryBlock);
		Value *registers = m.getGlobalVariable("Registers");
		Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
		Value* loBoundCk = builder.CreateICmpSGE(xVal, builder.getInt64(LoBound));
		Value* hiBoundCk = builder.CreateICmpSLE(xVal, builder.getInt64(HiBound));
		builder.CreateCondBr(builder.CreateAnd(loBoundCk, hiBoundCk), boundsChecked, boundsCheckFailed);
		builder.SetInsertPoint(boundsChecked);
		Value* valToStore = createStoreCast(ctx, builder, xVal, true);
		Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : emitRegisterLoad(ctx, builder, registers, regMap, zarg);
		Value* theA = makeA(ctx, builder, yVal, zVal);
		emitStoreMem(ctx, m, f, builder, theA, adjustEndianness(builder, valToStore), epilogue);
		builder.SetInsertPoint(boundsCheckFailed);
		Function* h = m.getFunction("HandleOverflow");
		builder.CreateCall(h);
		builder.CreateBr(epilogue);
		builder.SetInsertPoint(epilogue);
		return epilogue;
	}

	template<int Pow2> BasicBlock* EmitS<Pow2>::emitu(LLVMContext& ctx, Module& m, Function& f, BasicBlock* entry,
			RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
	{
		BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
		BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(entryBlock);
		Value *registers = m.getGlobalVariable("Registers");
		Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
		Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : emitRegisterLoad(ctx, builder, registers, regMap, zarg);
		Value* theA = makeA(ctx, builder, yVal, zVal);
		Value* valToStore = createStoreCast(ctx, builder, xVal, false);
		emitStoreMem(ctx, m, f, builder, theA, adjustEndianness(builder, valToStore), epilogue);
		builder.SetInsertPoint(epilogue);
		return epilogue;
	}

	template<> BasicBlock* EmitS<2>::emitht(LLVMContext& ctx, Module& m, Function& f, BasicBlock* entry,
		RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate)
	{
		BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
		BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(entryBlock);
		Value *registers = m.getGlobalVariable("Registers");
		Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
		Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
		Value* zVal = immediate ? builder.getInt64(zarg) : emitRegisterLoad(ctx, builder, registers, regMap, zarg);
		Value* theA = makeA(ctx, builder, yVal, zVal);
		Value* valToStore = createStoreCast(ctx, builder, builder.CreateLShr(xVal, builder.getInt64(32)), false);
		emitStoreMem(ctx, m, f, builder, theA, adjustEndianness(builder, valToStore), epilogue);
		builder.SetInsertPoint(epilogue);
		return epilogue;
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

BasicBlock* MmixLlvm::Private::emitSto(LLVMContext& ctx, Module& m, Function& f, BasicBlock* entry,
	RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<3>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, false);
}

BasicBlock* MmixLlvm::Private::emitStoi(LLVMContext& ctx, Module& m, Function& f, BasicBlock* entry,
	RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<3>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, true);
}

llvm::BasicBlock* MmixLlvm::Private::emitStt(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
	llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<2>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, false);
}

llvm::BasicBlock* MmixLlvm::Private::emitStti(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
	llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<2>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, true);
}

llvm::BasicBlock* MmixLlvm::Private::emitStw(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
	llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<1>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, false);
}

llvm::BasicBlock* MmixLlvm::Private::emitStwi(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
	llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<1>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, true);
}

llvm::BasicBlock* MmixLlvm::Private::emitStb(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
	llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<0>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, false);
}

llvm::BasicBlock* MmixLlvm::Private::emitStbi(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
	llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<0>::emit(ctx, m, f, entry, regMap, xarg, yarg, zarg, true);
}

BasicBlock* MmixLlvm::Private::emitSttu(LLVMContext& ctx, llvm::Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<2>::emitu(ctx, m, f, entry, regMap, xarg, yarg, zarg, false);
}

BasicBlock* MmixLlvm::Private::emitSttui(LLVMContext& ctx, llvm::Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<2>::emitu(ctx, m, f, entry, regMap, xarg, yarg, zarg, true);
}

BasicBlock* MmixLlvm::Private::emitStwu(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<1>::emitu(ctx, m, f, entry, regMap, xarg, yarg, zarg, false);
}

BasicBlock* MmixLlvm::Private::emitStwui(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<1>::emitu(ctx, m, f, entry, regMap, xarg, yarg, zarg, true);
}

BasicBlock* MmixLlvm::Private::emitStbu(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<0>::emitu(ctx, m, f, entry, regMap, xarg, yarg, zarg, false);
}

BasicBlock* MmixLlvm::Private::emitStbui(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<0>::emitu(ctx, m, f, entry, regMap, xarg, yarg, zarg, true);
}

BasicBlock* MmixLlvm::Private::emitStht(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<2>::emitht(ctx, m, f, entry, regMap, xarg, yarg, zarg, false);
}

BasicBlock* MmixLlvm::Private::emitSthti(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	return EmitS<2>::emitht(ctx, m, f, entry, regMap, xarg, yarg, zarg, true);
}
