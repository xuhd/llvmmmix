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

BasicBlock* MmixLlvm::Private::emitLdo(LLVMContext& ctx, llvm::Module& m, llvm::Function& f, RegistersMap& regMap,
	uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = emitRegisterLoad(ctx, builder, registers, regMap, zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x7)), builder.CreateAdd(yVal, zVal));
	Value* iref = emitFetchMem(ctx, m, f, builder, theA, Type::getInt64Ty(ctx), epilogue);
	builder.SetInsertPoint(epilogue);
	Value* result = emitAdjust64Endianness(builder, builder.CreateLoad(iref));
	RegisterRecord r0;
	r0.value = result;
	r0.changed = true;
	regMap[xarg] = r0;
	return epilogue;
}

BasicBlock* MmixLlvm::Private::emitLdt(LLVMContext& ctx, llvm::Module& m, llvm::Function& f, RegistersMap& regMap,
	uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	BasicBlock *entryBlock = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = emitRegisterLoad(ctx, builder, registers, regMap, zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x3)), builder.CreateAdd(yVal, zVal));
	Value* iref = emitFetchMem(ctx, m, f, builder, theA, Type::getInt32Ty(ctx), epilogue);
	builder.SetInsertPoint(epilogue);
	Value* result = builder.CreateIntCast(emitAdjust32Endianness(builder, builder.CreateLoad(iref)), Type::getInt64Ty(ctx), isSigned);
	RegisterRecord r0;
	r0.value = result;
	r0.changed = true;
	regMap[xarg] = r0;
	return epilogue;
}

BasicBlock* MmixLlvm::Private::emitLdw(LLVMContext& ctx, llvm::Module& m, llvm::Function& f, RegistersMap& regMap,
	uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	BasicBlock *entryBlock = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = emitRegisterLoad(ctx, builder, registers, regMap, zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x1)), builder.CreateAdd(yVal, zVal));
	Value* iref = emitFetchMem(ctx, m, f, builder, theA, Type::getInt16Ty(ctx), epilogue);
	builder.SetInsertPoint(epilogue);
	Value* result = builder.CreateIntCast(emitAdjust16Endianness(builder, builder.CreateLoad(iref)), Type::getInt64Ty(ctx), isSigned);
	RegisterRecord r0;
	r0.value = result;
	r0.changed = true;
	regMap[xarg] = r0;
	return epilogue;
}

BasicBlock* MmixLlvm::Private::emitLdb(LLVMContext& ctx, llvm::Module& m, llvm::Function& f, RegistersMap& regMap,
	uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	BasicBlock *entryBlock = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = emitRegisterLoad(ctx, builder, registers, regMap, zarg);
	Value* theA = builder.CreateAdd(yVal, zVal);
	Value* iref = emitFetchMem(ctx, m, f, builder, theA, Type::getInt8Ty(ctx), epilogue);
	builder.SetInsertPoint(epilogue);
	Value* result = builder.CreateIntCast(builder.CreateLoad(iref), Type::getInt64Ty(ctx), isSigned);
	RegisterRecord r0;
	r0.value = result;
	r0.changed = true;
	regMap[xarg] = r0;
	return epilogue;
}

BasicBlock* MmixLlvm::Private::emitLdoi(LLVMContext& ctx, llvm::Module& m, llvm::Function& f, RegistersMap& regMap,
	uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = builder.getInt64(zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x7)), builder.CreateAdd(yVal, zVal));
	Value* iref = emitFetchMem(ctx, m, f, builder, theA, Type::getInt64Ty(ctx), epilogue);
	builder.SetInsertPoint(epilogue);
	Value* result = emitAdjust64Endianness(builder, builder.CreateLoad(iref));
	RegisterRecord r0;
	r0.value = result;
	r0.changed = true;
	regMap[xarg] = r0;
	return epilogue;
}

BasicBlock* MmixLlvm::Private::emitLdti(LLVMContext& ctx, llvm::Module& m, llvm::Function& f, RegistersMap& regMap,
	uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	BasicBlock *entryBlock = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = builder.getInt64(zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x3)), builder.CreateAdd(yVal, zVal));
	Value* iref = emitFetchMem(ctx, m, f, builder, theA, Type::getInt32Ty(ctx), epilogue);
	builder.SetInsertPoint(epilogue);
	Value* result = builder.CreateIntCast(emitAdjust32Endianness(builder, builder.CreateLoad(iref)), Type::getInt64Ty(ctx), isSigned);
	RegisterRecord r0;
	r0.value = result;
	r0.changed = true;
	regMap[xarg] = r0;
	return epilogue;
}

BasicBlock* MmixLlvm::Private::emitLdwi(LLVMContext& ctx, llvm::Module& m, llvm::Function& f, RegistersMap& regMap,
	uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	BasicBlock *entryBlock = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = builder.getInt64(zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x1)), builder.CreateAdd(yVal, zVal));
	Value* iref = emitFetchMem(ctx, m, f, builder, theA, Type::getInt16Ty(ctx), epilogue);
	builder.SetInsertPoint(epilogue);
	Value* result = builder.CreateIntCast(emitAdjust16Endianness(builder, builder.CreateLoad(iref)), Type::getInt64Ty(ctx), isSigned);
	RegisterRecord r0;
	r0.value = result;
	r0.changed = true;
	regMap[xarg] = r0;
	return epilogue;
}

BasicBlock* MmixLlvm::Private::emitLdbi(LLVMContext& ctx, llvm::Module& m, llvm::Function& f, RegistersMap& regMap,
	uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned)
{
	BasicBlock *entryBlock = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = builder.getInt64(zarg);
	Value* theA = builder.CreateAdd(yVal, zVal);
	Value* iref = emitFetchMem(ctx, m, f, builder, theA, Type::getInt8Ty(ctx), epilogue);
	builder.SetInsertPoint(epilogue);
	Value* result = builder.CreateIntCast(builder.CreateLoad(iref), Type::getInt64Ty(ctx), isSigned);
	RegisterRecord r0;
	r0.value = result;
	r0.changed = true;
	regMap[xarg] = r0;
	return epilogue;
}

llvm::BasicBlock* MmixLlvm::Private::emitLdht(LLVMContext& ctx, llvm::Module& m, llvm::Function& f, RegistersMap& regMap,
			uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = emitRegisterLoad(ctx, builder, registers, regMap, zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x3)), builder.CreateAdd(yVal, zVal));
	Value* iref = emitFetchMem(ctx, m, f, builder, theA, Type::getInt32Ty(ctx), epilogue);
	builder.SetInsertPoint(epilogue);
	Value* result = builder.CreateIntCast(emitAdjust32Endianness(builder, builder.CreateLoad(iref)), Type::getInt64Ty(ctx), false);
	result = builder.CreateShl(result, builder.getInt64(32));
	RegisterRecord r0;
	r0.value = result;
	r0.changed = true;
	regMap[xarg] = r0;
	return epilogue;
}

llvm::BasicBlock* MmixLlvm::Private::emitLdhti(LLVMContext& ctx, llvm::Module& m, llvm::Function& f, RegistersMap& regMap,
			uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = builder.getInt64(zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x3)), builder.CreateAdd(yVal, zVal));
	Value* iref = emitFetchMem(ctx, m, f, builder, theA, Type::getInt32Ty(ctx), epilogue);
	builder.SetInsertPoint(epilogue);
	Value* result = builder.CreateIntCast(emitAdjust32Endianness(builder, builder.CreateLoad(iref)), Type::getInt64Ty(ctx), false);
	result = builder.CreateShl(result, builder.getInt64(32));
	RegisterRecord r0;
	r0.value = result;
	r0.changed = true;
	regMap[xarg] = r0;
	return epilogue;
}

