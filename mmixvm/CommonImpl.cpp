#include "stdafx.h"
#include "Util.h"
#include "MmixEmit.h"
#include "MmixEmitPvt.h"
#include "MmixDef.h"

using llvm::LLVMContext;
using llvm::Module;
using llvm::Type;
using llvm::PointerType;
using llvm::Value;
using llvm::Argument;
using llvm::Function;
using llvm::BasicBlock;
using llvm::IRBuilder;
using llvm::ArrayRef;
using llvm::SwitchInst;
using llvm::Twine;
using llvm::cast;

using namespace MmixLlvm::Util;
using namespace MmixLlvm::Private;
using MmixLlvm::Private::RegisterRecord;
using MmixLlvm::Private::RegistersMap;
using MmixLlvm::MXByte;
using MmixLlvm::MXTetra;
using MmixLlvm::MXOcta;

VerticeContext::~VerticeContext() { }

namespace {
	const MXTetra REGION_BIT_OFFSET = 61;
	const MXOcta TWO_ENABLED_BITS = 3i64;
	const MXOcta ADDR_MASK = ~(TWO_ENABLED_BITS << REGION_BIT_OFFSET);
};

Value* MmixLlvm::Private::emitAdjust64Endianness(VerticeContext& vctx, IRBuilder<>& builder, Value* val) {
	Function* f = vctx.getModuleFunction("Adjust64EndiannessImpl");
	Value* args[] = { val };
	return builder.CreateCall(f, ArrayRef<Value*>(args, args + 1));
}

Value* MmixLlvm::Private::emitAdjust32Endianness(VerticeContext& vctx, IRBuilder<>& builder, Value* val) {
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

Value* MmixLlvm::Private::emitAdjust16Endianness(VerticeContext& vctx, IRBuilder<>& builder, Value* val) {
	Value* b0 = builder.CreateLShr(val, builder.getInt16(8));
	Value* b1 = builder.CreateAnd(val, builder.getInt16(0xFF));
	Value* q0 = builder.CreateShl(b1, builder.getInt16(8));
	Value* q1 = builder.CreateOr(q0, b0);
	return q1;
}

Value* MmixLlvm::Private::emitQueryArithFlag(VerticeContext& vctx, IRBuilder<>& builder, MmixLlvm::ArithFlag flag) {
	Value* ra = vctx.getSpRegister(rA);
	return builder.CreateICmpNE(builder.CreateAnd(ra, builder.getInt64(flag)), builder.getInt64(0)); 
}

namespace {
	void saveRegisters(VerticeContext& vctx, IRBuilder<>& builder)
	{
		LLVMContext& ctx = vctx.getLctx();
		Value* regGlob = vctx.getModuleVar("RegisterStackTop");
		Value *specialRegisters = vctx.getModuleVar("SpecialRegisters");
		std::vector<MXByte> dirtyRegs(vctx.getDirtyRegisters());
		for (std::vector<MXByte>::iterator itr = dirtyRegs.begin();
			 itr != dirtyRegs.end();
			 ++itr)
		{
			builder.CreateStore(
				vctx.getRegister(*itr),
					builder.CreateGEP(
						builder.CreateLoad(regGlob, false),
						builder.getInt32(*itr)));
		}
		std::vector<MmixLlvm::SpecialReg> dirtySRegs(vctx.getDirtySpRegisters());
		for (std::vector<MmixLlvm::SpecialReg>::iterator itr = dirtySRegs.begin();
			 itr != dirtySRegs.end();
			++itr)
		{
			Value* ix[2];
			ix[0] = builder.getInt32(0);
			ix[1] = builder.getInt32(*itr);
			builder.CreateStore(
				vctx.getSpRegister(*itr),
				builder.CreatePointerCast(
					builder.CreateGEP(specialRegisters, ArrayRef<Value*>(ix, ix + 2)), Type::getInt64PtrTy(ctx)));
		}
	}
}

void MmixLlvm::Private::flushRegistersCache(VerticeContext& vctx, llvm::IRBuilder<>& builder) {
	saveRegisters(vctx, builder);
	vctx.markAllClean();
}

void MmixLlvm::Private::emitLeaveVerticeViaTrip(VerticeContext& vctx, llvm::IRBuilder<>& builder,
	llvm::Value* rY, llvm::Value* rZ, MXOcta target)
{
	boost::shared_ptr<VerticeContext> branch(vctx.makeBranch());
	branch->assignSpRegister(MmixLlvm::rW, builder.getInt64(branch->getXPtr() + 4));
	Value* r255 = branch->getRegister(255);
	branch->assignSpRegister(MmixLlvm::rB, r255);
	Value* rJ = branch->getSpRegister(MmixLlvm::rJ);
	branch->assignRegister(255, rJ);
	branch->assignSpRegister(MmixLlvm::rX, 
		builder.CreateOr(builder.CreateShl(builder.getInt64(1), 63), builder.getInt64(branch->getInstr())));
	branch->assignSpRegister(MmixLlvm::rY, rY);
	branch->assignSpRegister(MmixLlvm::rZ, rZ);
	saveRegisters(*branch, builder);
	std::vector<Argument*> args(vctx.getVerticeArgs());
	builder.CreateStore(builder.getInt64(branch->getXPtr()), args[0]);
	builder.CreateStore(builder.getInt64(target), args[1]);
	builder.CreateRetVoid();
}

void MmixLlvm::Private::emitLeaveVerticeViaJump(VerticeContext& vctx, IRBuilder<>& builder, MXOcta target) 
{
	saveRegisters(vctx, builder);
	std::vector<Argument*> args(vctx.getVerticeArgs());
	builder.CreateStore(builder.getInt64(vctx.getXPtr()), args[0]);
	builder.CreateStore(builder.getInt64(target), args[1]);
	builder.CreateRetVoid();
}

Value* MmixLlvm::Private::emitFetchMem(VerticeContext& vctx, IRBuilder<>& builder, Value* theA, Type* ty) 
{
	Value* glob = vctx.getModuleVar("AddressTranslateTable");
	Value* ix[2];
	ix[0] = builder.getInt32(0);
	ix[1] = builder.CreateIntCast(
		builder.CreateAnd(
			builder.CreateLShr(theA, REGION_BIT_OFFSET),
			builder.getInt64(TWO_ENABLED_BITS)),
			Type::getInt32Ty(vctx.getLctx()), false);
	Value *attVal = builder.CreateLoad(builder.CreateGEP(glob, ArrayRef<Value*>(ix, ix + 2)));
	glob = vctx.getModuleVar("Memory");
	Value* normAddr = builder.CreateIntCast(builder.CreateAnd(theA, builder.getInt64(ADDR_MASK)), Type::getInt32Ty(vctx.getLctx()), false);
	ix[1] = builder.CreateAdd(normAddr, attVal);
	Value* targetPtr = builder.CreatePointerCast(
		builder.CreateGEP(glob, ArrayRef<Value*>(ix, ix + 2)), PointerType::get(ty, 0)); 
	return builder.CreateLoad(targetPtr);
}

void MmixLlvm::Private::emitStoreMem(VerticeContext& vctx, IRBuilder<>& builder, Value* theA, Value* val)
{
	Value* glob = vctx.getModuleVar("AddressTranslateTable");
	Value* ix[2];
	ix[0] = builder.getInt32(0);
	ix[1] = builder.CreateIntCast(
		builder.CreateAnd(
			builder.CreateLShr(theA, REGION_BIT_OFFSET),
			builder.getInt64(TWO_ENABLED_BITS)),
				Type::getInt32Ty(vctx.getLctx()), false);
	Value *attVal = builder.CreateLoad(builder.CreateGEP(glob, ArrayRef<Value*>(ix, ix + 2)));
	glob = vctx.getModuleVar("Memory");
	Value* normAddr = builder.CreateIntCast(
		builder.CreateAnd(theA, builder.getInt64(ADDR_MASK)), 
			Type::getInt32Ty(vctx.getLctx()), false);
	ix[1] = builder.CreateAdd(normAddr, attVal);
	Value* targetPtr = builder.CreatePointerCast(
		builder.CreateGEP(glob, ArrayRef<Value*>(ix, ix + 2)), (*(*val).getType()).getPointerTo()); 
	builder.CreateStore(val, targetPtr);
}

MXOcta MmixLlvm::Private::getArithTripVector(MmixLlvm::ArithFlag flag) {
	switch (flag) {
	case MmixLlvm::X:
		return 128;
	case MmixLlvm::Z:
		return 112;
	case MmixLlvm::U:
		return 96;
	case MmixLlvm::O:
		return 80;
	case MmixLlvm::I:
		return 64;
	case MmixLlvm::W:
		return 48;
	case MmixLlvm::V:
		return 32;
	case MmixLlvm::D:
		return 16;
	default:
		return 0;
	}
}

void MmixLlvm::Private::debugInt64(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* val) {
	Value* callParams[] = { val };
	builder.CreateCall(vctx.getModuleFunction("DebugInt64"), ArrayRef<Value*>(callParams, callParams + 1));
}