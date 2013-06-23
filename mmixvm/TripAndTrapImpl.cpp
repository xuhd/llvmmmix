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
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	emitLeaveVerticeViaTrip(vctx, builder, builder.getInt64(yarg), builder.getInt64(zarg), 0);
}

void MmixLlvm::Private::emitTrap(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg)
{
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* callParams[] = {
		builder.CreateLoad(vctx.getModule().getGlobalVariable("ThisRef")),
		builder.getInt64(vctx.getXPtr()),
		vctx.getSpRegister(MmixLlvm::rTT)
	};
	Value* newxExpected = builder.getInt64(vctx.getXPtr() + 4);
	//addSpecialRegisterToCache(vctx, MmixLlvm::rWW, newxExpected, true);
	vctx.assignSpRegister(MmixLlvm::rWW, newxExpected);
	Value* r255 = vctx.getRegister(255);
	//addSpecialRegisterToCache(vctx, MmixLlvm::rBB, r255, true);
	vctx.assignSpRegister(MmixLlvm::rBB, r255);
	Value* rJ = vctx.getSpRegister(MmixLlvm::rJ);
	//addRegisterToCache(vctx, 255, rJ, true);
	vctx.assignRegister(255, rJ);
	vctx.assignSpRegister(MmixLlvm::rXX,
		builder.CreateOr(builder.CreateShl(builder.getInt64(1), 63), builder.getInt64(vctx.getInstr())));
	vctx.assignSpRegister(MmixLlvm::rYY, builder.getInt64(yarg));
	vctx.assignSpRegister(MmixLlvm::rZZ, builder.getInt64(zarg));
	flushRegistersCache(vctx, builder);
	Value* newx = builder.CreateCall(vctx.getModule().getFunction("TrapHandler"), ArrayRef<Value*>(callParams, callParams + 3));
	BasicBlock *verticeExit = BasicBlock::Create(vctx.getLctx(), genUniq("vertice_exit"), &vctx.getFunction());
	if (!(yarg == 0 && zarg == 0))
		builder.CreateCondBr(builder.CreateICmpEQ(newxExpected, newx), vctx.getOCExit(), verticeExit);
	else
		builder.CreateBr(verticeExit);
	builder.SetInsertPoint(verticeExit);
	Function::ArgumentListType::iterator argItr = vctx.getFunction().arg_begin();
	builder.CreateStore(builder.getInt64(vctx.getXPtr()), argItr);
	builder.CreateStore(newx, ++argItr);
	builder.CreateRetVoid();
}
