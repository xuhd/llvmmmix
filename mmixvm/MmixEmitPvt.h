#pragma once

#include <stdint.h>
#include <llvm/IR/IRBuilder.h>
#include <boost/unordered_map.hpp>
#include "MmixDef.h"

namespace MmixLlvm {
	namespace Private {
		struct RegisterRecord {
			llvm::Value* value;

			bool changed;
		};

		typedef boost::unordered_map<uint8_t, RegisterRecord> RegistersMap;

		struct VerticeContext {
			uint32_t Instr;

			uint64_t XPtr;

			llvm::LLVMContext* Ctx;

			llvm::Module* Module;

			llvm::Function* Function;

			llvm::BasicBlock *Entry;

			llvm::BasicBlock *Exit;

			RegistersMap* RegMap;

			RegistersMap* SpecialRegMap;
		};

		extern llvm::Value* emitAdjust64Endianness(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* val);

		extern llvm::Value* emitAdjust32Endianness(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* val);

		extern llvm::Value* emitAdjust16Endianness(VerticeContext& vctx,llvm::IRBuilder<>& builder, llvm::Value* val);

		extern llvm::Value* emitRegisterLoad(VerticeContext& vctx, llvm::IRBuilder<>& builder, uint8_t reg);

		extern llvm::Value* emitSpecialRegisterLoad(VerticeContext& vctx, llvm::IRBuilder<>& builder, MmixLlvm::SpecialReg sreg);

		extern llvm::Value* emitQueryArithFlag(VerticeContext& vctx, llvm::IRBuilder<>& builder, MmixLlvm::ArithFlag flag);

		extern llvm::Value* emitFetchMem(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::IRBuilder<>& builder, llvm::Value* theA, llvm::Type* ty);

		extern void emitStoreMem(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::IRBuilder<>& builder, llvm::Value* theA, llvm::Value* val);

		extern uint64_t getArithTripVector(MmixLlvm::ArithFlag flag);

		//extern void emitLeaveVertice(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* target);

		extern void addRegisterToCache(VerticeContext& vctx, uint8_t reg, llvm::Value* val, bool markDirty);

		extern void addSpecialRegisterToCache(VerticeContext& vctx, uint8_t reg, llvm::Value* val, bool markDirty);

		extern void emitLeaveVerticeViaTrip(VerticeContext& vctx, llvm::IRBuilder<>& builder,
			llvm::Value* rY, llvm::Value* rZ, llvm::Value* target);

		extern void emitLeaveVerticeViaTrap(VerticeContext& vctx, llvm::IRBuilder<>& builder,
			llvm::Value* rY, llvm::Value* rZ, llvm::Value* target);

		extern void emitLdo(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitLdt(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned);

		extern void emitLdw(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned);

		extern void emitLdb(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned);

		extern void emitLdoi(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitLdti(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned);

		extern void emitLdwi(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned);

		extern void emitLdbi(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned);

		extern void emitLdht(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitLdhti(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitSto(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitStoi(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitStt(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitStti(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitStw(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitStwi(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitStb(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitStbi(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitSttu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitStwu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitStbu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitSttui(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitStwui(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitStbui(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitStht(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitSthti(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitStco(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitStcoi(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitAdd(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitAddu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emit2Addu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emit4Addu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emit8Addu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emit16Addu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitSub(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitSubu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitMul(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitDiv(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitMulu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitDivu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitNeg(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitNegu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitSr(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitSru(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitSl(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitSlu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitCmp(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitCmpu(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitCsn(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
		
		extern void emitCsz(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
		
		extern void emitCsp(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
		
		extern void emitCsod(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
		
		extern void emitCsnn(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitCsnz(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitCsnp(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitCsev(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitZsn(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
		
		extern void emitZsz(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
		
		extern void emitZsp(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
		
		extern void emitZsod(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);
		
		extern void emitZsnn(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitZsnz(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitZsnp(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitZsev(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitAnd(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitOr(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitXor(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitAndn(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitOrn(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitNand(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitNor(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitNxor(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitMux(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitSadd(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool immediate);

		extern void emitSeth(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitSetmh(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitSetml(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitSetl(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitInch(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitIncmh(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitIncml(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitIncl(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitOrh(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitOrmh(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitOrml(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitOrl(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitAndnh(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitAndnmh(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitAndnml(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);
		
		extern void emitAndnl(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg);

		extern void emitTrip(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitTrap(VerticeContext& vctx, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern void emitGeta(VerticeContext& vctx, uint8_t xarg, uint16_t yzarg, bool backward);

		extern void debugInt64(VerticeContext& vctx, llvm::IRBuilder<>& builder, llvm::Value* val);
	};
};