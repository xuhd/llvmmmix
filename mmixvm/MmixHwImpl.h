#pragma once

#include <stdint.h>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include "Engine.h"
#include "MmixDef.h"
#include "MmixEmit.h"
#include "OS.h"

namespace MmixLlvm {
	class MmixHwImpl: public Engine {
		std::vector<uint64_t> _registers;

		std::vector<uint64_t> _spRegisters;

		std::vector<uint8_t> _memory;

		std::vector<uint32_t> _att;

		llvm::Module* _module;

		llvm::LLVMContext _lctx;

		boost::scoped_ptr<llvm::ExecutionEngine> _ee;

		boost::shared_ptr<OS> _os;

		typedef boost::unordered_map<uint64_t, Vertice> VerticeMap;

		VerticeMap _vertices;

		bool _halted;
		
		MmixHwImpl(const HardwareCfg& hwCfg, boost::shared_ptr<OS> os);

		void postInit();

		uint8_t* translateAddr(uint64_t addr, uint8_t mask);

		static void debugInt32(int arg);

		static void debugInt64(int64_t arg);
	
		static void muluImpl(uint64_t arg1, uint64_t arg2, uint64_t* hiProd, uint64_t* loProd);

		static void divuImpl(uint64_t hidivident, uint64_t lodivident, uint64_t divisor, uint64_t* quotient, uint64_t* remainder);
		
		static uint16_t adjust16Endianness(llvm::ArrayRef<uint8_t>& ref);

		static uint32_t adjust32Endianness(llvm::ArrayRef<uint8_t>& ref);

		static uint64_t adjust64Endianness(llvm::ArrayRef<uint8_t>& ref);

		static uint64_t adjust64EndiannessImpl(uint64_t arg);
	public:
		static boost::shared_ptr<MmixHwImpl> create(const HardwareCfg& hwCfg, boost::shared_ptr<OS> os);

		virtual void run(uint64_t xref);

		virtual void halt();

		virtual uint64_t getReg(uint8_t reg);

		virtual void setReg(uint8_t reg, uint64_t value);

		virtual uint64_t getSpReg(MmixLlvm::SpecialReg sreg);

		virtual void setSpReg(MmixLlvm::SpecialReg sreg, uint64_t value);

		virtual uint8_t readByte(uint64_t ref);

		virtual uint16_t readWyde(uint64_t ref);

		virtual uint32_t readTetra(uint64_t ref);

		virtual uint64_t readOcta(uint64_t ref);

		virtual void writeByte(uint64_t ref, uint8_t arg);

		virtual void writeWyde(uint64_t ref, uint16_t arg);

		virtual void writeTetra(uint64_t ref, uint32_t arg);

		virtual void writeOcta(uint64_t ref, uint64_t arg);

		virtual ~MmixHwImpl();
	};
};