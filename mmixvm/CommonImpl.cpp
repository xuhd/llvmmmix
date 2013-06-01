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
using llvm::cast;

using namespace MmixLlvm::Util;
using namespace MmixLlvm::Private;
using MmixLlvm::Private::RegisterRecord;
using MmixLlvm::Private::RegistersMap;

namespace {
	const uint32_t REGION_BIT_OFFSET = 61;
	const uint64_t TWO_ENABLED_BITS = 3i64;
	const uint64_t ADDR_MASK = ~(TWO_ENABLED_BITS << REGION_BIT_OFFSET);
};

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

namespace {
	Value* emitRegisterLoadImpl(VerticeContext& vctx, IRBuilder<>& builder, uint8_t reg, bool cache) {
		LLVMContext& ctx = *vctx.Ctx;
		RegistersMap& regMap = *vctx.RegMap;
		Value *regGlob = (*vctx.Module).getGlobalVariable("Registers");
		Value* retVal;
		RegistersMap::iterator itr = regMap.find(reg);
		if (itr == regMap.end()) {
			Value* ix[2];
			ix[0] = builder.getInt32(0);
			ix[1] = builder.getInt32(reg);
			retVal = builder.CreateLoad(
				builder.CreatePointerCast(
				builder.CreateGEP(regGlob, ArrayRef<Value*>(ix, ix + 2)), Type::getInt64PtrTy(ctx)), false, Twine("reg")+Twine(reg));
			if (cache) {
				RegisterRecord r0;
				r0.value = retVal;
				r0.changed = false;
				regMap[reg] = r0;
			}
		} else {
			retVal = (*itr).second.value;
		}
		return retVal;
	}

	Value* emitSpecialRegisterLoadImpl(VerticeContext& vctx, IRBuilder<>& builder, MmixLlvm::SpecialReg sreg, bool cache) {
		LLVMContext& ctx = *vctx.Ctx;
		RegistersMap& sregMap = *vctx.SpecialRegMap;
		Value *regGlob = (*vctx.Module).getGlobalVariable("SpecialRegisters");
		Value* retVal;
		RegistersMap::iterator itr = sregMap.find(sreg);
		if (itr == sregMap.end()) {
			Value* ix[2];
			ix[0] = builder.getInt32(0);
			ix[1] = builder.getInt32(sreg);
			retVal = builder.CreateLoad(
				builder.CreatePointerCast(
				builder.CreateGEP(regGlob, ArrayRef<Value*>(ix, ix + 2)), Type::getInt64PtrTy(ctx)), false, Twine("sreg")+Twine(sreg));
			if (cache) {
				RegisterRecord r0;
				r0.value = retVal;
				r0.changed = false;
				sregMap[sreg] = r0;
			}
		} else {
			retVal = (*itr).second.value;
		}
		return retVal;
	}
};

Value* MmixLlvm::Private::emitRegisterLoad(VerticeContext& vctx, IRBuilder<>& builder, uint8_t reg) {
	return emitRegisterLoadImpl(vctx, builder, reg, true);
}

Value* MmixLlvm::Private::emitSpecialRegisterLoad(VerticeContext& vctx, IRBuilder<>& builder, MmixLlvm::SpecialReg sreg) {
	return emitSpecialRegisterLoadImpl(vctx, builder, sreg, true);
}

Value* MmixLlvm::Private::emitQueryArithFlag(VerticeContext& vctx, IRBuilder<>& builder, MmixLlvm::ArithFlag flag) {
	Value* ra = emitSpecialRegisterLoad(vctx, builder, rA);
	return builder.CreateICmpNE(builder.CreateAnd(ra, builder.getInt64(flag)), builder.getInt64(0)); 
}

namespace {
	void emitLeaveVerticeImpl(VerticeContext& vctx, llvm::IRBuilder<>& builder,
		RegistersMap& regMap, RegistersMap& sregMap, uint64_t target)
	{
		LLVMContext& ctx = *vctx.Ctx;
		Value *specialRegisters = (*vctx.Module).getGlobalVariable("SpecialRegisters");
		for (RegistersMap::iterator itr = sregMap.begin(); itr != sregMap.end(); ++itr) {
			if ((*itr).second.changed) {
				Value* ix[2];
				ix[0] = builder.getInt32(0);
				ix[1] = builder.getInt32((*itr).first);
				builder.CreateStore(
					(*itr).second.value,
					builder.CreatePointerCast(
					builder.CreateGEP(specialRegisters, ArrayRef<Value*>(ix, ix + 2)), Type::getInt64PtrTy(ctx)));
			}
		}
		Value *registers = (*vctx.Module).getGlobalVariable("Registers");
		for (RegistersMap::iterator itr = regMap.begin(); itr != regMap.end(); ++itr) {
			if ((*itr).second.changed) {
				Value* ix[2];
				ix[0] = builder.getInt32(0);
				ix[1] = builder.getInt32((*itr).first);
				builder.CreateStore(
					(*itr).second.value,
					builder.CreatePointerCast(
					builder.CreateGEP(registers, ArrayRef<Value*>(ix, ix + 2)), Type::getInt64PtrTy(ctx)));
			}
		}
		Function::ArgumentListType::iterator argItr = (*vctx.Function).arg_begin();
		builder.CreateStore(builder.getInt64(vctx.XPtr), argItr);
		builder.CreateStore(builder.getInt64(target), ++argItr);
		builder.CreateRetVoid();
	}
};

void MmixLlvm::Private::emitLeaveVerticeViaTrip(VerticeContext& vctx, llvm::IRBuilder<>& builder,
	llvm::Value* rY, llvm::Value* rZ, uint64_t target)
{
	RegistersMap regMap(*vctx.RegMap);
	RegistersMap sregMap(*vctx.SpecialRegMap);
	RegisterRecord r0;
	r0.changed = true;
	r0.value = emitRegisterLoadImpl(vctx, builder, 255, false);
	sregMap[MmixLlvm::SpecialReg::rB] = r0;
	r0.value = emitSpecialRegisterLoadImpl(vctx, builder, MmixLlvm::SpecialReg::rJ, false);
	regMap[255] = r0;
	r0.value = builder.CreateOr(builder.CreateShl(builder.getInt64(1), 63), builder.getInt64(vctx.Instr));
	sregMap[MmixLlvm::SpecialReg::rX] = r0;
	r0.value = rY;
	sregMap[MmixLlvm::SpecialReg::rY] = r0;
	r0.value = rZ;
	sregMap[MmixLlvm::SpecialReg::rZ] = r0;
	emitLeaveVerticeImpl(vctx, builder, regMap, sregMap, target);
}

void MmixLlvm::Private::emitLeaveVertice(VerticeContext& vctx, IRBuilder<>& builder, uint64_t target) {
	emitLeaveVerticeImpl(vctx, builder, *vctx.RegMap, *vctx.SpecialRegMap, target);
}

Value* MmixLlvm::Private::emitFetchMem(LLVMContext& ctx, Module& m, Function& f,
	IRBuilder<>& builder, Value* theA, Type* ty) 
{
	Value* glob = m.getGlobalVariable("AddressTranslateTable");
	Value* ix[2];
	ix[0] = builder.getInt32(0);
	ix[1] = builder.CreateIntCast(
		builder.CreateAnd(
			builder.CreateLShr(theA, REGION_BIT_OFFSET),
			builder.getInt64(TWO_ENABLED_BITS)),
		Type::getInt32Ty(ctx), false);
	Value *attVal = builder.CreateLoad(builder.CreateGEP(glob, ArrayRef<Value*>(ix, ix + 2)));
	glob = m.getGlobalVariable("Memory");
	Value* normAddr = builder.CreateIntCast(builder.CreateAnd(theA, builder.getInt64(ADDR_MASK)), Type::getInt32Ty(ctx), false);
	ix[1] = builder.CreateAdd(normAddr, attVal);
	Value* targetPtr = builder.CreatePointerCast(
		builder.CreateGEP(glob, ArrayRef<Value*>(ix, ix + 2)), PointerType::get(ty, 0)); 
	return builder.CreateLoad(targetPtr);
}

void MmixLlvm::Private::emitStoreMem(LLVMContext& ctx, Module& m, Function& f,
	IRBuilder<>& builder, Value* theA, Value* val)
{
	Value* glob = m.getGlobalVariable("AddressTranslateTable");
	Value* ix[2];
	ix[0] = builder.getInt32(0);
	ix[1] = builder.CreateIntCast(
		builder.CreateAnd(
			builder.CreateLShr(theA, REGION_BIT_OFFSET),
			builder.getInt64(TWO_ENABLED_BITS)),
		Type::getInt32Ty(ctx), false);
	Value *attVal = builder.CreateLoad(builder.CreateGEP(glob, ArrayRef<Value*>(ix, ix + 2)));
	glob = m.getGlobalVariable("Memory");
	Value* normAddr = builder.CreateIntCast(builder.CreateAnd(theA, builder.getInt64(ADDR_MASK)), Type::getInt32Ty(ctx), false);
	ix[1] = builder.CreateAdd(normAddr, attVal);
	Value* targetPtr = builder.CreatePointerCast(
		builder.CreateGEP(glob, ArrayRef<Value*>(ix, ix + 2)), (*(*val).getType()).getPointerTo()); 
	builder.CreateStore(val, targetPtr);
}

uint64_t MmixLlvm::Private::getArithTripVector(MmixLlvm::ArithFlag flag) {
	switch (flag) {
	case MmixLlvm::ArithFlag::X:
		return 128;
	case MmixLlvm::ArithFlag::Z:
		return 112;
	case MmixLlvm::ArithFlag::U:
		return 96;
	case MmixLlvm::ArithFlag::O:
		return 80;
	case MmixLlvm::ArithFlag::I:
		return 64;
	case MmixLlvm::ArithFlag::W:
		return 48;
	case MmixLlvm::ArithFlag::V:
		return 32;
	case MmixLlvm::ArithFlag::D:
		return 16;
	default:
		return 0;
	}
}
