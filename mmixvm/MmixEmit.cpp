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
using llvm::cast;
using llvm::Twine;

using namespace MmixLlvm::Util;
using namespace MmixLlvm::Private;
using MmixLlvm::EdgeList;
using MmixLlvm::MemAccessor;
using MmixLlvm::SpecialReg;
using MmixLlvm::MXByte;
using MmixLlvm::MXWyde;
using MmixLlvm::MXTetra;
using MmixLlvm::MXOcta;
using llvm::Intrinsic::ID;

namespace {
	class SimpleVerticeContext : public VerticeContext {
		LLVMContext& _lctx;

		Module& _module;

		Function& _func;

		MXOcta _xptr;
		
		MXTetra _opcode;

		BasicBlock *_init;

		BasicBlock *_entry;

		BasicBlock *_mainEntry;

		BasicBlock *_exit;

		const VerticeContext* _trunk;

		struct RegisterRecord {
			bool Dirty;

			Value* Value;
		};

		typedef boost::unordered_map<MXByte, RegisterRecord> RefMap;

		typedef boost::unordered_map<MmixLlvm::SpecialReg, RegisterRecord> SRefMap;

		RefMap _regMap;

		SRefMap _spRegMap;

		std::vector<std::string> _twines;

		Twine getInstrTwine(MXTetra instr, MXOcta xptr);
	public:
		SimpleVerticeContext(LLVMContext& lctx, Module& module, Function& func);

		SimpleVerticeContext(const SimpleVerticeContext& o);

		virtual ~SimpleVerticeContext();

		virtual void feedNewOpcode(MXOcta xptr, MXTetra opcode, bool term);

		virtual MXTetra getInstr();

		virtual MXOcta getXPtr();

		virtual LLVMContext& getLctx();

		virtual Value* getModuleVar(const char* varName);

		virtual Function* getModuleFunction(const char* varName);

		virtual Function* getIntrinsic(ID id, ArrayRef<Type*> Tys);

		virtual BasicBlock *getOCEntry();

		virtual BasicBlock *getOCExit();

		virtual llvm::BasicBlock *makeBlock(const llvm::Twine& prefix);

		virtual std::vector<llvm::Argument*> getVerticeArgs();

		virtual Value* getRegister(MXByte reg);

		virtual Value* getSpRegister(SpecialReg reg);

		virtual void assignRegister(MXByte reg, llvm::Value* val);

		virtual void assignSpRegister(SpecialReg reg, llvm::Value* val);

		virtual std::vector<MXByte> getDirtyRegisters();

		virtual std::vector<SpecialReg> getDirtySpRegisters();

		virtual void markAllClean();

		virtual boost::shared_ptr<VerticeContext> makeBranch();
	};

	SimpleVerticeContext::SimpleVerticeContext(LLVMContext& lctx, Module& module, Function& func)
		:_lctx(lctx)
		,_module(module)
		,_func(func)
		,_xptr(0)
		,_opcode(0)
		,_init(0)
		,_entry(0)
		,_exit(0)
		,_trunk(NULL)
	{
		_init = BasicBlock::Create(_lctx, genUniq("init") + Twine(_xptr), &_func);
		_entry = BasicBlock::Create(_lctx, genUniq("entry") + Twine(_xptr), &_func);
		_mainEntry = _entry;
	}

	SimpleVerticeContext::SimpleVerticeContext(const SimpleVerticeContext& o)
		:_lctx(o._lctx)
		,_module(o._module)
		,_func(o._func)
		,_xptr(o._xptr)
		,_opcode(o._opcode)
		,_init(o._init)
		,_entry(o._entry)
		,_exit(o._exit)
		,_mainEntry(o._mainEntry)
		,_regMap(o._regMap)
		,_spRegMap(o._spRegMap)
		,_trunk(&o)
	{}

	SimpleVerticeContext::~SimpleVerticeContext()
	{
		if (!_trunk) {
			IRBuilder<> builder(_lctx);
			builder.SetInsertPoint(_init);
			builder.CreateBr(_mainEntry);
		}
	}

	void SimpleVerticeContext::feedNewOpcode(MXOcta xptr, MXTetra opcode, bool term)
	{
		_xptr = xptr;
		_opcode = opcode;
		if (_exit)
			_entry = _exit;
		_exit = !term ? BasicBlock::Create(_lctx, getInstrTwine(_opcode, _xptr), &_func) : 0;
	}

	MXTetra SimpleVerticeContext::getInstr() {
		return _opcode;
	}

	MXOcta SimpleVerticeContext::getXPtr() {
		return _xptr;
	}

	LLVMContext& SimpleVerticeContext::getLctx() {
		return _lctx;
	}

	Value* SimpleVerticeContext::getModuleVar(const char* varName) {
		return _module.getGlobalVariable(varName);
	}

	Function* SimpleVerticeContext::getModuleFunction(const char* funcName) {
		return _module.getFunction(funcName);
	}

	Function* SimpleVerticeContext::getIntrinsic(ID id, ArrayRef<Type*> argTypes) {
		return llvm::Intrinsic::getDeclaration(&_module, id, argTypes);
	}

	BasicBlock* SimpleVerticeContext::getOCEntry() {
		return _entry;
	}

	BasicBlock* SimpleVerticeContext::getOCExit() {
		return _exit;
	}

	BasicBlock* SimpleVerticeContext::makeBlock(const Twine& prefix) {
		return BasicBlock::Create(_lctx, genUniq(prefix), &_func);
	}

	std::vector<llvm::Argument*> SimpleVerticeContext::getVerticeArgs() {
		std::vector<llvm::Argument*> retVal;
		for (auto itr = _func.arg_begin(); itr != _func.arg_end(); ++itr)
			retVal.push_back(&*itr);
		return retVal;
	}

	Value* SimpleVerticeContext::getRegister(MXByte reg) {
		IRBuilder<> builder(_lctx);
		builder.SetInsertPoint(_init);
		RefMap::iterator itr = _regMap.find(reg);
		Value *retVal;
		if (itr == _regMap.end()) {
			Value* regGlob = builder.CreateSelect(
				builder.CreateICmpULT(builder.getInt64(reg), getSpRegister(MmixLlvm::rG)),
					_module.getGlobalVariable("RegisterStackTop"),
					_module.getGlobalVariable("RegisterStackBase"));
			Value* val = builder.CreateLoad(
				builder.CreateGEP(
					builder.CreateLoad(regGlob, false),
					builder.getInt32(reg)), false);
			RegisterRecord r0;
			r0.Dirty = false;
			r0.Value = val;
			_regMap[reg] = r0;
			retVal = r0.Value;
		} else {
			retVal = itr->second.Value;
		}
		return retVal;
	}

	Value* SimpleVerticeContext::getSpRegister(SpecialReg sreg) {
		IRBuilder<> builder(_lctx);
		builder.SetInsertPoint(_init);
		Value *regGlob = _module.getGlobalVariable("SpecialRegisters");
		SRefMap::iterator itr = _spRegMap.find(sreg);
		Value *retVal;
		if (itr == _spRegMap.end()) {
			Value* ix[2];
			ix[0] = builder.getInt32(0);
			ix[1] = builder.getInt32(sreg);
			Value* val = builder.CreateLoad(
				builder.CreatePointerCast(
				builder.CreateGEP(regGlob, ArrayRef<Value*>(ix, ix + 2)), Type::getInt64PtrTy(_lctx)), false, Twine("sreg")+Twine(sreg));
			RegisterRecord r0;
			r0.Dirty = false;
			r0.Value = val;
			_spRegMap[sreg] = r0;
			retVal = r0.Value;
		} else {
			retVal = itr->second.Value;
		}
		return retVal;
	}

	void SimpleVerticeContext::assignRegister(MXByte reg, Value* val) {
		RegisterRecord r0;
		r0.Dirty = true;
		r0.Value = val;
		_regMap[reg] = r0;
	}

	void SimpleVerticeContext::assignSpRegister(SpecialReg sreg, Value* val) {
		RegisterRecord r0;
		r0.Dirty = true;
		r0.Value = val;
		_spRegMap[sreg] = r0;
	}

	void SimpleVerticeContext::markAllClean() {
		for (RefMap::iterator itr = _regMap.begin(); itr != _regMap.end(); ++itr)
			itr->second.Dirty = false;
		for (SRefMap::iterator itr = _spRegMap.begin(); itr != _spRegMap.end(); ++itr)
			itr->second.Dirty = false;
	}

	std::vector<MXByte> SimpleVerticeContext::getDirtyRegisters() {
		std::vector<MXByte> retVal;
		for (RefMap::iterator itr = _regMap.begin(); itr != _regMap.end(); ++itr)
			if (itr->second.Dirty)
				retVal.push_back(itr->first);
		return retVal;
	}

	std::vector<SpecialReg> SimpleVerticeContext::getDirtySpRegisters() {
		std::vector<SpecialReg> retVal;
		for (SRefMap::iterator itr = _spRegMap.begin(); itr != _spRegMap.end(); ++itr)
			if (itr->second.Dirty)
				retVal.push_back(itr->first);
		return retVal;
	}

	boost::shared_ptr<VerticeContext> SimpleVerticeContext::makeBranch() {
		return boost::shared_ptr<VerticeContext>(new SimpleVerticeContext(*this));
	}

	Twine SimpleVerticeContext::getInstrTwine(MXTetra instr, MXOcta xptr)
	{
		std::ostringstream oss;
		oss<<"instr_"<<std::hex<<instr<<"@"<<xptr<<"#";
		_twines.push_back(oss.str());
		return genUniq(Twine(_twines.back()));
	}

	bool isTerm(MXTetra instr) {
		MXByte o0 = (MXByte) (instr >> 24);
		switch(o0) {
		case MmixLlvm::GO:
		case MmixLlvm::GOI:
		case MmixLlvm::JMP:
		case MmixLlvm::JMPB:
		case MmixLlvm::PUSHJ:
		case MmixLlvm::PUSHJB:
		case MmixLlvm::PUSHGO:
		case MmixLlvm::PUSHGOI:
		case MmixLlvm::POP:
		case MmixLlvm::TRIP:
			return true;
		case MmixLlvm::TRAP:
			return instr == 0;
		default:
			return false;
		}
	}
}

void MmixLlvm::emitSimpleVertice(LLVMContext& ctx, Module& m, Engine& e, 
	MXOcta xPtr, Vertice& out)
{
	MXOcta xPtr0 = xPtr;
	Function* f = cast<Function>(m.getOrInsertFunction(genUniq("fun").str(), Type::getVoidTy(ctx),
		Type::getInt64PtrTy(ctx),Type::getInt64PtrTy(ctx), (Type *)0));
	SimpleVerticeContext vctx(ctx, m, *f);
	vctx.getSpRegister(MmixLlvm::rL);
	bool term = false;
	while (!term) {
		MXTetra instr = e.readTetra(xPtr0);
		MXOcta xPtr1 = xPtr += sizeof(MXTetra);
		term = isTerm(instr);
		vctx.feedNewOpcode(xPtr0, instr, term);
		LLVMContext& ctx = vctx.getLctx();
		IRBuilder<> builder(ctx);
		builder.SetInsertPoint(vctx.getOCEntry());
		emitInstruction(vctx, builder);
		xPtr0 = xPtr1;
	}
	out.Function = f;
}
