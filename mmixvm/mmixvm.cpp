// mmixvm.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MmixHwImpl.h"
#include "Engine.h"
#include "OSImpl.h"
#include <windows.h>

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc >= 2) {
		llvm::InitializeNativeTarget();
		MmixLlvm::HardwareCfg cfg;
		enum { _16K = 16 * 1024 };
		cfg.TextSize = cfg.HeapSize = cfg.PoolSize = cfg.StackSize = _16K;
		std::vector< std::wstring > argv0;
		for (int i = 1; i < argc; i++)
			argv0.push_back(std::wstring(argv[i]));
		boost::shared_ptr<MmixLlvm::Engine> theHw(MmixLlvm::MmixHwImpl::create(cfg,
			boost::shared_ptr<MmixLlvm::OS>(new MmixLlvm::OSImpl(argv0))));
		theHw->run(0x100);
		llvm::llvm_shutdown();
	}
	return 0;
}
