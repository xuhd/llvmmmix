#include "stdafx.h"
#include "MmixDef.h"

using MmixLlvm::MXOcta;

const MXOcta MmixLlvm::TEXT_SEG = 0x0000000000000000ULL;

const MXOcta MmixLlvm::DATA_SEG = 0x2000000000000000ULL;

const MXOcta MmixLlvm::POOL_SEG = 0x4000000000000000ULL;

const MXOcta MmixLlvm::STACK_SEG = 0x6000000000000000ULL;

const MXOcta MmixLlvm::OS_TRAP_VECTOR = 0x8000000000000000ULL;
