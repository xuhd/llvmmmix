// mmixvm.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MemAccess.h"
#include "MmixDef.h"
#include "MmixHwImpl.h"
#include "MmixEmit.h"
#include "Engine.h"
#include "OS.h"

namespace {
	class OSImpl: public MmixLlvm::OS {
	public:
		virtual void handleTrap(MmixLlvm::Engine& e);
		virtual ~OSImpl();
	};

	void OSImpl::handleTrap(MmixLlvm::Engine& e) {
		e.halt();
	}

	OSImpl::~OSImpl() { }
};

int _tmain(int argc, _TCHAR* argv[])
{
	llvm::InitializeNativeTarget();
	MmixLlvm::HardwareCfg cfg;
	enum { _16K = 16 * 1024 };
	cfg.TextSize = cfg.HeapSize = cfg.PoolSize = cfg.StackSize = _16K;
	boost::shared_ptr<MmixLlvm::Engine> theHw(MmixLlvm::MmixHwImpl::create(cfg,
		boost::shared_ptr<MmixLlvm::OS>(new OSImpl)));
	(*theHw).setSpReg(MmixLlvm::rT, MmixLlvm::OS_TRAP_VECTOR);
	(*theHw).run(0x100);
	llvm::llvm_shutdown();
	return 0;
}
