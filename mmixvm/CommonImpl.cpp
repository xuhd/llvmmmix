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

namespace {
	const MXTetra REGION_BIT_OFFSET = 61;
	const MXOcta TWO_ENABLED_BITS = 3i64;
	const MXOcta ADDR_MASK = ~(TWO_ENABLED_BITS << REGION_BIT_OFFSET);
};

Value* MmixLlvm::Private::emitAdjust64Endianness(VerticeContext& vctx, IRBuilder<>& builder, Value* val) {
	Function* f = vctx.Module->getFunction("Adjust64EndiannessImpl");
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

namespace {
	Value* emitRegisterLoadImpl(VerticeContext& vctx, IRBuilder<>& builder, MXByte reg, bool cache) {
		LLVMContext& ctx = *vctx.Ctx;
		Value *regGlob = vctx.Module->getGlobalVariable("Registers");
		Value* retVal;
		RegistersMap::iterator itr = vctx.RegMap.find(reg);
		if (itr == vctx.RegMap.end()) {
			Value* ix[2];
			ix[0] = builder.getInt32(0);
			ix[1] = builder.getInt32(reg);
			retVal = builder.CreateLoad(
				builder.CreatePointerCast(
				builder.CreateGEP(regGlob, ArrayRef<Value*>(ix, ix + 2)), Type::getInt64PtrTy(ctx)), false, Twine("reg")+Twine(reg));
			if (cache)
				addRegisterToCache(vctx, reg, retVal, false);
		} else {
			retVal = itr->second.value;
		}
		return retVal;
	}

	Value* emitSpecialRegisterLoadImpl(VerticeContext& vctx, IRBuilder<>& builder, MmixLlvm::SpecialReg sreg, bool cache) {
		LLVMContext& ctx = *vctx.Ctx;
		Value *regGlob = vctx.Module->getGlobalVariable("SpecialRegisters");
		Value* retVal;
		RegistersMap::iterator itr = vctx.SpecialRegMap.find(sreg);
		if (itr == vctx.SpecialRegMap.end()) {
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
				vctx.SpecialRegMap[sreg] = r0;
			}
		} else {
			retVal = itr->second.value;
		}
		return retVal;
	}
};

void MmixLlvm::Private::addRegisterToCache(VerticeContext& vctx, MXByte reg, Value* val, bool markDirty) {
	RegisterRecord r0;
	r0.value = val;
	r0.changed = markDirty;
	vctx.RegMap[reg] = r0;
}

void MmixLlvm::Private::addSpecialRegisterToCache(VerticeContext& vctx, MXByte reg, Value* val, bool markDirty) {
	RegisterRecord r0;
	r0.value = val;
	r0.changed = markDirty;
	vctx.SpecialRegMap[reg] = r0;
}

Value* MmixLlvm::Private::emitRegisterLoad(VerticeContext& vctx, IRBuilder<>& builder, MXByte reg) {
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
	void saveRegisters(VerticeContext& vctx, IRBuilder<>& builder, RegistersMap& regMap, RegistersMap& sregMap, bool markClean)
	{
		LLVMContext& ctx = *vctx.Ctx;
		Value *specialRegisters = vctx.Module->getGlobalVariable("SpecialRegisters");
		for (RegistersMap::iterator itr = sregMap.begin(); itr != sregMap.end(); ++itr) {
			if (itr->second.changed) {
				Value* ix[2];
				ix[0] = builder.getInt32(0);
				ix[1] = builder.getInt32(itr->first);
				builder.CreateStore(
					itr->second.value,
					builder.CreatePointerCast(
					builder.CreateGEP(specialRegisters, ArrayRef<Value*>(ix, ix + 2)), Type::getInt64PtrTy(ctx)));
				if (markClean)
					itr->second.changed = false;
			}
		}
		Value *registers = vctx.Module->getGlobalVariable("Registers");
		for (RegistersMap::iterator itr = regMap.begin(); itr != regMap.end(); ++itr) {
			if (itr->second.changed) {
				Value* ix[2];
				ix[0] = builder.getInt32(0);
				ix[1] = builder.getInt32(itr->first);
				builder.CreateStore(
					itr->second.value,
					builder.CreatePointerCast(
					builder.CreateGEP(registers, ArrayRef<Value*>(ix, ix + 2)), Type::getInt64PtrTy(ctx)));
				if (markClean)
					itr->second.changed = false;
			}
		}
	}

	void emitLeaveVerticeImpl(VerticeContext& vctx, llvm::IRBuilder<>& builder,
		RegistersMap& regMap, RegistersMap& sregMap, MXOcta target)
	{
		saveRegisters(vctx, builder, regMap, sregMap, false);
		MmixLlvm::EdgeList& edgeList = *vctx.Edges;
		edgeList.push_back(target);
		Function::ArgumentListType::iterator argItr = vctx.Function->arg_begin();
		builder.CreateStore(builder.getInt64(vctx.XPtr), argItr);
		builder.CreateStore(builder.getInt64(target), ++argItr);
		builder.CreateRetVoid();
	}

	void emitLeaveVerticeViaTrapImpl(VerticeContext& vctx, llvm::IRBuilder<>& builder,
		RegistersMap& regMap, RegistersMap& sregMap)
	{
		Value* trapVector = emitSpecialRegisterLoad(vctx, builder, MmixLlvm::rT);
		saveRegisters(vctx, builder, regMap, sregMap, false);
		Function::ArgumentListType::iterator argItr = vctx.Function->arg_begin();
		builder.CreateStore(builder.getInt64(vctx.XPtr), argItr);
		builder.CreateStore(trapVector, ++argItr);
		builder.CreateRetVoid();
	}
}

void MmixLlvm::Private::flushRegistersCache(VerticeContext& vctx, llvm::IRBuilder<>& builder) {
	saveRegisters(vctx, builder, vctx.RegMap, vctx.SpecialRegMap, true);
}

void MmixLlvm::Private::emitLeaveVerticeViaTrip(VerticeContext& vctx, llvm::IRBuilder<>& builder,
	llvm::Value* rY, llvm::Value* rZ, MXOcta target)
{
	RegistersMap regMap(vctx.RegMap);
	RegistersMap sregMap(vctx.SpecialRegMap);
	RegisterRecord r0;
	r0.changed = true;
	r0.value = builder.getInt64(vctx.XPtr + 4);
	sregMap[MmixLlvm::rW] = r0;
	r0.value = emitRegisterLoadImpl(vctx, builder, 255, false);
	sregMap[MmixLlvm::rB] = r0;
	r0.value = emitSpecialRegisterLoadImpl(vctx, builder, MmixLlvm::rJ, false);
	regMap[255] = r0;
	r0.value = builder.CreateOr(builder.CreateShl(builder.getInt64(1), 63), builder.getInt64(vctx.Instr));
	sregMap[MmixLlvm::rX] = r0;
	r0.value = rY;
	sregMap[MmixLlvm::rY] = r0;
	r0.value = rZ;
	sregMap[MmixLlvm::rZ] = r0;
	emitLeaveVerticeImpl(vctx, builder, regMap, sregMap, target);
}

void MmixLlvm::Private::emitLeaveVerticeViaTrap(VerticeContext& vctx, IRBuilder<>& builder,
	Value* rY, Value* rZ)
{
	RegistersMap regMap(vctx.RegMap);
	RegistersMap sregMap(vctx.SpecialRegMap);
	RegisterRecord r0;
	r0.changed = true;
	r0.value = builder.getInt64(vctx.XPtr + 4);
	sregMap[MmixLlvm::rWW] = r0;
	r0.value = emitRegisterLoadImpl(vctx, builder, 255, false);
	sregMap[MmixLlvm::rBB] = r0;
	r0.value = emitSpecialRegisterLoadImpl(vctx, builder, MmixLlvm::rJ, false);
	regMap[255] = r0;
	r0.value = builder.CreateOr(builder.CreateShl(builder.getInt64(1), 63), builder.getInt64(vctx.Instr));
	sregMap[MmixLlvm::rXX] = r0;
	r0.value = rY;
	sregMap[MmixLlvm::rYY] = r0;
	r0.value = rZ;
	sregMap[MmixLlvm::rZZ] = r0;
	emitLeaveVerticeViaTrapImpl(vctx, builder, regMap, sregMap);
}

void MmixLlvm::Private::emitLeaveVerticeViaJump(VerticeContext& vctx, IRBuilder<>& builder, MXOcta target) 
{
	emitLeaveVerticeImpl(vctx, builder, vctx.RegMap, vctx.SpecialRegMap, target);
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
	builder.CreateCall(vctx.Module->getFunction("DebugInt64"), ArrayRef<Value*>(callParams, callParams + 1));
}