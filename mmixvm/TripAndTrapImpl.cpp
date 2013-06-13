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

void MmixLlvm::Private::emitTrip(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	emitLeaveVerticeViaTrip(vctx, builder, builder.getInt64(yarg), builder.getInt64(zarg), builder.getInt64(0));
}

void MmixLlvm::Private::emitTrap(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* trapVector = emitSpecialRegisterLoad(vctx, builder, MmixLlvm::rT);
	emitLeaveVerticeViaTrap(vctx, builder, builder.getInt64(yarg), builder.getInt64(zarg), trapVector);
}
