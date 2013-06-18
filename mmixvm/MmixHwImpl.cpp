#include "stdafx.h"
#include "Util.h"
#include "MmixHwImpl.h"

using llvm::Module;
using llvm::Type;
using llvm::ArrayType;
using llvm::FunctionType;
using llvm::GlobalVariable;
using llvm::GlobalValue;
using llvm::GenericValue;
using llvm::ArrayRef;
using llvm::Function;
using llvm::EngineBuilder;
using llvm::outs;

using MmixLlvm::OS;
using MmixLlvm::MmixHwImpl;
using MmixLlvm::HardwareCfg;

namespace {
	enum { REGION_BIT_OFFSET = 61 };
	const uint64_t TWO_ENABLED_BITS = 3LL;
	const uint64_t ADDR_MASK = ~(TWO_ENABLED_BITS << REGION_BIT_OFFSET);
};

enum {
	GENERIC_REGISTERS = 1 << 8,
	SPECIAL_REGISTERS = 1 << 5,
};

MmixHwImpl::MmixHwImpl(const HardwareCfg& hwCfg, boost::shared_ptr<OS> os)
	:_registers(GENERIC_REGISTERS)
	,_spRegisters(SPECIAL_REGISTERS)
	,_memory(hwCfg.TextSize + hwCfg.HeapSize + hwCfg.PoolSize + hwCfg.StackSize)
	,_att(4)
	,_os(os)
	,_halted(false)
{
	_att[0] = 0;
	_att[1] = hwCfg.TextSize;
	_att[2] = hwCfg.TextSize + hwCfg.HeapSize;
	_att[3] = hwCfg.TextSize + hwCfg.HeapSize + hwCfg.PoolSize;
}

void MmixHwImpl::postInit()
{
	_module = new Module("mmixvm", _lctx);
	GlobalVariable* registersGlob = new GlobalVariable(*_module,
		ArrayType::get(Type::getInt64Ty(_lctx), GENERIC_REGISTERS),
		false,
		GlobalValue::CommonLinkage,
		0,
		"Registers");
	registersGlob->setAlignment(8);

	GlobalVariable* specialRegistersGlob = new GlobalVariable(*_module,
		ArrayType::get(Type::getInt64Ty(_lctx), SPECIAL_REGISTERS),
		false,
		GlobalValue::CommonLinkage,
		0,
		"SpecialRegisters");
	specialRegistersGlob->setAlignment(8);

	GlobalVariable* memGlob = new GlobalVariable(*_module,
		ArrayType::get(Type::getInt8Ty(_lctx), _memory.size()),
		false,
		GlobalValue::CommonLinkage,
		0,
		"Memory");
	memGlob->setAlignment(8);

	GlobalVariable* addressTranslateTableGlob = new GlobalVariable(*_module,
		ArrayType::get(Type::getInt32Ty(_lctx), 4),
		false,
		GlobalValue::CommonLinkage,
		0,
		"AddressTranslateTable");
	addressTranslateTableGlob->setAlignment(8);

	Type* params[5];
	params[0] = Type::getInt64Ty(_lctx);
	params[1] = Type::getInt64Ty(_lctx);
	params[2] = Type::getInt64PtrTy(_lctx);
	params[3] = Type::getInt64PtrTy(_lctx);
	llvm::Function* muluImplF = llvm::Function::Create(
		FunctionType::get(Type::getVoidTy(_lctx), ArrayRef<Type*>(params, params + 4), false), 
		Function::ExternalLinkage, "MuluImpl", _module);

	params[0] = Type::getInt64Ty(_lctx);
	params[1] = Type::getInt64Ty(_lctx);
	params[2] = Type::getInt64Ty(_lctx);
	params[3] = Type::getInt64PtrTy(_lctx);
	params[4] = Type::getInt64PtrTy(_lctx);
	llvm::Function* divuImplF = llvm::Function::Create(
		FunctionType::get(Type::getVoidTy(_lctx), ArrayRef<Type*>(params, params + 5), false), 
		Function::ExternalLinkage, "DivuImpl", _module);

	params[0] = Type::getInt64Ty(_lctx);
	llvm::Function* adjust64EndiannessImplF = llvm::Function::Create(
		FunctionType::get(Type::getInt64Ty(_lctx), ArrayRef<Type*>(params, params + 1), false), 
		Function::ExternalLinkage, "Adjust64EndiannessImpl", _module);

	params[0] = Type::getInt32Ty(_lctx);
	llvm::Function* debugInt32 = llvm::Function::Create(
		FunctionType::get(Type::getVoidTy(_lctx), ArrayRef<Type*>(params, params + 1), false), 
		Function::ExternalLinkage, "DebugInt32", _module);

	params[0] = Type::getInt64Ty(_lctx);

	llvm::Function* debugInt64 = llvm::Function::Create(
		FunctionType::get(Type::getVoidTy(_lctx), ArrayRef<Type*>(params, params + 1), false), 
		Function::ExternalLinkage, "DebugInt64", _module);

	_ee.reset(EngineBuilder(_module).create());
	_ee->addGlobalMapping(muluImplF, &MmixHwImpl::muluImpl);
	_ee->addGlobalMapping(divuImplF, &MmixHwImpl::divuImpl);
	_ee->addGlobalMapping(adjust64EndiannessImplF, &MmixHwImpl::adjust64EndiannessImpl);
	_ee->addGlobalMapping(debugInt32, &MmixHwImpl::debugInt32);
	_ee->addGlobalMapping(debugInt64, &MmixHwImpl::debugInt64);
	_ee->addGlobalMapping(registersGlob, &_registers[0]);
	_ee->addGlobalMapping(specialRegistersGlob, &_spRegisters[0]);
	_ee->addGlobalMapping(memGlob, &_memory[0]);
	_ee->addGlobalMapping(addressTranslateTableGlob, &_att[0]);
}

uint8_t* MmixHwImpl::translateAddr(uint64_t addr, uint8_t mask) {
	uint64_t addr0 = addr & ~((uint64_t)mask);
	size_t base = _att[(size_t)((addr0 >> 61) & 3)];
	size_t offset = addr0 & ADDR_MASK;
	return &_memory[base + offset];
}

void MmixHwImpl::debugInt32(int arg) {
	outs() << "Debug int32 "<<arg<<'\n';
	outs().flush();
}

void MmixHwImpl::debugInt64(int64_t arg) {
	outs() << "Debug int64 "<<arg<<'\n';
	outs().flush();
}

namespace {
	const mpz_class _64MAX("FFFFFFFFFFFFFFFF", 16);
};

void MmixHwImpl::muluImpl(uint64_t arg1, uint64_t arg2, uint64_t* hiProd, uint64_t* loProd) {
	mpz_class arg1_ = (mpz_class((uint32_t)(arg1 >> 32)) << 32)
					   | mpz_class((uint32_t)(arg1 & UINT32_MAX));
	mpz_class arg2_ = (mpz_class((uint32_t)(arg2 >> 32)) << 32)
					   | mpz_class((uint32_t)(arg2 & UINT32_MAX));
	mpz_class prod_ = arg1_ * arg2_;
	mpz_class hiProd_ = prod_ >> 64;
	mpz_class loProd_ = prod_ & _64MAX;
	*hiProd = hiProd_.get_ux();
	*loProd = loProd_.get_ux();
}

void MmixHwImpl::divuImpl(uint64_t hidivident, uint64_t lodivident, uint64_t divisor, uint64_t* quotient, uint64_t* remainder) {
	mpz_class divident_ = (mpz_class((uint32_t)(hidivident>>32)) << 96) 
		                   | (mpz_class((uint32_t)(hidivident & UINT32_MAX)) << 64)
					       | (mpz_class((uint32_t)(lodivident >> 32)) << 32)
					       | mpz_class((uint32_t)(lodivident & UINT32_MAX));
	mpz_class divisor_ = (mpz_class((uint32_t)(divisor >> 32)) << 32)
					       | mpz_class((uint32_t)(divisor & UINT32_MAX));
	mpz_class quotient_ = divident_ / divisor_;
	mpz_class remainder_ = divident_ % divisor_;
	*quotient = quotient_.get_ux();
	*remainder = remainder_.get_ux();
}

uint64_t MmixHwImpl::adjust64EndiannessImpl(uint64_t arg) {
	uint8_t* ref = (uint8_t*)&arg;
	return  ((uint64_t)ref[0] << 56) | ((uint64_t)ref[1] << 48)
	          | ((uint64_t)ref[2] << 40) | ((uint64_t)ref[3] << 32)
		      | ((uint64_t)ref[4] << 24) | ((uint64_t)ref[5] << 16)
	          | ((uint64_t)ref[6] << 8)  |  (uint64_t)ref[7];
}

boost::shared_ptr<MmixHwImpl> MmixHwImpl::create(const HardwareCfg& hwCfg, boost::shared_ptr<OS> os)
{
	boost::shared_ptr<MmixHwImpl> retVal(new MmixHwImpl(hwCfg, os));
	retVal->postInit();
	return retVal;
}

void MmixHwImpl::run(uint64_t xref) {
	uint64_t xref0 = xref;
	uint64_t instrAddr, targetAddr;
	std::vector<GenericValue> args(2);
	args[0] = GenericValue(&instrAddr);
	args[1] = GenericValue(&targetAddr);
	_os->loadExecutable(*this);
	while(!_halted) {
		VerticeMap::iterator itr = _vertices.find(xref0);
		if (itr == _vertices.end()) {
			Vertice newVertice;
			emitSimpleVertice(_lctx, *_module, *this, xref0, newVertice);
			newVertice.Entry = 
				(void (*)(uint64_t*,uint64_t*))_ee->getPointerToFunction(newVertice.Function);
			_vertices[xref0] = newVertice;
			itr = _vertices.find(xref0);
		}
		Vertice& v = itr->second;
		(*v.Entry)(&instrAddr, &targetAddr);
		if ((targetAddr & (1ull << 63)) == 0) {
			xref0 = targetAddr;
		} else {
			_os->handleTrap(*this, targetAddr);
			xref0 = _spRegisters[MmixLlvm::rWW];
		}
	}
}

void MmixHwImpl::halt() {
	_halted = true;
}

uint64_t MmixHwImpl::getReg(uint8_t reg) {
	return _registers[reg];
}

void MmixHwImpl::setReg(uint8_t reg, uint64_t value) {
	_registers[reg] = value;
}

uint64_t MmixHwImpl::getSpReg(MmixLlvm::SpecialReg sreg) {
	return _spRegisters[sreg];
}

void MmixHwImpl::setSpReg(MmixLlvm::SpecialReg sreg, uint64_t value) {
	_spRegisters[sreg] = value;
}

uint8_t MmixHwImpl::readByte(uint64_t ref) {
	return *translateAddr(ref, 0);
}

uint16_t MmixHwImpl::readWyde(uint64_t ref) {
	uint8_t* t = translateAddr(ref, 1);
	return MmixLlvm::Util::adjust16Endianness(ArrayRef<uint8_t>(t, t + 2));
}

uint32_t MmixHwImpl::readTetra(uint64_t ref) {
	uint8_t* t = translateAddr(ref, 3);
	return MmixLlvm::Util::adjust32Endianness(ArrayRef<uint8_t>(t, t + 4));
}

uint64_t MmixHwImpl::readOcta(uint64_t ref) {
	uint8_t* t = translateAddr(ref, 7);
	return MmixLlvm::Util::adjust64Endianness(ArrayRef<uint8_t>(t, t + 8));
}

void MmixHwImpl::writeByte(uint64_t ref, uint8_t arg) {
	uint8_t* t = translateAddr(ref, 0);
	*t = arg;
}

void MmixHwImpl::writeWyde(uint64_t ref, uint16_t arg) {
	uint16_t* t = (uint16_t*)translateAddr(ref, 1);
	uint8_t* p0 = (uint8_t*)&arg;
	*t = MmixLlvm::Util::adjust16Endianness(ArrayRef<uint8_t>(p0, p0 + 2));
}

void MmixHwImpl::writeTetra(uint64_t ref, uint32_t arg) {
	uint32_t* t = (uint32_t*)translateAddr(ref, 3);
	uint8_t* p0 = (uint8_t*)&arg;
	*t = MmixLlvm::Util::adjust32Endianness(ArrayRef<uint8_t>(p0, p0 + 4));
}

void MmixHwImpl::writeOcta(uint64_t ref, uint64_t arg) {
	uint64_t* t = (uint64_t*)translateAddr(ref, 7);
	uint8_t* p0 = (uint8_t*)&arg;
	*t = MmixLlvm::Util::adjust64Endianness(ArrayRef<uint8_t>(p0, p0 + 8));
}

MmixHwImpl::~MmixHwImpl()
{
}