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
//using MmixLlvm::Private::RegisterRecord;
//using MmixLlvm::Private::RegistersMap;

void MmixLlvm::Private::emitSeth(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.getInt64((MXOcta)yzarg << 48);
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitSetmh(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.getInt64((MXOcta)yzarg << 32);
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitSetml(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.getInt64((MXOcta)yzarg << 16);
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitSetl(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.getInt64((MXOcta)yzarg);
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitInch(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.CreateAdd(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<48));
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitIncmh(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.CreateAdd(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<32));
	assignRegister(vctx, builder, xarg, result);	
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitIncml(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.CreateAdd(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<16));
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitIncl(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value *xval0 = vctx.getRegister(xarg);
	Value* result = builder.CreateAdd(xval0, builder.getInt64((MXOcta) yzarg));
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitOrh(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.CreateOr(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<48));
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitOrmh(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.CreateOr(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<32));
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitOrml(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.CreateOr(vctx.getRegister(xarg), builder.getInt64((MXOcta) yzarg<<16));
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitOrl(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.CreateOr(vctx.getRegister(xarg), builder.getInt64((MXOcta) yzarg));
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitAndnh(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.CreateAnd(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<48));
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitAndnmh(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.CreateAnd(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<32));
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitAndnml(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.CreateAnd(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg<<16));
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}

void MmixLlvm::Private::emitAndnl(VerticeContext& vctx, IRBuilder<>& builder,  MXByte xarg, MXWyde yzarg)
{
	Value* result = builder.CreateAnd(vctx.getRegister( xarg), builder.getInt64((MXOcta) yzarg));
	assignRegister(vctx, builder, xarg, result);
	builder.CreateBr(vctx.getOCExit());
}
