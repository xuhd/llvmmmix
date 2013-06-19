#pragma once

#include <fstream>
#include <vector>
#include "MmixDef.h"
#include "Engine.h"
#include "OS.h"

namespace MmixLlvm {
	class OSImpl: public MmixLlvm::OS {
		const std::vector< std::wstring > _argv;
		static void readSection(Engine& e, std::istream& stream, bool& isGreg, MXOcta& loc, size_t& size);
		void makeArgv(Engine& e, MXOcta topDataSegAddr);
		void doFputs(Engine& e, MXOcta rBB, MXOcta rXX, MXOcta rYY, MXOcta rZZ);
	public:
		OSImpl(const std::vector< std::wstring >& argv);
		virtual void loadExecutable(Engine& e);
		virtual MXOcta handleTrap(Engine& e, MXOcta instr, MXOcta vector);
		virtual ~OSImpl();
	};
}