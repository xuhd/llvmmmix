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
using llvm::GenericValue;
using llvm::Intrinsic::ID;

using namespace MmixLlvm::Util;
using namespace MmixLlvm::Private;
using MmixLlvm::Private::RegisterRecord;
using MmixLlvm::Private::RegistersMap;

void MmixLlvm::Private::emitTrip(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	emitLeaveVerticeViaTrip(vctx, builder, builder.getInt64(yarg), builder.getInt64(zarg), 0);
}

void MmixLlvm::Private::emitTrap(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	LLVMContext& ctx = *vctx.Ctx;
	IRBuilder<> builder(ctx);
	builder.SetInsertPoint(vctx.Entry);
	Value* callParams[] = {
		builder.CreateLoad(vctx.Module->getGlobalVariable("ThisRef")),
		builder.getInt64(vctx.XPtr),
		emitSpecialRegisterLoad(vctx, builder, MmixLlvm::rTT)
	};
	Value* newxExpected = builder.getInt64(vctx.XPtr + 4);
	addSpecialRegisterToCache(vctx, MmixLlvm::rWW, newxExpected, true);
	Value* r255 = emitRegisterLoad(vctx, builder, 255);
	addSpecialRegisterToCache(vctx, MmixLlvm::rBB, r255, true);
	Value* rJ = emitSpecialRegisterLoad(vctx, builder, MmixLlvm::rJ);
	addRegisterToCache(vctx, 255, rJ, true);
	addSpecialRegisterToCache(vctx, MmixLlvm::rXX,
		builder.CreateOr(builder.CreateShl(builder.getInt64(1), 63), builder.getInt64(vctx.Instr)), true);
	addSpecialRegisterToCache(vctx, MmixLlvm::rYY, builder.getInt64(yarg), true);
	addSpecialRegisterToCache(vctx, MmixLlvm::rZZ, builder.getInt64(zarg), true);
	flushRegistersCache(vctx, builder);
	Value* newx = builder.CreateCall(vctx.Module->getFunction("TrapHandler"), ArrayRef<Value*>(callParams, callParams + 3));
	BasicBlock *verticeExit = BasicBlock::Create(ctx, genUniq("vertice_exit"), vctx.Function);
	if (!(yarg == 0 && zarg == 0))
		builder.CreateCondBr(builder.CreateICmpEQ(newxExpected, newx), vctx.Exit, verticeExit);
	else
		builder.CreateBr(verticeExit);
	builder.SetInsertPoint(verticeExit);
	Function::ArgumentListType::iterator argItr = vctx.Function->arg_begin();
	builder.CreateStore(builder.getInt64(vctx.XPtr), argItr);
	builder.CreateStore(newx, ++argItr);
	builder.CreateRetVoid();
}
