#pragma once

#include <stdint.h>
#include <llvm/IR/IRBuilder.h>
#include <boost/unordered_map.hpp>
#include "MmixEmit.h"
#include "MmixDef.h"

namespace MmixLlvm {
	namespace Private {
		struct RegisterRecord {
			llvm::Value* value;

			bool changed;
		};

		typedef boost::unordered_map<MXByte, RegisterRecord> RegistersMap;

		struct VerticeContext {
			MXTetra Instr;

			MXOcta XPtr;

			llvm::LLVMContext* Ctx;

			llvm::Module* Module;

			llvm::Function* Function;

			llvm::BasicBlock *Entry;

			llvm::BasicBlock *Exit;

			MmixLlvm::EdgeList* Edges;

			RegistersMap RegMap;

			RegistersMap SpecialRegMap;
		};

		extern llvm::Value* emitAdjust64Endianness(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* val);

		extern llvm::Value* emitAdjust32Endianness(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* val);

		extern llvm::Value* emitAdjust16Endianness(VerticeContext& vctx,llvm::IRBuilder<>& builder, llvm::Value* val);

		extern llvm::Value* emitRegisterLoad(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXByte reg);

		extern llvm::Value* emitSpecialRegisterLoad(VerticeContext& vctx, llvm::IRBuilder<>& builder, MmixLlvm::SpecialReg sreg);

		extern llvm::Value* emitQueryArithFlag(VerticeContext& vctx, llvm::IRBuilder<>& builder, MmixLlvm::ArithFlag flag);

		extern llvm::Value* emitFetchMem(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::IRBuilder<>& builder, llvm::Value* theA, llvm::Type* ty);

		extern void emitStoreMem(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::IRBuilder<>& builder, llvm::Value* theA, llvm::Value* val);

		extern MXOcta getArithTripVector(MmixLlvm::ArithFlag flag);

		extern void addRegisterToCache(VerticeContext& vctx, MXByte reg, llvm::Value* val, bool markDirty);

		extern void addSpecialRegisterToCache(VerticeContext& vctx, MXByte reg, llvm::Value* val, bool markDirty);

		extern void emitLeaveVerticeViaTrip(VerticeContext& vctx, llvm::IRBuilder<>& builder,
			llvm::Value* rY, llvm::Value* rZ, MXOcta target);

		extern void emitLeaveVerticeViaTrap(VerticeContext& vctx, llvm::IRBuilder<>& builder,
			llvm::Value* rY, llvm::Value* rZ);

		extern void emitLeaveVerticeViaJump(VerticeContext& vctx, llvm::IRBuilder<>& builder, MXOcta target);

		extern void emitLdo(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitLdt(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned);

		extern void emitLdw(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned);

		extern void emitLdb(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned);

		extern void emitLdoi(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitLdti(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned);

		extern void emitLdwi(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned);

		extern void emitLdbi(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool isSigned);

		extern void emitLdht(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitLdhti(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitGet(VerticeContext& vctx, MXByte xarg, MXByte zarg);
		
		extern void emitPut(VerticeContext& vctx, MXByte xarg, MXByte zarg, bool immediate);

		extern void emitSto(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStoi(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStt(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStti(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStw(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStwi(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStb(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStbi(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitSttu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStwu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStbu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitSttui(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStwui(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStbui(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStht(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitSthti(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStco(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitStcoi(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitAdd(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitAddu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emit2Addu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emit4Addu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emit8Addu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emit16Addu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSub(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSubu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitMul(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitDiv(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitMulu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitDivu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitNeg(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitNegu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSr(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSru(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSl(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSlu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitCmp(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitCmpu(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitCsn(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitCsz(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitCsp(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitCsod(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitCsnn(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitCsnz(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitCsnp(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitCsev(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitZsn(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitZsz(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitZsp(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitZsod(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		
		extern void emitZsnn(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitZsnz(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitZsnp(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitZsev(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitAnd(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitOr(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitXor(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitAndn(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitOrn(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitNand(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitNor(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitNxor(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitMux(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSadd(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);

		extern void emitSeth(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitSetmh(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitSetml(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitSetl(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitInch(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitIncmh(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitIncml(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitIncl(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitOrh(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitOrmh(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitOrml(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitOrl(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitAndnh(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitAndnmh(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitAndnml(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);
		
		extern void emitAndnl(VerticeContext& vctx, MXByte xarg, MXWyde yzarg);

		extern void emitTrip(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitTrap(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg);

		extern void emitGeta(VerticeContext& vctx, MXByte xarg, MXWyde yzarg, bool backward);

		extern void emitJmp(VerticeContext& vctx, MXTetra xyzarg, bool backward);

		extern void emitBn(VerticeContext& vctx, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitBz(VerticeContext& vctx, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitBp(VerticeContext& vctx, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitBod(VerticeContext& vctx, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitBnn(VerticeContext& vctx, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitBnz(VerticeContext& vctx, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitBnp(VerticeContext& vctx, MXByte xarg, MXTetra yzarg, bool backward);

		extern void emitBev(VerticeContext& vctx, MXByte xarg, MXTetra yzarg, bool backward);

		extern void debugInt64(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* val);
	};
};