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
		void* _handback[1];

		std::vector<MXOcta> _registers;

		std::vector<MXOcta> _spRegisters;

		std::vector<MXByte> _memory;

		std::vector<MXTetra> _att;

		llvm::Module* _module;

		llvm::LLVMContext _lctx;

		boost::scoped_ptr<llvm::ExecutionEngine> _ee;

		boost::shared_ptr<OS> _os;

		typedef boost::unordered_map<MXOcta, Vertice> VerticeMap;

		VerticeMap _vertices;

		bool _halted;
		
		MmixHwImpl(const HardwareCfg& hwCfg, boost::shared_ptr<OS> os);

		void postInit();

		MXByte* translateAddr(MXOcta addr, MXByte mask);

		static void debugInt32(int arg);

		static void debugInt64(int64_t arg);
	
		static void muluImpl(MXOcta arg1, MXOcta arg2, MXOcta* hiProd, MXOcta* loProd);

		static void divuImpl(MXOcta hidivident, MXOcta lodivident, MXOcta divisor, MXOcta* quotient, MXOcta* remainder);

		static MXOcta trapHandlerImpl(void* handback, MXOcta instr, MXOcta vector);

		static MXOcta adjust64EndiannessImpl(MXOcta arg);
	public:
		static boost::shared_ptr<MmixHwImpl> create(const HardwareCfg& hwCfg, boost::shared_ptr<OS> os);

		virtual void run(MXOcta xref);

		virtual void halt();

		virtual MXOcta getReg(MXByte reg);

		virtual void setReg(MXByte reg, MXOcta value);

		virtual MXOcta getSpReg(MmixLlvm::SpecialReg sreg);

		virtual void setSpReg(MmixLlvm::SpecialReg sreg, MXOcta value);

		virtual MXByte readByte(MXOcta ref);

		virtual MXWyde readWyde(MXOcta ref);

		virtual MXTetra readTetra(MXOcta ref);

		virtual MXOcta readOcta(MXOcta ref);

		virtual void writeByte(MXOcta ref, MXByte arg);

		virtual void writeWyde(MXOcta ref, MXWyde arg);

		virtual void writeTetra(MXOcta ref, MXTetra arg);

		virtual void writeOcta(MXOcta ref, MXOcta arg);

		virtual ~MmixHwImpl();
	};
};