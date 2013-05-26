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
using llvm::Twine;

using namespace MmixLlvm::Util;
using namespace MmixLlvm::Private;
using MmixLlvm::EdgeList;
using MmixLlvm::MemAccessor;

namespace {
	bool isTerm(uint32_t instr) {
		return instr == 0; // todo
	}

	void emitInstruction(VerticeContext& vctx)
	{
		uint8_t o0 = (uint8_t) (vctx.Instr >> 24);
		uint8_t xarg = (uint8_t) ((vctx.Instr >> 16) & 0xff);
		uint8_t yarg = (uint8_t) ((vctx.Instr >> 8) & 0xff);
		uint8_t zarg = (uint8_t) (vctx.Instr & 0xff);
		switch(o0) {
		case MmixLlvm::LDO:
			emitLdo(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::LDOI:
			emitLdoi(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::LDT:
			emitLdt(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::LDTI:
			emitLdti(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::LDW:
			emitLdw(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::LDWI:
			emitLdwi(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::LDB:
			emitLdb(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::LDBI:
			emitLdbi(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::LDOU:
			emitLdo(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::LDOUI:
			emitLdoi(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::LDTU:
			emitLdt(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::LDTUI:
			emitLdti(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::LDWU:
			emitLdw(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::LDWUI:
			emitLdwi(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::LDBU:
			emitLdb(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::LDBUI:
			emitLdbi(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::LDHT:
			emitLdht(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::LDHTI:
			emitLdhti(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STO:
			emitSto(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STOI:
			emitStoi(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STT:
			emitStt(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STTI:
			emitStti(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STW:
			emitStw(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STWI:
			emitStwi(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STB:
			emitStb(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STBI:
			emitStbi(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STOU:
			emitSto(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STTU:
			emitSttu(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STWU:
			emitStwu(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STBU:
			emitStbu(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STOUI:
			emitStoi(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STTUI:
			emitSttui(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STWUI:
			emitStwui(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STBUI:
			emitStbui(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STHT:
			emitStht(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STHTI:
			emitSthti(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STCO:
			emitStco(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::STCOI:
			emitStcoi(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::ADD:
			emitAdd(vctx, xarg, yarg, zarg);
			break;
		default:
			assert(0 && "Not implemented");
		}
	}

	void emitEpilogue(VerticeContext& vctx) {
		LLVMContext& ctx = *vctx.Ctx;
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(vctx.Exit);
		Value *registers = (*vctx.Module).getGlobalVariable("Registers");
		RegistersMap& regMap = *vctx.RegMap;
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
		builder.CreateRetVoid();
	}
}

namespace {
	Twine getInstrTwine(std::vector<std::string>& twines, uint32_t instr, uint64_t xptr)
	{
		std::ostringstream oss;
		oss<<"instr_"<<std::hex<<instr<<"@"<<xptr<<"#";
		twines.push_back(oss.str());
		return genUniq(Twine(twines.back()));
	}
};

boost::tuple<Function*, EdgeList> MmixLlvm::emitSimpleVertice(LLVMContext& ctx, Module& m, MemAccessor& ma, uint64_t xPtr)
{
	std::vector<std::string> twines;
	RegistersMap regMap;
	uint64_t xPtr0 = xPtr;
	Function* f = cast<Function>(m.getOrInsertFunction(genUniq("fun").str(), Type::getVoidTy(ctx) , (Type *)0));
	BasicBlock *verticeEntry = BasicBlock::Create(ctx, genUniq("entry") + Twine(xPtr), f);
	std::vector<VerticeContext> oenv;
	BasicBlock *block = verticeEntry;
	for (;;) {
		uint32_t instr = ma.readTetra(xPtr0);
		if (isTerm(instr))
			break;
		VerticeContext e;
		e.Ctx = &ctx;
		e.Function = f;
		e.Module = &m;
		e.RegMap = &regMap;
		e.XPtr = xPtr0;
		e.Instr = instr;
		e.Entry = block;
		if (!oenv.empty())
			oenv.back().Exit = block;
		oenv.push_back(e);
		block = BasicBlock::Create(ctx, getInstrTwine(twines, e.Instr, e.XPtr), f);
		xPtr0 += sizeof(uint32_t);
	}
	if (!oenv.empty())
		oenv.back().Exit = BasicBlock::Create(ctx, genUniq("epilogue") + Twine(xPtr), f);
	
	for (std::vector<VerticeContext>::iterator itr = oenv.begin(); itr != oenv.end(); ++itr)
		emitInstruction(*itr);

	if (!oenv.empty())
		emitEpilogue(oenv.back());
	return boost::make_tuple(f, EdgeList());
}
