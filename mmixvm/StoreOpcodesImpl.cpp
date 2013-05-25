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

BasicBlock* MmixLlvm::Private::emitSto(LLVMContext& ctx, Module& m, Function& f, BasicBlock* entry,
	RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = emitRegisterLoad(ctx, builder, registers, regMap, zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x7)), builder.CreateAdd(yVal, zVal));
	emitStoreMem(ctx, m, f, builder, theA, emitAdjust64Endianness(builder, xVal), epilogue);
	builder.SetInsertPoint(epilogue);
	return epilogue;
}

BasicBlock* MmixLlvm::Private::emitStoi(LLVMContext& ctx, Module& m, Function& f, BasicBlock* entry,
	RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = builder.getInt64(zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x7)), builder.CreateAdd(yVal, zVal));
	emitStoreMem(ctx, m, f, builder, theA, emitAdjust64Endianness(builder, xVal), epilogue);
	builder.SetInsertPoint(epilogue);
	return epilogue;
}