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

void MmixLlvm::Private::emitSeth(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.getInt64((MXOcta)yzarg << 48);
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitSetmh(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.getInt64((MXOcta)yzarg << 32);
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitSetml(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.getInt64((MXOcta)yzarg << 16);
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitSetl(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.getInt64((MXOcta)yzarg);
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitInch(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.CreateAdd(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<48));
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitIncmh(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.CreateAdd(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<32));
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitIncml(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.CreateAdd(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<16));
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitIncl(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value *xval0 = vctx.getRegister( xarg);
	//debugInt64(vctx, builder, xval0);
	Value* result = builder.CreateAdd(xval0, builder.getInt64((MXOcta) yzarg));
	//debugInt64(vctx, builder, result);
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitOrh(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.CreateOr(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<48));
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitOrmh(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.CreateOr(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<32));
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitOrml(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.CreateOr(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<16));
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitOrl(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.CreateOr(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg));
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitAndnh(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.CreateAnd(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<48));
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitAndnmh(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.CreateAnd(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<32));
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitAndnml(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.CreateAnd(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<16));
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}

void MmixLlvm::Private::emitAndnl(VerticeContext& vctx, MXByte xarg, MXWyde yzarg)
{
	
	IRBuilder<> builder(vctx.getLctx());
	builder.SetInsertPoint(vctx.getOCEntry());
	Value* result = builder.CreateAnd(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg));
	builder.CreateBr(vctx.getOCExit());
	vctx.assignRegister(xarg,result );
}
