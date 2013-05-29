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
using llvm::Twine;

using namespace MmixLlvm::Util;
using namespace MmixLlvm::Private;
using MmixLlvm::Private::RegisterRecord;
using MmixLlvm::Private::RegistersMap;

void MmixLlvm::Private::emitAdd(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	RegistersMap& regMap = *vctx.RegMap;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value *registers = (*vctx.Module).getGlobalVariable("Registers");
	Type* intrinsicArgs[] = {Type::getInt64Ty(ctx) };
	Function* addIntr = llvm::Intrinsic::getDeclaration(vctx.Module,
		llvm::Intrinsic::sadd_with_overflow, 
		ArrayRef<Type*>(intrinsicArgs, intrinsicArgs + 1));
	Value* args[] = {
		emitRegisterLoad(ctx, builder, registers, regMap, yarg), 
		emitRegisterLoad(ctx, builder, registers, regMap, zarg)
	};
	Value* resStruct = builder.CreateCall(addIntr, ArrayRef<Value*>(args, args + 2));
	BasicBlock *success = BasicBlock::Create(ctx, genUniq("add_success"), vctx.Function);
	BasicBlock *overflow = BasicBlock::Create(ctx, genUniq("overflow"), vctx.Function);
	BasicBlock *addEpilogue = BasicBlock::Create(ctx, genUniq("add_epilogue"), vctx.Function);
	uint32_t idx[1];
	idx[0] = 1;
	Value* overflowFlag = builder.CreateExtractValue(resStruct, ArrayRef<uint32_t>(idx, idx + 1), "overflowFlag");
	Value* iref = builder.CreateAlloca(Type::getInt64Ty(ctx));
	builder.CreateCondBr(overflowFlag, overflow, success);
	builder.SetInsertPoint(success);
	idx[0] = 0;
	Value* addResult = builder.CreateExtractValue(resStruct, ArrayRef<uint32_t>(idx, idx + 1), "add" + Twine(yarg) + Twine(zarg));
	builder.CreateStore(addResult, iref);
	builder.CreateBr(addEpilogue);
	builder.SetInsertPoint(overflow);
	builder.CreateStore(emitRegisterLoad(ctx, builder, registers, regMap, xarg), iref);
	builder.CreateBr(addEpilogue);
	builder.SetInsertPoint(addEpilogue);
	Value* result = builder.CreateLoad(iref, false);
	builder.CreateBr(vctx.Exit);
	RegisterRecord r0;
	r0.value = result;
	r0.changed = true;
	regMap[xarg] = r0;
}