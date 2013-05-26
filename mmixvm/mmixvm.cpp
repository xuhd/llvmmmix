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
			 | ((uint64_t)ref[6] << 8)  | (uint64_t)ref[7] << 16;
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
	
	GlobalVariable* textGlob = new GlobalVariable(*M,
		ArrayType::get(Type::getInt8Ty(Context), TEXT_SIZE),
		false,
		GlobalValue::CommonLinkage,
		0,
		"TextSeg");

	textGlob->setAlignment(8);

	GlobalVariable* dataGlob = new GlobalVariable(*M,
		ArrayType::get(Type::getInt8Ty(Context), HEAP_SIZE),
		false,
		GlobalValue::CommonLinkage,
		0,
		"DataSeg");

	dataGlob->setAlignment(8);

	GlobalVariable* poolGlob = new GlobalVariable(*M,
		ArrayType::get(Type::getInt8Ty(Context), POOL_SIZE),
		false,
		GlobalValue::CommonLinkage,
		0,
		"PoolSeg");

	poolGlob->setAlignment(8);

	GlobalVariable* stackGlob = new GlobalVariable(*M,
		ArrayType::get(Type::getInt8Ty(Context), STACK_SIZE),
		false,
		GlobalValue::CommonLinkage,
		0,
		"StackSeg");
	stackGlob->setAlignment(8);

	//GlobalVariable* HandleOverflowRef = new GlobalVariable(*M,
	//	FunctionType::get(Type::getVoidTy(Context), false),
	//	false,
	//	GlobalValue::CommonLinkage,
	//	0,
//		"HandleOverflow");
	
	llvm::Function* handleOverflowRef = llvm::Function::Create(
		FunctionType::get(Type::getVoidTy(Context), false), 
		Function::ExternalLinkage, "HandleOverflow", M);

	boost::scoped_ptr<ExecutionEngine> EE(EngineBuilder(M).create());
	EE->addGlobalMapping(handleOverflowRef, &HandleOverflow);
	uint64_t *arr;
	arr = new uint64_t[GENERIC_REGISTERS];
	memset(arr, 0, sizeof(*arr) * GENERIC_REGISTERS);
	arr[1] = MmixLlvm::DATA_SEG;
	arr[2] = 32;
	EE->addGlobalMapping(registersGlob, arr);
	
	uint8_t *textSegPhys = new uint8_t[TEXT_SIZE];
	memset(textSegPhys, 0, sizeof(*textSegPhys) * TEXT_SIZE);
	textSegPhys[0x100] = MmixLlvm::LDT;
	textSegPhys[0x101] = 0;
	textSegPhys[0x102] = 1;
	textSegPhys[0x103] = 2;
	textSegPhys[0x104] = MmixLlvm::STHTI;
	textSegPhys[0x105] = 0;
	textSegPhys[0x106] = 1;
	textSegPhys[0x107] = 50;
	EE->addGlobalMapping(textGlob, textSegPhys);

	uint8_t *dataSegPhys = new uint8_t[HEAP_SIZE];
	memset(dataSegPhys, 0, sizeof(*dataSegPhys) * HEAP_SIZE); 
	dataSegPhys[32] = 1;
	dataSegPhys[33] = 2;
	dataSegPhys[34] = 3;
	dataSegPhys[35] = 4;
	dataSegPhys[36] = 5;
	dataSegPhys[37] = 6;
	dataSegPhys[38] = 7;
	dataSegPhys[39] = 8;
	EE->addGlobalMapping(dataGlob, dataSegPhys);

	uint8_t *poolSegPhys = new uint8_t[POOL_SIZE];
	memset(poolSegPhys, 0, sizeof(*poolSegPhys) * POOL_SIZE); 
	EE->addGlobalMapping(poolGlob, poolSegPhys);

	uint8_t *stackSegPhys = new uint8_t[STACK_SIZE];
	memset(stackSegPhys, 0, sizeof(*stackSegPhys) * STACK_SIZE); 
	EE->addGlobalMapping(stackGlob, stackSegPhys);

	boost::scoped_ptr<MmixLlvm::MemAccessor> accessor(
		new MemAccessImpl(llvm::ArrayRef<uint8_t>(textSegPhys, textSegPhys + TEXT_SIZE),
		            llvm::ArrayRef<uint8_t>(dataSegPhys, dataSegPhys + HEAP_SIZE),
					llvm::ArrayRef<uint8_t>(poolSegPhys, poolSegPhys + POOL_SIZE),
					llvm::ArrayRef<uint8_t>(stackSegPhys, stackSegPhys + STACK_SIZE)));

	//MmixLlvm::emitSimpleVertice(Context, *M, *iterArray, 0x100); 
	Function *f;
	MmixLlvm::EdgeList edgeList;

	boost::tie(f, edgeList) = MmixLlvm::emitSimpleVertice(Context, *M, *accessor, 0x100);

	outs() << "We just constructed this LLVM module:\n\n" << *M;
	std::vector<GenericValue> noargs;

	GenericValue gv = EE->runFunction(f, noargs);

	// Import result of execution:
	outs() << "Result: " << gv.IntVal << "\n";
	outs().flush();
	EE->freeMachineCodeForFunction(f);
	llvm_shutdown();
	return 0;
}
