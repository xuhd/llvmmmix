#pragma once

#include <stdint.h>
#include "MmixEmit.h"
#include "MmixDef.h"
#include "VerticeContext.h"

namespace MmixLlvm {
	namespace Private {
		extern llvm::Value* emitAdjust64Endianness(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* val);

		extern llvm::Value* emitAdjust32Endianness(VerticeContext& vctx,  llvm::IRBuilder<>& builder, llvm::Value* val);
		
		extern llvm::Value* emitAdjust16Endianness(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* val);

		extern llvm::Value* emitQueryArithFlag(VerticeContext& vctx, llvm::IRBuilder<>& builder, MmixLlvm::ArithFlag flag);

		extern llvm::Value* emitFetchMem(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* theA, llvm::Type* ty);

		extern void emitStoreMem(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* theA, llvm::Value* val);

		extern MXOcta getArithTripVector(MmixLlvm::ArithFlag flag);

		extern void emitLeaveVerticeViaTrip(VerticeContext& vctx, llvm::IRBuilder<>& builder,
			llvm::Value* rY, llvm::Value* rZ, MXOcta target);

		extern void assignRegister(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte reg, llvm::Value* value);

		extern void flushRegistersCache(VerticeContext& vctx, llvm::IRBuilder<>& builder);

		extern void emitInstruction(VerticeContext& vctx, llvm::IRBuilder<>& builder);

		extern void emitLeaveVerticeViaJump(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXOcta target);

		extern void emitLeaveVerticeViaIndirectJump(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* target);

		extern void emitPushRegsAndLeaveVerticeViaJump(VerticeContext&vctx, MXByte xarg, llvm::IRBuilder<>& builder, MXOcta target);

		extern void emitPushRegsAndLeaveVerticeViaIndirectJump(VerticeContext&vctx, MXByte xarg, llvm::IRBuilder<>& builder, llvm::Value* target);

		extern void emitLeaveVerticeViaPop(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* retainLocalRegs, llvm::Value* target);

		extern void emitLdo(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitLdt(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned);

		extern void emitLdw(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned);

		extern void emitLdb(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned);

		extern void emitLdoi(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitLdti(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned);

		extern void emitLdwi(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned);

		extern void emitLdbi(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned);

		extern void emitLdht(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitLdhti(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitGet(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte zarg);
		
		extern void emitPut(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte zarg, bool immediate);

		extern void emitSto(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStoi(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStt(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStti(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStw(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStwi(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStb(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStbi(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitSttu(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStwu(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStbu(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitSttui(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStwui(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStbui(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStht(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitSthti(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStco(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStcoi(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitAdd(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitAddu(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emit2Addu(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emit4Addu(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emit8Addu(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emit16Addu(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSub(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSubu(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitMul(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitDiv(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitMulu(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitDivu(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitNeg(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitNegu(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSr(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSru(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSl(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSlu(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitCmp(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitCmpu(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitCsn(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitCsz(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitCsp(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitCsod(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitCsnn(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitCsnz(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitCsnp(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitCsev(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitZsn(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitZsz(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitZsp(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitZsod(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitZsnn(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitZsnz(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitZsnp(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitZsev(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitAnd(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitOr(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitXor(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitAndn(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitOrn(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitNand(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitNor(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitNxor(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitMux(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSadd(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitMor(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitMxor(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSeth(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitSetmh(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitSetml(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitSetl(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitInch(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitIncmh(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitIncml(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitIncl(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitOrh(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitOrmh(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitOrml(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitOrl(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitAndnh(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitAndnmh(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitAndnml(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);
		
		extern void emitAndnl(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg);

		extern void emitTrip(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitTrap(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitGeta(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXWyde yzarg, bool backward);

		extern void emitJmp(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXTetra xyzarg, bool backward);

		extern void emitPushj(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitPop(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXTetra yzarg);

		extern void emitGo(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitPushgo(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitBn(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitBz(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitBp(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitBod(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitBnn(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitBnz(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitBnp(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitBev(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte xarg, MXTetra yzarg, bool backward);

		extern void debugInt64(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* val);
	};
};