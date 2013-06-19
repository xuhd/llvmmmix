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
using MmixLlvm::MXByte;
using MmixLlvm::MXWyde;
using MmixLlvm::MXTetra;
using MmixLlvm::MXOcta;

namespace {
	bool isTerm(MXTetra instr) {
		MXByte o0 = (MXByte) (instr >> 24);
		switch(o0) {
		case MmixLlvm::JMP:
		case MmixLlvm::JMPB:
		case MmixLlvm::TRIP:
		case MmixLlvm::TRAP:
			return true;
		default:
			return false;
		}
	}

	void emitInstruction(VerticeContext& vctx)
	{
		MXByte o0 = (MXByte) (vctx.Instr >> 24);
		MXByte xarg = (MXByte) ((vctx.Instr >> 16) & 0xff);
		MXByte yarg = (MXByte) ((vctx.Instr >> 8) & 0xff);
		MXByte zarg = (MXByte) (vctx.Instr & 0xff);
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
		case MmixLlvm::GET:
			emitGet(vctx, xarg, zarg);
			break;
		case MmixLlvm::PUT:
			emitPut(vctx, xarg, zarg, false);
			break;
		case MmixLlvm::PUTI:
			emitPut(vctx, xarg, zarg, true);
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
			emitAdd(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::ADDI:
			emitAdd(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::ADDU:
			emitAddu(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::ADDUI:
			emitAddu(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::_2ADDU:
			emit2Addu(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::_2ADDUI:
			emit2Addu(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::_4ADDU:
			emit4Addu(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::_4ADDUI:
			emit4Addu(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::_8ADDU:
			emit8Addu(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::_8ADDUI:
			emit8Addu(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::_16ADDU:
			emit16Addu(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::_16ADDUI:
			emit16Addu(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::SUB:
			emitSub(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::SUBI:
			emitSub(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::SUBU:
			emitSubu(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::SUBUI:
			emitSubu(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::MUL:
			emitMul(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::MULI:
			emitMul(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::DIV:
			emitDiv(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::DIVI:
			emitDiv(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::MULU:
			emitMulu(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::MULUI:
			emitMulu(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::DIVU:
			emitDivu(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::DIVUI:
			emitDivu(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::NEG:
			emitNeg(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::NEGI:
			emitNeg(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::NEGU:
			emitNegu(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::NEGUI:
			emitNegu(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::SL:
			emitSl(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::SLI:
			emitSl(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::SR:
			emitSr(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::SRI:
			emitSr(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::SLU:
			emitSlu(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::SLUI:
			emitSlu(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::SRU:
			emitSru(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::SRUI:
			emitSru(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::CMP:
			emitCmp(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::CMPI:
			emitCmp(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::CMPU:
			emitCmpu(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::CMPUI:
			emitCmpu(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::CSN:
			emitCsn(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::CSNI:
			emitCsn(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::CSZ:
			emitCsz(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::CSZI:
			emitCsz(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::CSP:
			emitCsp(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::CSPI:
			emitCsp(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::CSOD:
			emitCsod(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::CSODI:
			emitCsod(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::CSNN:
			emitCsnn(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::CSNNI:
			emitCsnn(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::CSNZ:
			emitCsnz(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::CSNZI:
			emitCsnz(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::CSNP:
			emitCsnp(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::CSNPI:
			emitCsnp(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::CSEV:
			emitCsev(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::CSEVI:
			emitCsev(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::ZSN:
			emitZsn(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::ZSNI:
			emitZsn(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::ZSZ:
			emitZsz(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::ZSZI:
			emitZsz(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::ZSP:
			emitZsp(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::ZSPI:
			emitZsp(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::ZSOD:
			emitZsod(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::ZSODI:
			emitZsod(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::ZSNN:
			emitZsnn(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::ZSNNI:
			emitZsnn(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::ZSNZ:
			emitZsnz(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::ZSNZI:
			emitZsnz(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::ZSNP:
			emitZsnp(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::ZSNPI:
			emitZsnp(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::ZSEV:
			emitZsev(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::ZSEVI:
			emitZsev(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::AND:
			emitAnd(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::ANDI:
			emitAnd(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::OR:
			emitOr(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::ORI:
			emitOr(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::XOR:
			emitXor(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::XORI:
			emitXor(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::ANDN:
			emitAndn(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::ANDNI:
			emitAndn(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::ORN:
			emitOrn(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::ORNI:
			emitOrn(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::NAND:
			emitNand(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::NANDI:
			emitNand(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::NOR:
			emitNor(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::NORI:
			emitNor(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::NXOR:
			emitNxor(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::NXORI:
			emitNxor(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::MUX:
			emitMux(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::MUXI:
			emitMux(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::SADD:
			emitSadd(vctx, xarg, yarg, zarg, false);
			break;
		case MmixLlvm::SADDI:
			emitSadd(vctx, xarg, yarg, zarg, true);
			break;
		case MmixLlvm::SETH:
			emitSeth(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::SETMH:
			emitSetmh(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::SETML:
			emitSetml(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::SETL:
			emitSetl(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::INCH:
			emitInch(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::INCMH:
			emitIncmh(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::INCML:
			emitIncml(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::INCL:
			emitIncl(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::ORH:
			emitOrh(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::ORMH:
			emitOrmh(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::ORML:
			emitOrml(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::ORL:
			emitOrl(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::ANDNH:
			emitAndnh(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::ANDNMH:
			emitAndnmh(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::ANDNML:
			emitAndnml(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::ANDNL:
			emitAndnl(vctx, xarg, ((MXWyde)yarg << 8) | zarg);
			break;
		case MmixLlvm::TRIP:
			emitTrip(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::TRAP:
			emitTrap(vctx, xarg, yarg, zarg);
			break;
		case MmixLlvm::GETA:
			emitGeta(vctx, xarg, ((MXWyde)yarg << 8) | zarg, false);
			break;
		case MmixLlvm::GETAB:
			emitGeta(vctx, xarg, ((MXWyde)yarg << 8) | zarg, true);
			break;
		case MmixLlvm::JMP:
			emitJmp(vctx, ((MXTetra)xarg << 16) | ((MXTetra)yarg << 8) | zarg, false);
			break;
		case MmixLlvm::JMPB:
			emitJmp(vctx, ((MXTetra)xarg << 16) | ((MXTetra)yarg << 8) | zarg, true);
			break;
		case MmixLlvm::BN:
			emitBn(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::BNB:
			emitBn(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::BZ:
			emitBz(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::BZB:
			emitBz(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::BP:
			emitBp(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::BPB:
			emitBp(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::BOD:
			emitBod(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::BODB:
			emitBod(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::BNN:
			emitBnn(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::BNNB:
			emitBnn(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::BNZ:
			emitBnz(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::BNZB:
			emitBnz(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::BNP:
			emitBnp(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::BNPB:
			emitBnp(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::BEV:
			emitBev(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::BEVB:
			emitBev(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::PBN:
			emitBn(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::PBNB:
			emitBn(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::PBZ:
			emitBz(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::PBZB:
			emitBz(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::PBP:
			emitBp(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::PBPB:
			emitBp(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::PBOD:
			emitBod(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::PBODB:
			emitBod(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::PBNN:
			emitBnn(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::PBNNB:
			emitBnn(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::PBNZ:
			emitBnz(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::PBNZB:
			emitBnz(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::PBNP:
			emitBnp(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::PBNPB:
			emitBnp(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		case MmixLlvm::PBEV:
			emitBev(vctx, xarg, ((MXWyde) yarg << 8) | zarg, false);
			break;
		case MmixLlvm::PBEVB:
			emitBev(vctx, xarg, ((MXWyde) yarg << 8) | zarg, true);
			break;
		default:
			assert(0 && "Not implemented");
		}
	}
}

namespace {
	Twine getInstrTwine(std::vector<std::string>& twines, MXTetra instr, MXOcta xptr)
	{
		std::ostringstream oss;
		oss<<"instr_"<<std::hex<<instr<<"@"<<xptr<<"#";
		twines.push_back(oss.str());
		return genUniq(Twine(twines.back()));
	}
};

void MmixLlvm::emitSimpleVertice(LLVMContext& ctx, Module& m, MemAccessor& ma, MXOcta xPtr, Vertice& out)
{
	std::vector<std::string> twines;
	MXOcta xPtr0 = xPtr;
	Function* f = cast<Function>(m.getOrInsertFunction(genUniq("fun").str(), Type::getVoidTy(ctx),
		Type::getInt64PtrTy(ctx),Type::getInt64PtrTy(ctx), (Type *)0));
	VerticeContext vctx;
	vctx.Ctx = &ctx;
	vctx.Function = f;
	vctx.Module = &m;
	vctx.Edges = &out.EdgeList;
	BasicBlock *entry = BasicBlock::Create(ctx, genUniq("entry") + Twine(xPtr), f);
	bool term = false;
	while (!term) {
		MXTetra instr = ma.readTetra(xPtr0);
		MXOcta xPtr1 = xPtr += sizeof(MXTetra);
		term = isTerm(instr);
		vctx.XPtr = xPtr0;
		vctx.Instr = instr;
		vctx.Entry = entry;
		BasicBlock *exit = !term ? BasicBlock::Create(ctx, getInstrTwine(twines, instr, xPtr1), f) : 0;
		vctx.Exit = exit;
		emitInstruction(vctx);
		entry = exit;
		xPtr0 = xPtr1;
	}
	out.Function = f;
}
