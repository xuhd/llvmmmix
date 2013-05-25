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

llvm::BasicBlock* MmixLlvm::Private::emitStt(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
	llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *boundsChecked = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *boundsCheckFailed = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
	Value* loBoundCk = builder.CreateICmpSGE(xVal, builder.getInt64(INT_MIN));
	Value* hiBoundCk = builder.CreateICmpSLE(xVal, builder.getInt64(INT_MAX));
	builder.CreateCondBr(builder.CreateAnd(loBoundCk, hiBoundCk), boundsChecked, boundsCheckFailed);
	builder.SetInsertPoint(boundsChecked);
	Value* valToStore = builder.CreateIntCast(xVal, Type::getInt32Ty(ctx), true);
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = emitRegisterLoad(ctx, builder, registers, regMap, zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x3)), builder.CreateAdd(yVal, zVal));
	emitStoreMem(ctx, m, f, builder, theA, emitAdjust32Endianness(builder, valToStore), epilogue);
	builder.SetInsertPoint(boundsCheckFailed);
	builder.CreateBr(epilogue); // ToDo : handle overflow
	builder.SetInsertPoint(epilogue);
	return epilogue;
}

llvm::BasicBlock* MmixLlvm::Private::emitStti(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
	llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *boundsChecked = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *boundsCheckFailed = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
	Value* loBoundCk = builder.CreateICmpSGE(xVal, builder.getInt64(INT_MIN));
	Value* hiBoundCk = builder.CreateICmpSLE(xVal, builder.getInt64(INT_MAX));
	builder.CreateCondBr(builder.CreateAnd(loBoundCk, hiBoundCk), boundsChecked, boundsCheckFailed);
	builder.SetInsertPoint(boundsChecked);
	Value* valToStore = builder.CreateIntCast(xVal, Type::getInt32Ty(ctx), true);
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = builder.getInt64(zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x3)), builder.CreateAdd(yVal, zVal));
	emitStoreMem(ctx, m, f, builder, theA, emitAdjust32Endianness(builder, valToStore), epilogue);
	builder.SetInsertPoint(boundsCheckFailed);
	builder.CreateBr(epilogue); // ToDo : handle overflow
	builder.SetInsertPoint(epilogue);
	return epilogue;
}

llvm::BasicBlock* MmixLlvm::Private::emitStw(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
	llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *boundsChecked = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *boundsCheckFailed = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
	Value* loBoundCk = builder.CreateICmpSGE(xVal, builder.getInt64(SHRT_MIN));
	Value* hiBoundCk = builder.CreateICmpSLE(xVal, builder.getInt64(SHRT_MAX));
	builder.CreateCondBr(builder.CreateAnd(loBoundCk, hiBoundCk), boundsChecked, boundsCheckFailed);
	builder.SetInsertPoint(boundsChecked);
	Value* valToStore = builder.CreateIntCast(xVal, Type::getInt16Ty(ctx), true);
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = emitRegisterLoad(ctx, builder, registers, regMap, zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x1)), builder.CreateAdd(yVal, zVal));
	emitStoreMem(ctx, m, f, builder, theA, emitAdjust16Endianness(builder, valToStore), epilogue);
	builder.SetInsertPoint(boundsCheckFailed);
	builder.CreateBr(epilogue); // ToDo : handle overflow
	builder.SetInsertPoint(epilogue);
	return epilogue;
}

llvm::BasicBlock* MmixLlvm::Private::emitStwi(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
	llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *boundsChecked = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *boundsCheckFailed = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
	Value* loBoundCk = builder.CreateICmpSGE(xVal, builder.getInt64(SHRT_MIN));
	Value* hiBoundCk = builder.CreateICmpSLE(xVal, builder.getInt64(SHRT_MAX));
	builder.CreateCondBr(builder.CreateAnd(loBoundCk, hiBoundCk), boundsChecked, boundsCheckFailed);
	builder.SetInsertPoint(boundsChecked);
	Value* valToStore = builder.CreateIntCast(xVal, Type::getInt16Ty(ctx), true);
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = builder.getInt64(zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x1)), builder.CreateAdd(yVal, zVal));
	emitStoreMem(ctx, m, f, builder, theA, emitAdjust16Endianness(builder, valToStore), epilogue);
	builder.SetInsertPoint(boundsCheckFailed);
	builder.CreateBr(epilogue); // ToDo : handle overflow
	builder.SetInsertPoint(epilogue);
	return epilogue;
}

llvm::BasicBlock* MmixLlvm::Private::emitStb(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
	llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *boundsChecked = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *boundsCheckFailed = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
	Value* loBoundCk = builder.CreateICmpSGE(xVal, builder.getInt64(SCHAR_MIN));
	Value* hiBoundCk = builder.CreateICmpSLE(xVal, builder.getInt64(SCHAR_MIN));
	builder.CreateCondBr(builder.CreateAnd(loBoundCk, hiBoundCk), boundsChecked, boundsCheckFailed);
	builder.SetInsertPoint(boundsChecked);
	Value* valToStore = builder.CreateIntCast(xVal, Type::getInt8Ty(ctx), true);
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = emitRegisterLoad(ctx, builder, registers, regMap, zarg);
	Value* theA = builder.CreateAdd(yVal, zVal);
	emitStoreMem(ctx, m, f, builder, theA, valToStore, epilogue);
	builder.SetInsertPoint(boundsCheckFailed);
	builder.CreateBr(epilogue); // ToDo : handle overflow
	builder.SetInsertPoint(epilogue);
	return epilogue;
}

llvm::BasicBlock* MmixLlvm::Private::emitStbi(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
	llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *boundsChecked = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *boundsCheckFailed = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
	Value* loBoundCk = builder.CreateICmpSGE(xVal, builder.getInt64(SCHAR_MIN));
	Value* hiBoundCk = builder.CreateICmpSLE(xVal, builder.getInt64(SCHAR_MIN));
	builder.CreateCondBr(builder.CreateAnd(loBoundCk, hiBoundCk), boundsChecked, boundsCheckFailed);
	builder.SetInsertPoint(boundsChecked);
	Value* valToStore = builder.CreateIntCast(xVal, Type::getInt8Ty(ctx), true);
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = builder.getInt64(zarg);
	Value* theA = builder.CreateAdd(yVal, zVal);
	emitStoreMem(ctx, m, f, builder, theA, valToStore, epilogue);
	builder.SetInsertPoint(boundsCheckFailed);
	builder.CreateBr(epilogue); // ToDo : handle overflow
	builder.SetInsertPoint(epilogue);
	return epilogue;
}

BasicBlock* MmixLlvm::Private::emitSttu(LLVMContext& ctx, llvm::Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = emitRegisterLoad(ctx, builder, registers, regMap, zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x3)), builder.CreateAdd(yVal, zVal));
	Value* valToStore = builder.CreateIntCast(xVal, Type::getInt32Ty(ctx), false);
	emitStoreMem(ctx, m, f, builder, theA, emitAdjust32Endianness(builder, valToStore), epilogue);
	builder.SetInsertPoint(epilogue);
	return epilogue;
}

BasicBlock* MmixLlvm::Private::emitStwu(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = emitRegisterLoad(ctx, builder, registers, regMap, zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x1)), builder.CreateAdd(yVal, zVal));
	Value* valToStore = builder.CreateIntCast(xVal, Type::getInt16Ty(ctx), false);
	emitStoreMem(ctx, m, f, builder, theA, emitAdjust16Endianness(builder, valToStore), epilogue);
	builder.SetInsertPoint(epilogue);
	return epilogue;
}

BasicBlock* MmixLlvm::Private::emitStbu(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = emitRegisterLoad(ctx, builder, registers, regMap, zarg);
	Value* theA = builder.CreateAdd(yVal, zVal);
	Value* valToStore = builder.CreateIntCast(xVal, Type::getInt8Ty(ctx), false);
	emitStoreMem(ctx, m, f, builder, theA, valToStore, epilogue);
	builder.SetInsertPoint(epilogue);
	return epilogue;
}

BasicBlock* MmixLlvm::Private::emitSttui(LLVMContext& ctx, llvm::Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = builder.getInt64(zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x3)), builder.CreateAdd(yVal, zVal));
	Value* valToStore = builder.CreateIntCast(xVal, Type::getInt32Ty(ctx), false);
	emitStoreMem(ctx, m, f, builder, theA, emitAdjust32Endianness(builder, valToStore), epilogue);
	builder.SetInsertPoint(epilogue);
	return epilogue;
}

BasicBlock* MmixLlvm::Private::emitStwui(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = builder.getInt64(zarg);
	Value* theA = builder.CreateAnd(builder.CreateNot(builder.getInt64(0x1)), builder.CreateAdd(yVal, zVal));
	Value* valToStore = builder.CreateIntCast(xVal, Type::getInt16Ty(ctx), false);
	emitStoreMem(ctx, m, f, builder, theA, emitAdjust16Endianness(builder, valToStore), epilogue);
	builder.SetInsertPoint(epilogue);
	return epilogue;
}

BasicBlock* MmixLlvm::Private::emitStbui(LLVMContext& ctx, Module& m, Function& f,
	BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	BasicBlock *entryBlock = entry != 0 ? entry : BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("block"), &f);
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(entryBlock);
	Value *registers = m.getGlobalVariable("Registers");
	Value* xVal = emitRegisterLoad(ctx, builder, registers, regMap, xarg);
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = builder.getInt64(zarg);
	Value* theA = builder.CreateAdd(yVal, zVal);
	Value* valToStore = builder.CreateIntCast(xVal, Type::getInt8Ty(ctx), false);
	emitStoreMem(ctx, m, f, builder, theA, valToStore, epilogue);
	builder.SetInsertPoint(epilogue);
	return epilogue;
}

