#include "stdafx.h"
#include "Util.h"
#include "MmixEmitPvt.h"
#include "MmixDef.h"

using llvm::LLVMContext;
using llvm::Module;
using llvm::Type;
using llvm::PointerType;
using llvm::IntegerType;
using llvm::Value;
using llvm::Function;
using llvm::BasicBlock;
using llvm::IRBuilder;
using llvm::ArrayRef;
using llvm::SwitchInst;
using llvm::Twine;
using llvm::PHINode; 
using llvm::Intrinsic::ID;

using namespace MmixLlvm::Util;
using namespace MmixLlvm::Private;
using MmixLlvm::Private::RegisterRecord;
using MmixLlvm::Private::RegistersMap;

void MmixLlvm::Private::emitSeth(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.getInt64((uint64_t)yzarg << 48);
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitSetmh(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.getInt64((uint64_t)yzarg << 32);
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitSetml(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.getInt64((uint64_t)yzarg << 16);
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitSetl(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.getInt64((uint64_t)yzarg);
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitInch(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.CreateAdd(emitRegisterLoad(vctx, builder, xarg), builder.getInt64((uint64_t) yzarg<<48));
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitIncmh(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.CreateAdd(emitRegisterLoad(vctx, builder, xarg), builder.getInt64((uint64_t) yzarg<<32));
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitIncml(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.CreateAdd(emitRegisterLoad(vctx, builder, xarg), builder.getInt64((uint64_t) yzarg<<16));
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitIncl(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.CreateAdd(emitRegisterLoad(vctx, builder, xarg), builder.getInt64((uint64_t) yzarg));
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitOrh(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.CreateOr(emitRegisterLoad(vctx, builder, xarg), builder.getInt64((uint64_t) yzarg<<48));
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitOrmh(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.CreateOr(emitRegisterLoad(vctx, builder, xarg), builder.getInt64((uint64_t) yzarg<<32));
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitOrml(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.CreateOr(emitRegisterLoad(vctx, builder, xarg), builder.getInt64((uint64_t) yzarg<<16));
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitOrl(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.CreateOr(emitRegisterLoad(vctx, builder, xarg), builder.getInt64((uint64_t) yzarg));
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitAndnh(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.CreateAnd(emitRegisterLoad(vctx, builder, xarg), builder.getInt64((uint64_t) yzarg<<48));
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitAndnmh(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.CreateAnd(emitRegisterLoad(vctx, builder, xarg), builder.getInt64((uint64_t) yzarg<<32));
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitAndnml(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.CreateAnd(emitRegisterLoad(vctx, builder, xarg), builder.getInt64((uint64_t) yzarg<<16));
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}

void MmixLlvm::Private::emitAndnl(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* result = builder.CreateAnd(emitRegisterLoad(vctx, builder, xarg), builder.getInt64((uint64_t) yzarg));
	builder.CreateBr(vctx.Exit);
	addRegisterToCache(vctx, xarg, result, true);
}
