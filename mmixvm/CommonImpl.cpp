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
using llvm::PHINode;
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
using MmixLlvm::MXByte;
using MmixLlvm::MXTetra;
using MmixLlvm::MXOcta;

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
		std::vector<MXByte> dirtyRegs(vctx.getDirtyRegisters());
		for (auto itr = dirtyRegs.begin(); itr != dirtyRegs.end(); ++itr) {
			Value* regGlob = builder.CreateSelect(
				builder.CreateICmpULT(builder.getInt64(*itr), 
					vctx.getSpRegister(MmixLlvm::rG)),
						vctx.getModuleVar("RegisterStackTop"),
						vctx.getModuleVar("RegisterStackBase"));
			builder.CreateStore(
				vctx.getRegister(*itr),
					builder.CreateGEP(
						builder.CreateLoad(regGlob, false),
						builder.getInt32(*itr)));
		}
		Value* specialRegisters = vctx.getModuleVar("SpecialRegisters");
		std::vector<MmixLlvm::SpecialReg> dirtySRegs(vctx.getDirtySpRegisters());
		for (auto itr = dirtySRegs.begin(); itr != dirtySRegs.end(); ++itr) {
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

void MmixLlvm::Private::assignRegister(VerticeContext& vctx, IRBuilder<>& builder, MXByte reg, Value* value) {
	Value* rG = vctx.getSpRegister(MmixLlvm::rG);
	Value* rL = vctx.getSpRegister(MmixLlvm::rL);
	Value* reg0 = builder.getInt64(reg);
	Value* cond = builder.CreateAnd(builder.CreateICmpUGE(reg0, rL), builder.CreateICmpULT(reg0, rG));
	vctx.assignSpRegister(MmixLlvm::rL, builder.CreateSelect(cond, builder.CreateAdd(reg0, builder.getInt64(1LL)), rL));
	vctx.assignRegister(reg, value);
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

void MmixLlvm::Private::emitPushRegsAndLeaveVerticeViaJump(VerticeContext&vctx,
	MXByte xarg, IRBuilder<>& builder, MXOcta target)
{
	LLVMContext& ctx = vctx.getLctx();
	Value* rLVal = vctx.getSpRegister(MmixLlvm::rL);
	Value* k = builder.getInt64(xarg);
	saveRegisters(vctx, builder);
	Value* callParams[] = {
		builder.CreateLoad(vctx.getModuleVar("ThisRef")),
		builder.CreateAdd(k, builder.getInt64(1)),
		vctx.getSpRegister(MmixLlvm::rL)
	};
	builder.CreateCall(vctx.getModuleFunction("PushRegStack"), ArrayRef<Value*>(callParams, callParams + 3));
	Value* newRlVal = builder.CreateSelect(
		builder.CreateICmpULT(k, rLVal),
		builder.CreateSub(rLVal, builder.CreateAdd(k, builder.getInt64(1))),
		builder.CreateAdd(k, builder.getInt64(1)));
	Value* specialRegisters = vctx.getModuleVar("SpecialRegisters");
	Value* ix[2];
	ix[0] = builder.getInt32(0);
	ix[1] = builder.getInt32((int)MmixLlvm::rL);
	builder.CreateStore(
		newRlVal,
		builder.CreatePointerCast(
			builder.CreateGEP(specialRegisters, ArrayRef<Value*>(ix, ix + 2)), Type::getInt64PtrTy(ctx)));
	std::vector<Argument*> args(vctx.getVerticeArgs());
	builder.CreateStore(builder.getInt64(vctx.getXPtr()), args[0]);
	builder.CreateStore(builder.getInt64(target), args[1]);
	builder.CreateRetVoid();
}

void MmixLlvm::Private::emitPushRegsAndLeaveVerticeViaIndirectJump(VerticeContext&vctx, 
	MXByte xarg, IRBuilder<>& builder, Value* target)
{
	LLVMContext& ctx = vctx.getLctx();
	Value* rLVal = vctx.getSpRegister(MmixLlvm::rL);
	Value* k = builder.getInt64(xarg);
	saveRegisters(vctx, builder);
	Value* callParams[] = {
		builder.CreateLoad(vctx.getModuleVar("ThisRef")),
		builder.CreateAdd(k, builder.getInt64(1)),
		vctx.getSpRegister(MmixLlvm::rL)
	};
	builder.CreateCall(vctx.getModuleFunction("PushRegStack"), ArrayRef<Value*>(callParams, callParams + 3));
	Value* newRlVal = builder.CreateSelect(
		builder.CreateICmpULT(k, rLVal),
		builder.CreateSub(rLVal, builder.CreateAdd(k, builder.getInt64(1))),
		builder.CreateAdd(k, builder.getInt64(1)));
	Value* specialRegisters = vctx.getModuleVar("SpecialRegisters");
	Value* ix[2];
	ix[0] = builder.getInt32(0);
	ix[1] = builder.getInt32((int)MmixLlvm::rL);
	builder.CreateStore(
		newRlVal,
		builder.CreatePointerCast(
			builder.CreateGEP(specialRegisters, ArrayRef<Value*>(ix, ix + 2)), Type::getInt64PtrTy(ctx)));
	std::vector<Argument*> args(vctx.getVerticeArgs());
	builder.CreateStore(builder.getInt64(vctx.getXPtr()), args[0]);
	builder.CreateStore(target, args[1]);
	builder.CreateRetVoid();
}

void MmixLlvm::Private::emitLeaveVerticeViaPop(VerticeContext& vctx, IRBuilder<>& builder, 
	Value* retainLocalRegs, Value* target) 
{
	LLVMContext& ctx = vctx.getLctx();
	saveRegisters(vctx, builder);
	Value* newRlVal = builder.CreateAlloca(builder.getInt64Ty());
	Value* callParams[] = {
		builder.CreateLoad(vctx.getModuleVar("ThisRef")),
		retainLocalRegs,
		newRlVal
	};
	Value* specialRegisters = vctx.getModuleVar("SpecialRegisters");
	builder.CreateCall(vctx.getModuleFunction("PopRegStack"), ArrayRef<Value*>(callParams, callParams + 3));
	std::vector<Argument*> args(vctx.getVerticeArgs());
	Value* ix[2];
	ix[0] = builder.getInt32(0);
	ix[1] = builder.getInt32((int)MmixLlvm::rL);
	builder.CreateStore(
		builder.CreateLoad(newRlVal, false),
		builder.CreatePointerCast(
			builder.CreateGEP(specialRegisters, ArrayRef<Value*>(ix, ix + 2)), Type::getInt64PtrTy(ctx)));
	builder.CreateStore(builder.getInt64(vctx.getXPtr()), args[0]);
	builder.CreateStore(target, args[1]);
	builder.CreateRetVoid();
}

void MmixLlvm::Private::emitLeaveVerticeViaIndirectJump(VerticeContext& vctx, IRBuilder<>& builder, Value* target) 
{
	saveRegisters(vctx, builder);
	std::vector<Argument*> args(vctx.getVerticeArgs());
	builder.CreateStore(builder.getInt64(vctx.getXPtr()), args[0]);
	builder.CreateStore(target, args[1]);
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

MXOcta MmixLlvm::Private::getArithTripVector(ArithFlag flag) {
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

void MmixLlvm::Private::debugInt64(VerticeContext& vctx, IRBuilder<>& builder, Value* val) {
	Value* callParams[] = { val };
	builder.CreateCall(vctx.getModuleFunction("DebugInt64"), ArrayRef<Value*>(callParams, callParams + 1));
}

void MmixLlvm::Private::emitInstruction(VerticeContext& vctx, IRBuilder<>& builder) {
	MXTetra instr = vctx.getInstr();
	MXByte o0 = (MXByte) (instr >> 24);
	MXByte xarg = (MXByte) ((instr >> 16) & 0xff);
	MXByte yarg = (MXByte) ((instr >> 8) & 0xff);
	MXByte zarg = (MXByte) (instr & 0xff);
	switch(o0) {
	case MmixLlvm::LDO:
		emitLdo(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::LDOI:
		emitLdoi(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::LDT:
		emitLdt(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::LDTI:
		emitLdti(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::LDW:
		emitLdw(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::LDWI:
		emitLdwi(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::LDB:
		emitLdb(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::LDBI:
		emitLdbi(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::LDOU:
		emitLdo(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::LDOUI:
		emitLdoi(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::LDTU:
		emitLdt(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::LDTUI:
		emitLdti(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::LDWU:
		emitLdw(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::LDWUI:
		emitLdwi(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::LDBU:
		emitLdb(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::LDBUI:
		emitLdbi(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::LDHT:
		emitLdht(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::LDHTI:
		emitLdhti(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::GET:
		emitGet(vctx, builder, xarg, zarg);
		break;
	case MmixLlvm::PUT:
		emitPut(vctx, builder, xarg, zarg, false);
		break;
	case MmixLlvm::PUTI:
		emitPut(vctx, builder, xarg, zarg, true);
		break;
	case MmixLlvm::STO:
		emitSto(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STOI:
		emitStoi(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STT:
		emitStt(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STTI:
		emitStti(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STW:
		emitStw(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STWI:
		emitStwi(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STB:
		emitStb(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STBI:
		emitStbi(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STOU:
		emitSto(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STTU:
		emitSttu(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STWU:
		emitStwu(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STBU:
		emitStbu(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STOUI:
		emitStoi(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STTUI:
		emitSttui(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STWUI:
		emitStwui(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STBUI:
		emitStbui(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STHT:
		emitStht(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STHTI:
		emitSthti(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STCO:
		emitStco(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::STCOI:
		emitStcoi(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::ADD:
		emitAdd(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::ADDI:
		emitAdd(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::ADDU:
		emitAddu(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::ADDUI:
		emitAddu(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::_2ADDU:
		emit2Addu(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::_2ADDUI:
		emit2Addu(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::_4ADDU:
		emit4Addu(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::_4ADDUI:
		emit4Addu(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::_8ADDU:
		emit8Addu(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::_8ADDUI:
		emit8Addu(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::_16ADDU:
		emit16Addu(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::_16ADDUI:
		emit16Addu(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::SUB:
		emitSub(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::SUBI:
		emitSub(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::SUBU:
		emitSubu(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::SUBUI:
		emitSubu(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::MUL:
		emitMul(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::MULI:
		emitMul(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::DIV:
		emitDiv(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::DIVI:
		emitDiv(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::MULU:
		emitMulu(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::MULUI:
		emitMulu(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::DIVU:
		emitDivu(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::DIVUI:
		emitDivu(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::NEG:
		emitNeg(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::NEGI:
		emitNeg(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::NEGU:
		emitNegu(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::NEGUI:
		emitNegu(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::SL:
		emitSl(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::SLI:
		emitSl(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::SR:
		emitSr(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::SRI:
		emitSr(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::SLU:
		emitSlu(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::SLUI:
		emitSlu(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::SRU:
		emitSru(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::SRUI:
		emitSru(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::CMP:
		emitCmp(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::CMPI:
		emitCmp(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::CMPU:
		emitCmpu(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::CMPUI:
		emitCmpu(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::CSN:
		emitCsn(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::CSNI:
		emitCsn(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::CSZ:
		emitCsz(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::CSZI:
		emitCsz(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::CSP:
		emitCsp(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::CSPI:
		emitCsp(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::CSOD:
		emitCsod(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::CSODI:
		emitCsod(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::CSNN:
		emitCsnn(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::CSNNI:
		emitCsnn(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::CSNZ:
		emitCsnz(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::CSNZI:
		emitCsnz(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::CSNP:
		emitCsnp(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::CSNPI:
		emitCsnp(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::CSEV:
		emitCsev(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::CSEVI:
		emitCsev(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::ZSN:
		emitZsn(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::ZSNI:
		emitZsn(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::ZSZ:
		emitZsz(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::ZSZI:
		emitZsz(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::ZSP:
		emitZsp(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::ZSPI:
		emitZsp(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::ZSOD:
		emitZsod(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::ZSODI:
		emitZsod(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::ZSNN:
		emitZsnn(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::ZSNNI:
		emitZsnn(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::ZSNZ:
		emitZsnz(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::ZSNZI:
		emitZsnz(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::ZSNP:
		emitZsnp(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::ZSNPI:
		emitZsnp(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::ZSEV:
		emitZsev(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::ZSEVI:
		emitZsev(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::AND:
		emitAnd(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::ANDI:
		emitAnd(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::OR:
		emitOr(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::ORI:
		emitOr(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::XOR:
		emitXor(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::XORI:
		emitXor(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::ANDN:
		emitAndn(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::ANDNI:
		emitAndn(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::ORN:
		emitOrn(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::ORNI:
		emitOrn(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::NAND:
		emitNand(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::NANDI:
		emitNand(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::NOR:
		emitNor(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::NORI:
		emitNor(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::NXOR:
		emitNxor(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::NXORI:
		emitNxor(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::MUX:
		emitMux(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::MUXI:
		emitMux(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::SADD:
		emitSadd(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::SADDI:
		emitSadd(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::MOR:
		emitMor(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::MORI:
		emitMor(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::MXOR:
		emitMxor(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::MXORI:
		emitMxor(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::SETH:
		emitSeth(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::SETMH:
		emitSetmh(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::SETML:
		emitSetml(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::SETL:
		emitSetl(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::INCH:
		emitInch(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::INCMH:
		emitIncmh(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::INCML:
		emitIncml(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::INCL:
		emitIncl(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::ORH:
		emitOrh(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::ORMH:
		emitOrmh(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::ORML:
		emitOrml(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::ORL:
		emitOrl(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::ANDNH:
		emitAndnh(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::ANDNMH:
		emitAndnmh(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::ANDNML:
		emitAndnml(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::ANDNL:
		emitAndnl(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg);
		break;
	case MmixLlvm::TRIP:
		emitTrip(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::TRAP:
		emitTrap(vctx, builder, xarg, yarg, zarg);
		break;
	case MmixLlvm::GETA:
		emitGeta(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg, false);
		break;
	case MmixLlvm::GETAB:
		emitGeta(vctx, builder, xarg, ((MXWyde)yarg << 8) | zarg, true);
		break;
	case MmixLlvm::JMP:
		emitJmp(vctx, builder, ((MXTetra)xarg << 16) | ((MXTetra)yarg << 8) | zarg, false);
		break;
	case MmixLlvm::JMPB:
		emitJmp(vctx, builder, ((MXTetra)xarg << 16) | ((MXTetra)yarg << 8) | zarg, true);
		break;
	case MmixLlvm::BN:
		emitBn(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::BNB:
		emitBn(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::BZ:
		emitBz(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::BZB:
		emitBz(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::BP:
		emitBp(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::BPB:
		emitBp(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::BOD:
		emitBod(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::BODB:
		emitBod(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::BNN:
		emitBnn(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::BNNB:
		emitBnn(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::BNZ:
		emitBnz(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::BNZB:
		emitBnz(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::BNP:
		emitBnp(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::BNPB:
		emitBnp(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::BEV:
		emitBev(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::BEVB:
		emitBev(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::PBN:
		emitBn(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::PBNB:
		emitBn(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::PBZ:
		emitBz(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::PBZB:
		emitBz(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::PBP:
		emitBp(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::PBPB:
		emitBp(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::PBOD:
		emitBod(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::PBODB:
		emitBod(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::PBNN:
		emitBnn(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::PBNNB:
		emitBnn(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::PBNZ:
		emitBnz(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::PBNZB:
		emitBnz(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::PBNP:
		emitBnp(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::PBNPB:
		emitBnp(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::PBEV:
		emitBev(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::PBEVB:
		emitBev(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::GO:
		emitGo(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::GOI:
		emitGo(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::PUSHGO:
		emitPushgo(vctx, builder, xarg, yarg, zarg, false);
		break;
	case MmixLlvm::PUSHGOI:
		emitPushgo(vctx, builder, xarg, yarg, zarg, true);
		break;
	case MmixLlvm::PUSHJ:
		emitPushj(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, false);
		break;
	case MmixLlvm::PUSHJB:
		emitPushj(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg, true);
		break;
	case MmixLlvm::POP:
		emitPop(vctx, builder, xarg, ((MXWyde) yarg << 8) | zarg);
		break;
	default:
		assert(0 && "Not implemented");
	}
}
