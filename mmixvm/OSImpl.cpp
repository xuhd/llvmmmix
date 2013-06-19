#include "stdafx.h"
#include "OSImpl.h"
#include "Util.h"
#include "MemAccess.h"
#include "MmixDef.h"
#include "MmixHwImpl.h"
#include "MmixEmit.h"
#include "Engine.h"
#include <windows.h>

using MmixLlvm::Engine;
using MmixLlvm::OSImpl;
using llvm::ArrayRef;
using MmixLlvm::MXOcta;

OSImpl::OSImpl(const std::vector<std::wstring >& argv)
	:_argv(argv)
{}

void OSImpl::loadExecutable(Engine& e) {
	e.setSpReg(MmixLlvm::rT, MmixLlvm::OS_TRAP_VECTOR);
	MXOcta topDataSegAddr = MmixLlvm::DATA_SEG;
	std::ifstream f(_argv[0], std::ios::binary);
	while (!f.eof()) {
		bool isGreg = false;
		MXOcta loc = 0;
		size_t size = 0;
		readSection(e, f, isGreg, loc, size);
		if (!isGreg) {
			bool isDataSeg = loc >= MmixLlvm::DATA_SEG && loc < MmixLlvm::POOL_SEG;
			MXOcta segTop = loc + size;
			if (isDataSeg && topDataSegAddr < segTop)
				topDataSegAddr = segTop;
		}
	}
	makeArgv(e, topDataSegAddr);
}

void OSImpl::makeArgv(Engine& e, MXOcta topDataSegAddr) {
	std::vector<MXByte> result;
	std::vector<size_t> offsets;
	size_t curPos = 0;
	for (std::vector< std::wstring >::const_iterator itr = _argv.cbegin();
		itr != _argv.cend();
		++itr)
	{
		std::vector<MXByte> asBytes(MmixLlvm::Util::stringToBytes(*itr, CP_UTF8));
		size_t tail = asBytes.size() % 8;
		size_t alignedSize = asBytes.size() + (tail > 0 ? 8 - tail : 0);
		asBytes.resize(alignedSize);
		std::copy(asBytes.begin(), asBytes.end(), std::back_inserter(result));
		offsets.push_back(curPos);
		curPos = curPos + alignedSize;
	}
	MXOcta ref = topDataSegAddr;
	MXOcta argsRef = ref + (offsets.size() + 1) * 8;
	for (std::vector<size_t>::iterator itr = offsets.begin();
		itr != offsets.end();
		++itr) 
	{
		e.writeOcta (ref, argsRef + *itr);
		ref += 8;
	}
	e.writeOcta (ref, 0);
	ref += 8;
	for (size_t i = 0; i < result.size(); i++)
		e.writeByte(ref + i, result[i]);
	e.setReg(0, _argv.size());
	e.setReg(1, topDataSegAddr);
}

MXOcta OSImpl::handleTrap(Engine& e, MXOcta instr, MXOcta vector) {
	enum {Fopen = 1,  Fclose = 2,
		  Fread = 3,  Fgets = 4,
		  Fgetws = 5, Fwrite = 6,
		  Fputs = 7,  Fputws = 8,
		  Fseek = 9,  Ftell = 10
	};
	switch (e.getSpReg(MmixLlvm::rYY)) 
	{
	case Fopen:
		break;
	case Fclose:
		break;
	case Fread:
		break;
	case Fgets:
		break;
	case Fgetws:
		break;
	case Fwrite:
		break;
	case Fputs:
		doFputs(e, e.getSpReg(rBB), e.getSpReg(rXX), e.getSpReg(rYY), e.getSpReg(rZZ));
		break;
	case Fputws:
		break;
	case Fseek:
		break;
	case Ftell:
		break;
	default:
		e.halt();
	}
	return e.getSpReg(MmixLlvm::rWW);
}

void OSImpl::doFputs(Engine& e, MXOcta rBB, MXOcta rXX, MXOcta rYY, MXOcta rZZ) {
	MXOcta p0 = rBB;
	MXByte c;
	std::vector<MXByte> tmp;
	while ((c = e.readByte(p0++)) != 0)
		tmp.push_back(c);
	HANDLE h;
	switch (rZZ) {
	case 1:
		h = GetStdHandle(STD_OUTPUT_HANDLE);
		break;
	case 2:
		h = GetStdHandle(STD_ERROR_HANDLE);
		break;
	default:
		h = (HANDLE)rZZ;
	}
	DWORD bytesWritten;
	::WriteFile(h, &tmp[0], tmp.size(), &bytesWritten, NULL);
}

void OSImpl::readSection(Engine& e, std::istream& stream, bool& isGreg, MXOcta& loc, size_t& size) {
	enum {SECTION_ALIGN = 0x20};
	MXByte buff[SECTION_ALIGN];
	stream.read((char*)buff, 8);
	if (stream) {
		if (strcmp((char*)buff, ".greg") != 0) {
			stream.read((char*)buff, SECTION_ALIGN - 8);
			MXOcta loBound = MmixLlvm::Util::adjust64Endianness(ArrayRef<MXByte>(buff, buff + 8));
			MXOcta hiBound = MmixLlvm::Util::adjust64Endianness(ArrayRef<MXByte>(buff + 8, buff + 16));
			size_t sectionSize = hiBound - loBound;
			size_t tail = sectionSize % SECTION_ALIGN;
			size_t alignedSectionSize = sectionSize + (tail > 0 ? (SECTION_ALIGN - tail) : 0);
			std::vector<MXByte> tmp(alignedSectionSize);
			ArrayRef<MXByte> buff1(tmp);
			stream.read((char*)&buff1[0], alignedSectionSize);
			for (size_t i = 0; i < sectionSize; i += 4) {
				MXOcta tetraLoc = loBound + i;
				e.writeTetra(tetraLoc, MmixLlvm::Util::adjust32Endianness(buff1.slice(i, 4))); 
			}
			isGreg = false;
			loc = loBound;
			size = alignedSectionSize;
		} else {
			isGreg = true;
			loc = 0;
			size = 0;
			stream.read((char*)buff, 8);
			MXOcta lastGreg = MmixLlvm::Util::adjust64Endianness(ArrayRef<MXByte>(buff, buff + 8));
			size_t s0 = 16 + (255 - lastGreg) * 8;
			size_t tail = s0 % SECTION_ALIGN;
			size_t alignedSectionSize = s0 + (tail > 0 ? (SECTION_ALIGN - tail) : 0);
			size_t bytesRead = 16;
			for (MXOcta i = lastGreg; i < 255ULL; i++) {
				stream.read((char*)buff, 8);
				bytesRead += 8;
				MXOcta gregVal = MmixLlvm::Util::adjust64Endianness(ArrayRef<MXByte>(buff, buff + 8));
				e.setReg((MXByte)i, gregVal);
			}
			while (bytesRead < alignedSectionSize) {
				stream.read((char*)buff, 8);
				bytesRead += 8;
			}
		}
	}
}

OSImpl::~OSImpl() { }
