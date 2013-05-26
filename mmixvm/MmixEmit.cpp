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
using llvm::cast;

using namespace MmixLlvm::Util;
using namespace MmixLlvm::Private;
using MmixLlvm::EdgeList;
using MmixLlvm::MemAccessor;

namespace {
	void emitEpilogue(LLVMContext& ctx, Module& m, Function& f, BasicBlock& epilogue, RegistersMap& regmap) {
		RegistersMap::iterator itr;
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(&epilogue);
		for (itr = regmap.begin(); itr != regmap.end(); ++itr) {
			if ((*itr).second.changed) {
				Value *registers = m.getGlobalVariable("Registers");
				Value* ix[2];
				ix[0] = builder.getInt32(0);
				ix[1] = builder.getInt32((*itr).first);
				builder.CreateStore(
					(*itr).second.value,
					builder.CreatePointerCast(
						builder.CreateGEP(registers, ArrayRef<Value*>(ix, ix + 2)), Type::getInt64PtrTy(ctx)));
			}
		}
		builder.CreateRetVoid();
	}

	bool isTerm(uint32_t instr) {
		return instr == 0; // todo
	}

	BasicBlock* emitInstruction(llvm::LLVMContext& ctx, llvm::Module& m, Function& f,
		uint32_t instr, llvm::BasicBlock *cp, uint64_t xPtr, RegistersMap& regMap)
	{
		BasicBlock* retVal = 0;
		uint8_t o0 = (uint8_t) (instr >> 24);
		uint8_t xarg = (uint8_t) ((instr >> 16) & 0xff);
		uint8_t yarg = (uint8_t) ((instr >> 8) & 0xff);
		uint8_t zarg = (uint8_t) (instr & 0xff);
		switch(o0) {
		case MmixLlvm::LDO:
			retVal = emitLdo(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::LDOI:
			retVal = emitLdoi(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::LDT:
			retVal = emitLdt(ctx, m, f, cp, regMap, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::LDTI:
			retVal = emitLdti(ctx, m, f, cp, regMap, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::LDW:
			retVal = emitLdw(ctx, m, f, cp, regMap, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::LDWI:
			retVal = emitLdwi(ctx, m, f, cp, regMap, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::LDB:
			retVal = emitLdb(ctx, m, f, cp, regMap, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::LDBI:
			retVal = emitLdbi(ctx, m, f, cp, regMap, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::LDOU:
			retVal = emitLdo(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::LDOUI:
			retVal = emitLdoi(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::LDTU:
			retVal = emitLdt(ctx, m, f, cp, regMap, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::LDTUI:
			retVal = emitLdti(ctx, m, f, cp, regMap, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::LDWU:
			retVal = emitLdw(ctx, m, f, cp, regMap, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::LDWUI:
			retVal = emitLdwi(ctx, m, f, cp, regMap, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::LDBU:
			retVal = emitLdb(ctx, m, f, cp, regMap, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::LDBUI:
			retVal = emitLdbi(ctx, m, f, cp, regMap, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::LDHT:
			retVal = emitLdht(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::LDHTI:
			retVal = emitLdhti(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STO:
			retVal = emitSto(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STOI:
			retVal = emitStoi(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STT:
			retVal = emitStt(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STTI:
			retVal = emitStti(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STW:
			retVal = emitStw(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STWI:
			retVal = emitStwi(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STB:
			retVal = emitStb(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STBI:
			retVal = emitStbi(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STOU:
			retVal = emitSto(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STTU:
			retVal = emitSttu(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STWU:
			retVal = emitStwu(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STBU:
			retVal = emitStbu(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STOUI:
			retVal = emitStoi(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STTUI:
			retVal = emitSttui(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STWUI:
			retVal = emitStwui(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STBUI:
			retVal = emitStbui(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STHT:
			retVal = emitStht(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STHTI:
			retVal = emitSthti(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STCO:
			retVal = emitStco(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		case MmixLlvm::STCOI:
			retVal = emitStcoi(ctx, m, f, cp, regMap, xarg, yarg, zarg);
			break;
		default:
			assert(0 && "Not implemented");
		}
		return retVal;
	}
}

boost::tuple<Function*, EdgeList> MmixLlvm::emitSimpleVertice(LLVMContext& ctx, Module& m, MemAccessor& ma, uint64_t xPtr)
{
	RegistersMap regMap;
	uint64_t xPtr0 = xPtr;
	Function* f = cast<Function>(m.getOrInsertFunction(genUniq("fun").str(), Type::getVoidTy(ctx) , (Type *)0));
	llvm::BasicBlock *connectionPoint = 0;
	for (;;) {
		uint32_t instr = ma.readTetra(xPtr0);
		if (isTerm(instr))
			break;
		connectionPoint = emitInstruction(ctx, m, *f, instr, connectionPoint, xPtr, regMap);
		xPtr0 += sizeof(uint32_t);
	}
	if (connectionPoint)
		emitEpilogue(ctx, m, *f, *connectionPoint, regMap);
	return boost::make_tuple(f, EdgeList());
}
