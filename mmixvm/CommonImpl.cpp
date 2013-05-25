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

Value* MmixLlvm::Private::emitAdjust64Endianness(IRBuilder<>& builder, Value* val) {
	Value* b0 = builder.CreateLShr(val, builder.getInt64(56));
	Value* b1 = builder.CreateAnd(builder.CreateLShr(val, builder.getInt64(48)), builder.getInt64(0xFF));
	Value* b2 = builder.CreateAnd(builder.CreateLShr(val, builder.getInt64(40)), builder.getInt64(0xFF));
	Value* b3 = builder.CreateAnd(builder.CreateLShr(val, builder.getInt64(32)), builder.getInt64(0xFF));
	Value* b4 = builder.CreateAnd(builder.CreateLShr(val, builder.getInt64(24)), builder.getInt64(0xFF));
	Value* b5 = builder.CreateAnd(builder.CreateLShr(val, builder.getInt64(16)), builder.getInt64(0xFF));
	Value* b6 = builder.CreateAnd(builder.CreateLShr(val, builder.getInt64(8)), builder.getInt64(0xFF));
	Value* b7 = builder.CreateAnd(val, builder.getInt64(0xFF));
	Value* q0 = builder.CreateShl(b7, builder.getInt64(56));
	Value* q1 = builder.CreateOr(q0, builder.CreateShl(b6, builder.getInt64(48)));
	Value* q2 = builder.CreateOr(q1, builder.CreateShl(b5, builder.getInt64(40)));
	Value* q3 = builder.CreateOr(q2, builder.CreateShl(b4, builder.getInt64(32)));
	Value* q4 = builder.CreateOr(q3, builder.CreateShl(b3, builder.getInt64(24)));
	Value* q5 = builder.CreateOr(q4, builder.CreateShl(b2, builder.getInt64(16)));
	Value* q6 = builder.CreateOr(q5, builder.CreateShl(b1, builder.getInt64(8)));
	Value* q7 = builder.CreateOr(q6, b0);
	return q7;
}

Value* MmixLlvm::Private::emitAdjust32Endianness(IRBuilder<>& builder, Value* val) {
	Value* b0 = builder.CreateLShr(val, builder.getInt32(24));
	Value* b1 = builder.CreateAnd(builder.CreateLShr(val, builder.getInt32(16)), builder.getInt32(0xFF));
	Value* b2 = builder.CreateAnd(builder.CreateLShr(val, builder.getInt32(8)), builder.getInt32(0xFF));
	Value* b3 = builder.CreateAnd(val, builder.getInt32(0xFF));
	Value* q0 = builder.CreateShl(b3, builder.getInt32(24));
	Value* q1 = builder.CreateOr(q0, builder.CreateShl(b2, builder.getInt32(16)));
	Value* q2 = builder.CreateOr(q1, builder.CreateShl(b1, builder.getInt32(8)));
	Value* q3 = builder.CreateOr(q2, b0);
	return q3;
}

Value* MmixLlvm::Private::emitAdjust16Endianness(IRBuilder<>& builder, Value* val) {
	Value* b0 = builder.CreateLShr(val, builder.getInt16(8));
	Value* b1 = builder.CreateAnd(val, builder.getInt16(0xFF));
	Value* q0 = builder.CreateShl(b1, builder.getInt16(8));
	Value* q1 = builder.CreateOr(q0, b0);
	return q1;
}

Value* MmixLlvm::Private::emitRegisterLoad(llvm::LLVMContext& ctx, IRBuilder<>& builder, Value* regGlob, RegistersMap& regMap, uint8_t reg) {
	Value* retVal;
	RegistersMap::iterator itr = regMap.find(reg);
	if (itr == regMap.end()) {
		Value* ix[2];
		ix[0] = builder.getInt32(0);
		ix[1] = builder.getInt32(reg);
		retVal = builder.CreateLoad(
			builder.CreatePointerCast(
			builder.CreateGEP(regGlob, ArrayRef<Value*>(ix, ix + 2)), Type::getInt64PtrTy(ctx)));
		RegisterRecord r0;
		r0.value = retVal;
		r0.changed = false;
		regMap[reg] = r0;
	} else {
		retVal = (*itr).second.value;
	}
	return retVal;
}

Value* MmixLlvm::Private::emitGetTextPtr(llvm::LLVMContext& ctx, llvm::Module& m, IRBuilder<>& builder, Value* theA, PointerType* ty) {
	Value* glob = m.getGlobalVariable("TextSeg");
	Value* ix[2];
	ix[0] = builder.getInt32(0);
	ix[1] = builder.CreateIntCast(theA, Type::getInt32Ty(ctx), false);
	return builder.CreatePointerCast(
		builder.CreateGEP(glob, ArrayRef<Value*>(ix, ix + 2)), ty);
}

Value* MmixLlvm::Private::emitGetDataPtr(llvm::LLVMContext& ctx, llvm::Module& m, IRBuilder<>& builder, Value* theA, PointerType* ty) {
	Value* glob = m.getGlobalVariable("DataSeg");
	Value* ix[2];
	ix[0] = builder.getInt32(0);
	ix[1] = builder.CreateIntCast(
		builder.CreateSub(theA, builder.getInt64(MmixLlvm::DATA_SEG)),
		Type::getInt32Ty(ctx), false);
	return builder.CreatePointerCast(
		builder.CreateGEP(glob, ArrayRef<Value*>(ix, ix + 2)), ty);
}

Value* MmixLlvm::Private::emitGetPoolPtr(llvm::LLVMContext& ctx, llvm::Module& m, IRBuilder<>& builder, Value* theA, PointerType* ty) {
	Value* glob = m.getGlobalVariable("PoolSeg");
	Value* ix[2];
	ix[0] = builder.getInt32(0);
	ix[1] = builder.CreateIntCast(
		builder.CreateSub(theA, builder.getInt64(MmixLlvm::POOL_SEG)),
		Type::getInt32Ty(ctx), false);
	return builder.CreatePointerCast(
		builder.CreateGEP(glob, ArrayRef<Value*>(ix, ix + 2)), ty);
}

Value* MmixLlvm::Private::emitGetStackPtr(llvm::LLVMContext& ctx, llvm::Module& m, IRBuilder<>& builder, Value* theA, PointerType* ty) {
	Value* glob = m.getGlobalVariable("StackSeg");
	Value* ix[2];
	ix[0] = builder.getInt32(0);
	ix[1] = builder.CreateIntCast(
		builder.CreateSub(theA, builder.getInt64(MmixLlvm::STACK_SEG)),
		Type::getInt32Ty(ctx), false);
	return builder.CreatePointerCast(
		builder.CreateGEP(glob, ArrayRef<Value*>(ix, ix + 2)), ty);
}

Value* MmixLlvm::Private::emitFetchMem(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
	IRBuilder<>& builder, Value* theA, Type* ty, BasicBlock* exit) 
{
	Value* retVal = builder.CreateAlloca(ty);
	BasicBlock *readTextBlock = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *readDataBlock = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *readPoolBlock = BasicBlock::Create(ctx, genUniq("block"), &f);
	BasicBlock *readStackBlock = BasicBlock::Create(ctx, genUniq("block"), &f);
	SwitchInst *switchInst = builder.CreateSwitch(builder.CreateLShr(theA, 61), readTextBlock, 4);
	(*switchInst).addCase(builder.getInt64(0), readTextBlock);
	(*switchInst).addCase(builder.getInt64(1), readDataBlock);
	(*switchInst).addCase(builder.getInt64(2), readPoolBlock);
	(*switchInst).addCase(builder.getInt64(3), readStackBlock);
	Value* theValue;
	builder.SetInsertPoint(readTextBlock);
	theValue = builder.CreateLoad(emitGetTextPtr(ctx, m, builder, theA, PointerType::get(ty, 0)));
	builder.CreateStore(theValue, retVal);
	builder.CreateBr(exit);
	builder.SetInsertPoint(readDataBlock);
	theValue = builder.CreateLoad(emitGetDataPtr(ctx, m, builder, theA, PointerType::get(ty, 0)));
	builder.CreateStore(theValue, retVal);
	builder.CreateBr(exit);
	builder.SetInsertPoint(readPoolBlock);
	theValue = builder.CreateLoad(emitGetPoolPtr(ctx, m, builder, theA, PointerType::get(ty, 0)));
	builder.CreateStore(theValue, retVal);
	builder.CreateBr(exit);
	builder.SetInsertPoint(readStackBlock);
	theValue = builder.CreateLoad(emitGetStackPtr(ctx, m, builder, theA, PointerType::get(ty, 0)));
	builder.CreateStore(theValue, retVal);
	builder.CreateBr(exit);
	return retVal;
}
