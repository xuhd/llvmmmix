#pragma once

#include <stdint.h>

namespace MmixLlvm {
	typedef uint8_t MXByte;

	typedef uint16_t MXWyde;

	typedef uint32_t MXTetra;

	typedef uint64_t MXOcta;

	extern const MXOcta TEXT_SEG;

	extern const MXOcta DATA_SEG;

	extern const MXOcta POOL_SEG;

	extern const MXOcta STACK_SEG;

	extern const MXOcta OS_TRAP_VECTOR;

	enum MmixOpcode {
        //#0			  #1			   #2			  #3			  #4			   #5			  #6			  #7
/*0x#*/ TRAP=0x00,  	  FCMP =0x01,      FUN=0x02,      FEQL=0x03,      FADD=0x04,       FIX=0x05,      FSUB=0x06,      FIXU=0x07,
/*1x#*/	FMUL=0x10,		  FCMPE=0x11, 	   FUNE=0x12,	  FEQLE=0x13,     FDIV=0x14,       FSQRT=0x15,    FREM=0x16,      FINT=0x17,
/*2x#*/	ADD=0x20,         ADDI=0x21,	   ADDU=0x22,     ADDUI=0x23,	  SUB=0x24,        SUBI=0x25,     SUBU=0x26,      SUBUI=0x27,
/*3x#*/ CMP=0x30,         CMPI=0x31,       CMPU=0x32,     CMPUI=0x33,     NEG=0x34,        NEGI=0x35,     NEGU=0x36,      NEGUI=0x37,
/*4x#*/ BN=0x40,          BNB=0x41,        BZ=0x42,       BZB=0x43,       BP=0x44,         BPB=0x45,      BOD=0x46,       BODB=0x47,
/*5x#*/ PBN=0x50,         PBNB=0x51,       PBZ=0x52,      PBZB=0x53,      PBP=0x54,        PBPB=0x55,      PBOD=0x56,      PBODB=0x57,
/*6x#*/ CSN=0x60,         CSNI=0x61,       CSZ=0x62,      CSZI=0x63,      CSP=0x64,        CSPI=0x65,     CSOD=0x66,      CSODI=0x67,
/*7x#*/ ZSN=0x70,         ZSNI=0x71,       ZSZ=0x72,      ZSZI=0x73,      ZSP=0x74,        ZSPI=0x75,     ZSOD=0x76,      ZSODI=0x77,
/*8x#*/ LDB=0x80,         LDBI=0x81,       LDBU=0x82,     LDBUI=0x83,     LDW=0x84,        LDWI=0x85,     LDWU=0x86,      LDWUI=0x87,
/*9x#*/ LDSF=0x90,        LDSFI=0x91,      LDHT=0x92,     LDHTI=0x93,     CSWAP=0x94,      CSWAPI=0x95,   LDUNC=0x96,     LDUNCI=0x97,
/*Ax#*/ STB=0xA0,         STBI=0xA1,       STBU=0xA2,     STBUI=0xA3,     STW=0xA4,        STWI=0xA5,     STWU=0xA6,      STWUI=0xA7,
/*Bx#*/ STSF=0xB0,        STSFI=0xB1,      STHT=0xB2,     STHTI=0xB3,     STCO=0xB4,       STCOI=0xB5,    STUNC=0xB6,     STUNCI=0xB7,
/*Cx#*/ OR=0xC0,          ORI=0xC1,        ORN=0xC2,      ORNI=0xC3,      NOR=0xC4,        NORI=0xC5,     XOR=0xC6,       XORI=0xC7,
/*Dx#*/ BDIF=0xD0,        BDIFI=0xD1,      WDIF=0xD2,     WDIFI=0xD3,     TDIF=0xD4,       TDIFI=0xD5,    ODIF=0xD6,      ODIFI=0xD7,
/*Ex#*/ SETH=0xE0,        SETMH=0xE1,      SETML=0xE2,    SETL=0xE3,      INCH=0xE4,       INCMH=0xE5,    INCML=0xE6,     INCL=0xE7,
/*Fx#*/ JMP=0xF0,         JMPB=0xF1,       PUSHJ=0xF2,    PUSHJB=0xF3,    GETA=0xF4,       GETAB=0xF5,    PUT=0xF6,       PUTI=0xF7,
        //#8              #9               #A             #B              #C               #D             #E              #F
/*0x#*/ FLOT=0x08,        FLOTI=0x09,      FLOTU=0x0A,    FLOTUI=0x0B,    SFLOT=0x0C,      SFLOTI=0x0D,   SFLOTU=0x0E,    SFLOTUI=0x0F,
/*1x#*/ MUL=0x18,         MULI=0x19,       MULU=0x1A,     MULUI=0x1B,     DIV=0x1C,        DIVI=0x1D,     DIVU=0x1E,      DIVUI=0x1F,              
/*2x#*/ _2ADDU=0x28,      _2ADDUI=0x29,    _4ADDU=0x2A,   _4ADDUI=0x2B,   _8ADDU=0x2C,     _8ADDUI=0x2D,  _16ADDU=0x2E,   _16ADDUI=0x2F,           
/*3x#*/ SL=0x38,          SLI=0x39,        SLU=0x3A,      SLUI=0x3B,      SR=0x3C,         SRI=0x3D,      SRU=0x3E,       SRUI=0x3F,               
/*4x#*/ BNN=0x48,         BNNB=0x49,       BNZ=0x4A,      BNZB=0x4B,      BNP=0x4C,        BNPB=0x4D,     BEV=0x4E,       BEVB=0x4F,
/*5x#*/ PBNN=0x58,        PBNNB=0x59,      PBNZ=0x5A,     PBNZB=0x5B,     PBNP=0x5C,       PBNPB=0x5D,    PBEV=0x5E,      PBEVB=0x5F,
/*6x#*/ CSNN=0x68,        CSNNI=0x69,      CSNZ=0x6A,     CSNZI=0x6B,     CSNP=0x6C,       CSNPI=0x6D,    CSEV=0x6E,      CSEVI=0x6F,
/*7x#*/ ZSNN=0x78,        ZSNNI=0x79,      ZSNZ=0x7A,     ZSNZI=0x7B,     ZSNP=0x7C,       ZSNPI=0x7D,    ZSEV=0x7E,      ZSEVI=0x7F,
/*8x#*/ LDT=0x88,         LDTI=0x89,       LDTU=0x8A,     LDTUI=0x8B,     LDO=0x8C,        LDOI=0x8D,     LDOU=0x8E,      LDOUI=0x8F,
/*9x#*/ LDVTS=0x98,       LDVTSI=0x99,     PRELD=0x9A,    PRELDI=0x9B,    PREGO=0x9C,      PREGOI=0x9D,   GO=0x9E,        GOI=0x9F,
/*Ax#*/ STT=0xA8,         STTI=0xA9,       STTU=0xAA,     STTUI=0xAB,     STO=0xAC,        STOI=0xAD,     STOU=0xAE,      STOUI=0xAF,
/*Bx#*/ SYNCD=0xB8,       SYNCDI=0xB9,     PREST=0xBA,    PRESTI=0xBB,    SYNCID=0xBC,     SYNCIDI=0xBD,  PUSHGO=0xBE,    PUSHGOI=0xBF,
/*Cx#*/ AND=0xC8,         ANDI=0xC9,       ANDN=0xCA,     ANDNI=0xCB,     NAND=0xCC,       NANDI=0xCD,    NXOR=0xCE,      NXORI=0xCF,
/*Dx#*/ MUX=0xD8,         MUXI=0xD9,       SADD=0xDA,     SADDI=0xDB,     MOR=0xDC,        MORI=0xDD,     MXOR=0xDE,      MXORI=0xDF,
/*Ex#*/ ORH=0xE8,         ORMH=0xE9,       ORML=0xEA,     ORL=0xEB,       ANDNH=0xEC,      ANDNMH=0xED,   ANDNML=0xEE,    ANDNL=0xEF,
/*Fx#*/ POP=0xF8,         RESUME=0xF9,     SAVE=0xFA,     UNSAVE=0xFB,    SYNC=0xFC,       SWYM=0xFD,     GET=0xFE,       TRIP=0xFF
    };

	enum SpecialReg {
		rA = 21,  rB = 0,   rC = 8,   rD = 1,
        rE = 2,   rF = 22,  rG = 19,  rH = 3,
        rI = 12,  rJ = 4,   rK = 15,  rL = 20,
        rM = 5,   rN = 9,   rO = 10,  rP = 23,
        rQ = 16,  rR = 6,   rS = 11,  rT = 13,
        rU = 17,  rV = 18,  rW = 24,  rX = 25,
        rY = 26,  rZ = 27,  rBB = 7,  rTT = 14,
        rWW = 28, rXX = 29, rYY = 30, rZZ = 31,
	};

	// DVWIOUZX
	enum ArithFlag {
		X = 1,
		Z = 1<<1,
		U = 1<<2,
		O = 1<<3,
		I = 1<<4,
		W = 1<<5,
		V = 1<<6,
		D = 1<<7
	};

	struct HardwareCfg {
		size_t TextSize;
		
		size_t HeapSize;
		
		size_t PoolSize;

		size_t StackSize;
	};
};