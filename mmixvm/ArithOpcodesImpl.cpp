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
	Value* yVal = emitRegisterLoad(ctx, builder, registers, regMap, yarg);
	Value* zVal = emitRegisterLoad(ctx, builder, registers, regMap, zarg);
	Value* result = builder.CreateAdd(yVal, zVal, Twine("Add") + Twine(yarg) + Twine(zarg));
	builder.CreateBr(vctx.Exit);
	RegisterRecord r0;
	r0.value = result;
	r0.changed = true;
	regMap[xarg] = r0;
}