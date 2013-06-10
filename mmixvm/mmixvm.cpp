// mmixvm.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MemAccess.h"
#include "MmixDef.h"
#include "MmixEmit.h"

enum {
	GENERIC_REGISTERS = 1 << 8,
	SPECIAL_REGISTERS = 1 << 5,
	TEXT_SIZE = 16 * 1024,
	HEAP_SIZE = 16 * 1024,
	POOL_SIZE = 16 * 1024,
	STACK_SIZE = 16 * 1024,
};

using namespace llvm;

namespace {
	void HandleOverflow() {
		outs() << "Overflow !!!\n\n";
		outs().flush();
	}

	void DebugInt32(int arg) {
		outs() << "Debug int32 "<<arg<<'\n';
		outs().flush();
	}

	void DebugInt64(int64_t arg) {
		outs() << "Debug int64 "<<arg<<'\n';
		outs().flush();
	}

	const mpz_class _64MAX("FFFFFFFFFFFFFFFF", 16);

	void MuluImpl(uint64_t arg1, uint64_t arg2, uint64_t* hiProd, uint64_t* loProd)
	{
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

	void DivuImpl(uint64_t hidivident, uint64_t lodivident, uint64_t divisor, uint64_t* quotient, uint64_t* remainder) 
	{
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

	uint64_t Adjust64EndiannessImpl(uint64_t arg) {
		uint8_t* ref = (uint8_t*)&arg;
		return  ((uint64_t)ref[0] << 56) | ((uint64_t)ref[1] << 48)
	          | ((uint64_t)ref[2] << 40) | ((uint64_t)ref[3] << 32)
		      | ((uint64_t)ref[4] << 24) | ((uint64_t)ref[5] << 16)
	          | ((uint64_t)ref[6] << 8)  |  (uint64_t)ref[7];
	}

	class MemAccessImpl : public MmixLlvm::MemAccessor {
		const llvm::ArrayRef<uint8_t> _textRef;

		const llvm::ArrayRef<uint8_t> _dataRef;

		const llvm::ArrayRef<uint8_t> _poolRef;

		const llvm::ArrayRef<uint8_t> _stackRef;

		uint16_t adjust16Endianness(llvm::ArrayRef<uint8_t>& ref);

		uint32_t adjust32Endianness(llvm::ArrayRef<uint8_t>& ref);

		uint64_t adjust64Endianness(llvm::ArrayRef<uint8_t>& ref);
	public:
		MemAccessImpl(llvm::ArrayRef<uint8_t> textRef, 
			llvm::ArrayRef<uint8_t> dataRef, 
			llvm::ArrayRef<uint8_t> poolRef, 
			llvm::ArrayRef<uint8_t> stackRef);

		virtual uint8_t readByte(uint64_t ref);

		virtual uint16_t readWyde(uint64_t ref);

		virtual uint32_t readTetra(uint64_t ref);

		virtual uint64_t readOcta(uint64_t ref);

		virtual ~MemAccessImpl();
	};

	MemAccessImpl::MemAccessImpl(llvm::ArrayRef<uint8_t> textRef, 
			llvm::ArrayRef<uint8_t> dataRef, 
			llvm::ArrayRef<uint8_t> poolRef, 
			llvm::ArrayRef<uint8_t> stackRef)
		:_textRef(textRef)
		,_dataRef(dataRef)
		,_poolRef(poolRef)
		,_stackRef(stackRef)
	{}

	MemAccessImpl::~MemAccessImpl()
	{}

	uint8_t MemAccessImpl::readByte(uint64_t ref) {
		if (ref < MmixLlvm::DATA_SEG)
			return _textRef[ref];
		else if (ref >= MmixLlvm::DATA_SEG && ref < MmixLlvm::POOL_SEG)
			return _dataRef[ref];
		else if (ref >= MmixLlvm::POOL_SEG && ref < MmixLlvm::STACK_SEG)
			return _poolRef[ref];
		else
			return _stackRef[ref];
	}

	uint16_t MemAccessImpl::readWyde(uint64_t ref) {
		if (ref < MmixLlvm::DATA_SEG)
			return adjust16Endianness(_textRef.slice(ref, 2));
		else if (ref >= MmixLlvm::DATA_SEG && ref < MmixLlvm::POOL_SEG)
			return adjust16Endianness(_dataRef.slice(ref, 2));
		else if (ref >= MmixLlvm::POOL_SEG && ref < MmixLlvm::STACK_SEG)
			return adjust16Endianness(_poolRef.slice(ref, 2));
		else
			return adjust16Endianness(_stackRef.slice(ref, 2));
	}

	uint32_t MemAccessImpl::readTetra(uint64_t ref) {
		if (ref < MmixLlvm::DATA_SEG)
			return adjust32Endianness(_textRef.slice(ref, 4));
		else if (ref >= MmixLlvm::DATA_SEG && ref < MmixLlvm::POOL_SEG)
			return adjust32Endianness(_dataRef.slice(ref, 4));
		else if (ref >= MmixLlvm::POOL_SEG && ref < MmixLlvm::STACK_SEG)
			return adjust32Endianness(_poolRef.slice(ref, 4));
		else
			return adjust32Endianness(_stackRef.slice(ref, 4));
	}

	uint64_t MemAccessImpl::readOcta(uint64_t ref) {
		if (ref < MmixLlvm::DATA_SEG)
			return adjust64Endianness(_textRef.slice(ref, 8));
		else if (ref >= MmixLlvm::DATA_SEG && ref < MmixLlvm::POOL_SEG)
			return adjust64Endianness(_dataRef.slice(ref, 8));
		else if (ref >= MmixLlvm::POOL_SEG && ref < MmixLlvm::STACK_SEG)
			return adjust64Endianness(_poolRef.slice(ref, 8));
		else
			return adjust64Endianness(_stackRef.slice(ref, 8));
	}

	uint16_t MemAccessImpl::adjust16Endianness(llvm::ArrayRef<uint8_t>& ref) {
		return ((uint16_t)ref[0] << 8) | (uint16_t)ref[1];
	}

	uint32_t MemAccessImpl::adjust32Endianness(llvm::ArrayRef<uint8_t>& ref) {
		return ((uint32_t)ref[0] << 24) | ((uint32_t)ref[1] << 16) 
			 | ((uint32_t)ref[2] << 8)  | (uint32_t)ref[3];
	}

	uint64_t MemAccessImpl::adjust64Endianness(llvm::ArrayRef<uint8_t>& ref) {
		return ((uint64_t)ref[0] << 56) | ((uint64_t)ref[1] << 48)
			 | ((uint64_t)ref[2] << 40) | ((uint64_t)ref[3] << 32)
			 | ((uint64_t)ref[4] << 24) | ((uint64_t)ref[5] << 16)
			 | ((uint64_t)ref[6] << 8)  |  (uint64_t)ref[7];
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	InitializeNativeTarget();
	LLVMContext Context;
	// Create some module to put our function into it.
	
	Module *M = new Module("test", Context);
	
	GlobalVariable* registersGlob = new GlobalVariable(*M,
		ArrayType::get(Type::getInt64Ty(Context), GENERIC_REGISTERS),
		false,
		GlobalValue::CommonLinkage,
		0,
		"Registers");
	registersGlob->setAlignment(8);

	GlobalVariable* specialRegistersGlob = new GlobalVariable(*M,
		ArrayType::get(Type::getInt64Ty(Context), SPECIAL_REGISTERS),
		false,
		GlobalValue::CommonLinkage,
		0,
		"SpecialRegisters");
	specialRegistersGlob->setAlignment(8);

	enum {MEM_ARR_SIZE = TEXT_SIZE + HEAP_SIZE + POOL_SIZE + STACK_SIZE};
	
	GlobalVariable* memGlob = new GlobalVariable(*M,
		ArrayType::get(Type::getInt8Ty(Context), MEM_ARR_SIZE),
		false,
		GlobalValue::CommonLinkage,
		0,
		"Memory");

	memGlob->setAlignment(8);

	GlobalVariable* addressTranslateTableGlob = new GlobalVariable(*M,
		ArrayType::get(Type::getInt32Ty(Context), 4),
		false,
		GlobalValue::CommonLinkage,
		0,
		"AddressTranslateTable");
	addressTranslateTableGlob->setAlignment(8);

	Type* params[5];
	params[0] = Type::getInt64Ty(Context);
	params[1] = Type::getInt64Ty(Context);
	params[2] = Type::getInt64PtrTy(Context);
	params[3] = Type::getInt64PtrTy(Context);
	llvm::Function* muluImplF = llvm::Function::Create(
		FunctionType::get(Type::getVoidTy(Context), ArrayRef<Type*>(params, params + 4), false), 
		Function::ExternalLinkage, "MuluImpl", M);

	params[0] = Type::getInt64Ty(Context);
	params[1] = Type::getInt64Ty(Context);
	params[2] = Type::getInt64Ty(Context);
	params[3] = Type::getInt64PtrTy(Context);
	params[4] = Type::getInt64PtrTy(Context);
	llvm::Function* divuImplF = llvm::Function::Create(
		FunctionType::get(Type::getVoidTy(Context), ArrayRef<Type*>(params, params + 5), false), 
		Function::ExternalLinkage, "DivuImpl", M);

	params[0] = Type::getInt64Ty(Context);
	llvm::Function* adjust64EndiannessImplF = llvm::Function::Create(
		FunctionType::get(Type::getInt64Ty(Context), ArrayRef<Type*>(params, params + 1), false), 
		Function::ExternalLinkage, "Adjust64EndiannessImpl", M);

	params[0] = Type::getInt32Ty(Context);
	llvm::Function* debugInt32 = llvm::Function::Create(
		FunctionType::get(Type::getVoidTy(Context), ArrayRef<Type*>(params, params + 1), false), 
		Function::ExternalLinkage, "DebugInt32", M);

	params[0] = Type::getInt64Ty(Context);

	llvm::Function* debugInt64 = llvm::Function::Create(
		FunctionType::get(Type::getVoidTy(Context), ArrayRef<Type*>(params, params + 1), false), 
		Function::ExternalLinkage, "DebugInt64", M);

	boost::scoped_ptr<ExecutionEngine> EE(EngineBuilder(M).create());
	EE->addGlobalMapping(muluImplF, &MuluImpl);
	EE->addGlobalMapping(divuImplF, &DivuImpl);
	EE->addGlobalMapping(adjust64EndiannessImplF, &Adjust64EndiannessImpl);
	EE->addGlobalMapping(debugInt32, &DebugInt32);
	EE->addGlobalMapping(debugInt64, &DebugInt64);

	std::vector<uint64_t> arr(GENERIC_REGISTERS);
	arr[1] = MmixLlvm::DATA_SEG;
	arr[2] = 32;
	arr[3] = 40;
	arr[10] = -13;
	arr[20] = 0xCCCCCCCCCCCCCCCC;
	arr[21] = 0xAAAAAAAAAAAAAAAA;
	EE->addGlobalMapping(registersGlob, &arr[0]);

	std::vector<uint64_t> spArr(SPECIAL_REGISTERS);
	spArr[MmixLlvm::rA] = 0; //MmixLlvm::V;
	spArr[MmixLlvm::rD] = -1;
	spArr[MmixLlvm::rM] = 0xFF00FF00FF00FF00;
	EE->addGlobalMapping(specialRegistersGlob, &spArr[0]);
	
	std::vector<uint8_t> memPhys(MEM_ARR_SIZE);
	memPhys[0x100] = MmixLlvm::LDO;
	memPhys[0x101] = 4;
	memPhys[0x102] = 1;
	memPhys[0x103] = 2;
	memPhys[0x104] = MmixLlvm::LDO;
	memPhys[0x105] = 5;
	memPhys[0x106] = 1;
	memPhys[0x107] = 3;
	memPhys[0x108] = MmixLlvm::LDO;
	memPhys[0x109] = 6;
	memPhys[0x10a] = 1;
	memPhys[0x10b] = 2;
	memPhys[0x10c] = MmixLlvm::LDO;
	memPhys[0x10d] = 7;
	memPhys[0x10e] = 1;
	memPhys[0x10f] = 3;
	memPhys[0x110] = MmixLlvm::MUX;
	memPhys[0x111] = 11;
	memPhys[0x112] = 20;
	memPhys[0x113] = 21;
	memPhys[0x114] = MmixLlvm::DIVU;
	memPhys[0x115] = 0;
	memPhys[0x116] = 4;
	memPhys[0x117] = 5;
	memPhys[0x118] = MmixLlvm::SUBI;
	memPhys[0x119] = 0;
	memPhys[0x11a] = 0;
	memPhys[0x11b] = 1;
	memPhys[0x11c] = MmixLlvm::STOI;
	memPhys[0x11d] = 0;
	memPhys[0x11e] = 1;
	memPhys[0x11f] = 64;
	EE->addGlobalMapping(memGlob, &memPhys[0]);

	uint8_t* dataPtr = &memPhys[0] + TEXT_SIZE;
	//#9e 37 79 b9 7f 4a 71 6
	dataPtr[32] = 0;
	dataPtr[33] = 0;
	dataPtr[34] = 0;
	dataPtr[35] = 0;
	dataPtr[36] = 0;
	dataPtr[37] = 0;
	dataPtr[38] = 0;
	dataPtr[39] = 0;
	dataPtr[40] = 0;
	dataPtr[41] = 0;
	dataPtr[42] = 0;
	dataPtr[43] = 0;
	dataPtr[44] = 0;
	dataPtr[45] = 0;
	dataPtr[46] = 0;
	dataPtr[47] = 13;

	std::vector<uint32_t> att(4);
	att[0] = 0;
	att[1] = TEXT_SIZE;
	att[2] = TEXT_SIZE + HEAP_SIZE;
	att[3] = TEXT_SIZE + HEAP_SIZE + POOL_SIZE;
	EE->addGlobalMapping(addressTranslateTableGlob, &att[0]);

	boost::scoped_ptr<MmixLlvm::MemAccessor> accessor(
		new MemAccessImpl(llvm::ArrayRef<uint8_t>(&memPhys[0] + att[0], &memPhys[0] + att[1]),
		            llvm::ArrayRef<uint8_t>(&memPhys[0] + att[1], &memPhys[0] + att[2]),
					llvm::ArrayRef<uint8_t>(&memPhys[0] + att[2], &memPhys[0] + att[3]),
					llvm::ArrayRef<uint8_t>(&memPhys[0] + att[3], &memPhys[0] + MEM_ARR_SIZE)));

	Function *f;
	MmixLlvm::EdgeList edgeList;

	boost::tie(f, edgeList) = MmixLlvm::emitSimpleVertice(Context, *M, *accessor, 0x100);

	outs() << *M;
	outs().flush();
	uint64_t instrAddr, targetAddr;
	std::vector<GenericValue> args(2);
	args[0] = GenericValue(&instrAddr);
	args[1] = GenericValue(&targetAddr);
	GenericValue gv = EE->runFunction(f, args);
	EE->freeMachineCodeForFunction(f);
	llvm_shutdown();
	return 0;
}
