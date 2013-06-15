#pragma once

#include <fstream>
#include <vector>
#include "MmixDef.h"
#include "Engine.h"
#include "OS.h"

namespace MmixLlvm {
	class OSImpl: public MmixLlvm::OS {
		const std::vector< std::wstring > _argv;
		static void readSection(Engine& e, std::istream& stream, bool& isGreg, uint64_t& loc, size_t& size);
		void makeArgv(Engine& e, uint64_t topDataSegAddr);
		void doFputs(Engine& e, uint64_t rBB, uint64_t rXX, uint64_t rYY, uint64_t rZZ);
	public:
		OSImpl(const std::vector< std::wstring >& argv);
		virtual void loadExecutable(Engine& e);
		virtual void handleTrap(Engine& e, uint64_t vector);
		virtual ~OSImpl();
	};
}