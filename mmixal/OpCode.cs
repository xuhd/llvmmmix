using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

/*
	#0			  #1			   #2			  #3			  #4			   #5			  #6			  #7
0x	TRAP  	  	  FCMP             FUN            FEQL            FADD             FIX            FSUB            FIXU
	FLOT[I]                        FLOTU[I]                       SFLOT[I]                        SFLOTU[I]
1x	FMUL		  FCMPE 		   FUNE			  FEQLE			  FDIV             FSQRT          FREM            FINT
	MUL[I]		  				   MULU[I]						  DIV[I]		   				  DIVU[I]              
2x  ADD[I]						   ADDU[I]						  SUB[I]						  SUBU[I]
    2ADDU[I]					   4ADDU[I] 					  8ADDU[I]						  16ADDU[I]           
3x  CMP[I]                         CMPU[I]                        NEG[I]                          NEGU[I]                 
    SL[I]                          SLU[I]                         SR[I]                           SRU[I]               
#4x BN[B]                          BZ[B]                          BP[B]                           BOD[B]
    BNN[B]                         BNZ[B]                         BNP[B]                          BEV[B]
#5x PBN[B]                         PBZ[B]                         PBP[B]                          PBOD[B]
    PBNN[B]                        PBNZ[B]                        PBNP[B]                         PBEV[B]
#6x CSN[I]                         CSZ[I]                         CSP[I]                          CSOD[I]
    CSNN[I]                        CSNZ[I]                        CSNP[I]                         CSEV[I
#7x ZSN[I]                         ZSZ[I]                         ZSP[I]                          ZSOD[I]
    ZSNN[I]                        ZSNZ[I]                        ZSNP[I]                         ZSEV[I]
#8x LDB[I]                         LDBU[I]                        LDW[I]                          LDWU[I]
    LDT[I]                         LDTU[I]                        LDO[I]                          LDOU[I]
#9x LDSF[I]                        LDHT[I]                        CSWAP[I]                        LDUNC[I]
    LDVTS[I]                       PRELD[I]                       PREGO[I]                        GO[I]
#Ax STB[I]                         STBU[I]                        STW[I]                          STWU[I]
    STT[I]                         STTU[I]                        STO[I]                          STOU[I]
#Bx STSF[I]                        STHT[I]                        STCO[I]                         STUNC[I]
    SYNCD[I]                       PREST[I]                       SYNCID[I]                       PUSHGO[I]
#Cx OR[I]                          ORN[I]                         NOR[I]                          XOR[I]
    AND[I]                         ANDN[I]                        NAND[I]                         NXOR[I]
#Dx BDIF[I]                        WDIF[I]                        TDIF[I]                         ODIF[I]
    MUX[I]                         SADD[I]                        MOR[I]                          MXOR[I
#Ex SETH          SETMH            SETML          SETL            INCH             INCMH          INCML           INCL
    ORH           ORMH             ORML           ORL             ANDNH            ANDNMH         ANDNML          ANDNL
#Fx JMP[B]                         PUSHJ[B]                       GETA[B]                         PUT[I]
    POP           RESUME           [UN]SAVE                       SYNC            SWYM            GET             TRIP
    #8            #9               #A             #B              #C              #D              #E              #F
*/

namespace mmixal {
    public enum MmixOpcode {
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
/*Fx#*/ POP=0xF8,         RESUME=0xF9,     SAVE=0xFA,     UNSAVE=0xFB,    SYNC=0xFC,       SWYM=0xFD,     GET=0xFE,       TRIP=0xFF,
        ILLEGAL=0x100
    }

    public enum MmixOpcodeCategory {
        NonStrict,
        ThreeRegs,
        TwoRegsAndImmediate,
        RegAndWydeImmediate,
        Branch,
        Jump,
        GetSpecialRegister,
        PutSpecialRegister
    }

    public static class OpcodeUtil {
        private static readonly Dictionary<string, MmixOpcode> THE_DICT =
            new Tuple<string, MmixOpcode>[]{
                Tuple.Create("TRAP", MmixOpcode.TRAP),
                Tuple.Create("FCMP", MmixOpcode.FCMP),
                Tuple.Create("FUN", MmixOpcode.FUN),
                Tuple.Create("FEQL", MmixOpcode.FEQL),
                Tuple.Create("FADD", MmixOpcode.FADD),
                Tuple.Create("FIX", MmixOpcode.FIX),
                Tuple.Create("FSUB", MmixOpcode.FSUB),
                Tuple.Create("FIXU", MmixOpcode.FIXU),
                Tuple.Create("FLOT", MmixOpcode.FLOT),
                Tuple.Create("FLOTU", MmixOpcode.FLOTU),
                Tuple.Create("SFLOT", MmixOpcode.SFLOT),
                Tuple.Create("SFLOTU", MmixOpcode.SFLOTU),
                Tuple.Create("FMUL", MmixOpcode.FMUL),
                Tuple.Create("FCMPE", MmixOpcode.FCMPE),
                Tuple.Create("FUNE", MmixOpcode.FUNE),
                Tuple.Create("FEQLE", MmixOpcode.FEQLE),
                Tuple.Create("FDIV", MmixOpcode.FDIV),
                Tuple.Create("FSQRT", MmixOpcode.FSQRT),
                Tuple.Create("FREM", MmixOpcode.FREM),
                Tuple.Create("FINT", MmixOpcode.FINT),
                Tuple.Create("MUL", MmixOpcode.MUL),
                Tuple.Create("MULU", MmixOpcode.MULU),
                Tuple.Create("DIV", MmixOpcode.DIV),
                Tuple.Create("DIVU", MmixOpcode.DIVU),
                Tuple.Create("ADD", MmixOpcode.ADD),
                Tuple.Create("ADDU", MmixOpcode.ADDU),
                Tuple.Create("SUB", MmixOpcode.SUB),
                Tuple.Create("SUBU", MmixOpcode.SUBU),
                Tuple.Create("2ADDU", MmixOpcode._2ADDU),
                Tuple.Create("4ADDU", MmixOpcode._4ADDU),
                Tuple.Create("8ADDU", MmixOpcode._8ADDU),
                Tuple.Create("16ADDU", MmixOpcode._16ADDU),
                Tuple.Create("CMP", MmixOpcode.CMP),                         
                Tuple.Create("CMPU", MmixOpcode.CMPU),
                Tuple.Create("NEG", MmixOpcode.NEG),
                Tuple.Create("NEGU", MmixOpcode.NEGU),
                Tuple.Create("SL", MmixOpcode.SL),
                Tuple.Create("SLU", MmixOpcode.SLU),
                Tuple.Create("SR", MmixOpcode.SR),
                Tuple.Create("SRU", MmixOpcode.SRU),
                Tuple.Create("BN", MmixOpcode.BN),
                Tuple.Create("BZ", MmixOpcode.BZ),
                Tuple.Create("BP", MmixOpcode.BP),
                Tuple.Create("BOD", MmixOpcode.BOD),
                Tuple.Create("BNN", MmixOpcode.BNN),
                Tuple.Create("BNZ", MmixOpcode.BNZ),
                Tuple.Create("BNP", MmixOpcode.BNP),
                Tuple.Create("BEV", MmixOpcode.BEV),
                Tuple.Create("PBN", MmixOpcode.PBN),
                Tuple.Create("PBZ", MmixOpcode.PBZ),
                Tuple.Create("PBP", MmixOpcode.PBP),
                Tuple.Create("PBOD", MmixOpcode.PBOD),
                Tuple.Create("PBNN", MmixOpcode.PBNN),
                Tuple.Create("PBNZ", MmixOpcode.PBNZ),
                Tuple.Create("PBNP", MmixOpcode.PBNP),
                Tuple.Create("PBEV", MmixOpcode.PBEV),
                Tuple.Create("CSN", MmixOpcode.CSN),
                Tuple.Create("CSZ", MmixOpcode.CSZ),
                Tuple.Create("CSP", MmixOpcode.CSP),
                Tuple.Create("CSOD", MmixOpcode.CSOD),
                Tuple.Create("CSNN", MmixOpcode.CSNN),
                Tuple.Create("CSNZ", MmixOpcode.CSNZ),
                Tuple.Create("CSNP", MmixOpcode.CSNP),
                Tuple.Create("CSEV", MmixOpcode.CSEV),
                Tuple.Create("ZSN", MmixOpcode.ZSN),
                Tuple.Create("ZSZ", MmixOpcode.ZSN),
                Tuple.Create("ZSP", MmixOpcode.ZSP),
                Tuple.Create("ZSOD", MmixOpcode.ZSP),
                Tuple.Create("ZSNN", MmixOpcode.ZSNN),
                Tuple.Create("ZSNZ", MmixOpcode.ZSNZ),
                Tuple.Create("ZSNP", MmixOpcode.ZSNP),
                Tuple.Create("ZSEV", MmixOpcode.ZSEV),
                Tuple.Create("LDB", MmixOpcode.LDB),
                Tuple.Create("LDBU", MmixOpcode.LDBU),
                Tuple.Create("LDW", MmixOpcode.LDW),
                Tuple.Create("LDWU", MmixOpcode.LDWU),
                Tuple.Create("LDT", MmixOpcode.LDT),
                Tuple.Create("LDTU", MmixOpcode.LDTU),
                Tuple.Create("LDO", MmixOpcode.LDO),
                Tuple.Create("LDOU", MmixOpcode.LDOU),
                Tuple.Create("LDSF", MmixOpcode.LDSF),
                Tuple.Create("LDHT", MmixOpcode.LDHT),
                Tuple.Create("CSWAP", MmixOpcode.CSWAP),
                Tuple.Create("LDUNC", MmixOpcode.LDUNC),
                Tuple.Create("LDVTS", MmixOpcode.LDVTS),
                Tuple.Create("PRELD", MmixOpcode.PRELD),
                Tuple.Create("PREGO", MmixOpcode.PREGO),
                Tuple.Create("GO", MmixOpcode.GO),
                Tuple.Create("STB", MmixOpcode.STB),                         
                Tuple.Create("STBU", MmixOpcode.STBU),
                Tuple.Create("STW", MmixOpcode.STW),
                Tuple.Create("STWU", MmixOpcode.STWU),
                Tuple.Create("STT", MmixOpcode.STT),
                Tuple.Create("STTU", MmixOpcode.STTU),
                Tuple.Create("STO", MmixOpcode.STO),
                Tuple.Create("STOU", MmixOpcode.STOU),
                Tuple.Create("STSF", MmixOpcode.STSF),
                Tuple.Create("STHT", MmixOpcode.STHT),
                Tuple.Create("STCO", MmixOpcode.STCO),
                Tuple.Create("STUNC", MmixOpcode.STUNC),
                Tuple.Create("SYNCD", MmixOpcode.SYNCD),
                Tuple.Create("PREST", MmixOpcode.PREST),
                Tuple.Create("SYNCID", MmixOpcode.SYNCID),
                Tuple.Create("PUSHGO", MmixOpcode.PUSHGO),
                Tuple.Create("OR", MmixOpcode.OR),
                Tuple.Create("ORN", MmixOpcode.ORN),
                Tuple.Create("NOR", MmixOpcode.NOR),
                Tuple.Create("XOR", MmixOpcode.XOR),
                Tuple.Create("AND", MmixOpcode.AND),
                Tuple.Create("ANDN", MmixOpcode.ANDN),
                Tuple.Create("NAND", MmixOpcode.NAND),
                Tuple.Create("NXOR", MmixOpcode.NXOR),
                Tuple.Create("BDIF", MmixOpcode.BDIF),
                Tuple.Create("WDIF", MmixOpcode.WDIF),
                Tuple.Create("TDIF", MmixOpcode.TDIF),
                Tuple.Create("ODIF", MmixOpcode.ODIF),
                Tuple.Create("MUX", MmixOpcode.MUX),
                Tuple.Create("SADD", MmixOpcode.SADD),
                Tuple.Create("MOR", MmixOpcode.MOR),
                Tuple.Create("MXOR", MmixOpcode.MXOR),
                Tuple.Create("SETH", MmixOpcode.SETH),
                Tuple.Create("SETMH", MmixOpcode.SETMH),
                Tuple.Create("SETML", MmixOpcode.SETML),
                Tuple.Create("SETL", MmixOpcode.SETL),
                Tuple.Create("INCH", MmixOpcode.INCH),
                Tuple.Create("INCMH", MmixOpcode.INCMH),
                Tuple.Create("INCML", MmixOpcode.INCML),
                Tuple.Create("INCL", MmixOpcode.INCL),
                Tuple.Create("ORH", MmixOpcode.ORH),
                Tuple.Create("ORMH", MmixOpcode.ORMH),
                Tuple.Create("ORML", MmixOpcode.ORML),
                Tuple.Create("ORL", MmixOpcode.ORL),
                Tuple.Create("ANDNH", MmixOpcode.ANDNH),
                Tuple.Create("ANDNMH", MmixOpcode.ANDNMH),
                Tuple.Create("ANDNML", MmixOpcode.ANDNML),
                Tuple.Create("ANDNL", MmixOpcode.ANDNL),
                Tuple.Create("JMP", MmixOpcode.JMP),
                Tuple.Create("PUSHJ", MmixOpcode.PUSHJ),
                Tuple.Create("GETA", MmixOpcode.GETA),
                Tuple.Create("PUT", MmixOpcode.PUT),
                Tuple.Create("POP", MmixOpcode.POP),
                Tuple.Create("RESUME", MmixOpcode.RESUME),
                Tuple.Create("SAVE", MmixOpcode.SAVE),
                Tuple.Create("UNSAVE", MmixOpcode.UNSAVE),
                Tuple.Create("SYNC", MmixOpcode.SYNC),
                Tuple.Create("SWYM", MmixOpcode.SWYM),
                Tuple.Create("GET", MmixOpcode.GET),
                Tuple.Create("TRIP", MmixOpcode.TRIP)
            }.ToDictionary(
                (Tuple<string, MmixOpcode> arg) => arg.Item1,
                (Tuple<string, MmixOpcode> arg) => arg.Item2);

        public static MmixOpcode fromString(string arg) {
            MmixOpcode retVal;
            return THE_DICT.TryGetValue(arg, out retVal) ? retVal : MmixOpcode.ILLEGAL;
        }

        public static MmixOpcode getImmediateCounterpart(MmixOpcode arg) {
            switch (arg) {
                case MmixOpcode.FLOT:
                    return MmixOpcode.FLOTI;
                case MmixOpcode.FLOTU:
                    return MmixOpcode.FLOTUI;
                case MmixOpcode.SFLOT:
                    return MmixOpcode.SFLOTI;
                case MmixOpcode.SFLOTU:
                    return MmixOpcode.SFLOTUI;
                case MmixOpcode.MUL:
                    return MmixOpcode.MULI;
                case MmixOpcode.MULU:
                    return MmixOpcode.MULUI;
                case MmixOpcode.DIV:
                    return MmixOpcode.DIVI;
                case MmixOpcode.DIVU:
                    return MmixOpcode.DIVUI;
                case MmixOpcode.ADD:
                    return MmixOpcode.ADDI;
                case MmixOpcode.ADDU:
                    return MmixOpcode.ADDUI;
                case MmixOpcode.SUB:
                    return MmixOpcode.SUBI;
                case MmixOpcode.SUBU:
                    return MmixOpcode.SUBUI;
                case MmixOpcode._2ADDU:
                    return MmixOpcode._2ADDUI;
                case MmixOpcode._4ADDU:
                    return MmixOpcode._4ADDUI;
                case MmixOpcode._8ADDU:
                    return MmixOpcode._8ADDUI;
                case MmixOpcode._16ADDU:
                    return MmixOpcode._16ADDUI;
                case MmixOpcode.CMP:
                    return MmixOpcode.CMPI;
                case MmixOpcode.CMPU:
                    return MmixOpcode.CMPUI;
                case MmixOpcode.NEG:
                    return MmixOpcode.NEGI;
                case MmixOpcode.NEGU:
                    return MmixOpcode.NEGUI;
                case MmixOpcode.SL:
                    return MmixOpcode.SLI;
                case MmixOpcode.SLU:
                    return MmixOpcode.SLUI;
                case MmixOpcode.SR:
                    return MmixOpcode.SRI;
                case MmixOpcode.SRU:
                    return MmixOpcode.SRUI;
                case MmixOpcode.CSN:
                    return MmixOpcode.CSNI;
                case MmixOpcode.CSZ:
                    return MmixOpcode.CSZI;
                case MmixOpcode.CSP:
                    return MmixOpcode.CSPI;
                case MmixOpcode.CSOD:
                    return MmixOpcode.CSODI;
                case MmixOpcode.CSNN:
                    return MmixOpcode.CSNI;
                case MmixOpcode.CSNZ:
                    return MmixOpcode.CSNZI;
                case MmixOpcode.CSNP:
                    return MmixOpcode.CSNPI;
                case MmixOpcode.CSEV:
                    return MmixOpcode.CSEVI;
                case MmixOpcode.ZSN:
                    return MmixOpcode.ZSNI;
                case MmixOpcode.ZSZ:
                    return MmixOpcode.ZSZI;
                case MmixOpcode.ZSP:
                    return MmixOpcode.ZSPI;
                case MmixOpcode.ZSOD:
                    return MmixOpcode.ZSODI;
                case MmixOpcode.ZSNN:
                    return MmixOpcode.ZSNNI;
                case MmixOpcode.ZSNZ:
                    return MmixOpcode.ZSNZI;
                case MmixOpcode.ZSNP:
                    return MmixOpcode.ZSNPI;
                case MmixOpcode.ZSEV:
                    return MmixOpcode.ZSEVI;
                case MmixOpcode.LDB:
                    return MmixOpcode.LDBI;
                case MmixOpcode.LDBU:
                    return MmixOpcode.LDBUI;
                case MmixOpcode.LDW:
                    return MmixOpcode.LDWI;
                case MmixOpcode.LDWU:
                    return MmixOpcode.LDWUI;
                case MmixOpcode.LDT:
                    return MmixOpcode.LDTI;
                case MmixOpcode.LDTU:
                    return MmixOpcode.LDTUI;
                case MmixOpcode.LDO:
                    return MmixOpcode.LDOI;
                case MmixOpcode.LDOU:
                    return MmixOpcode.LDOUI;
                case MmixOpcode.LDSF:
                    return MmixOpcode.LDSFI;
                case MmixOpcode.LDHT:
                    return MmixOpcode.LDHTI;
                case MmixOpcode.CSWAP:
                    return MmixOpcode.CSWAPI;
                case MmixOpcode.LDUNC:
                    return MmixOpcode.LDUNCI;
                case MmixOpcode.LDVTS:
                    return MmixOpcode.LDVTSI;
                case MmixOpcode.PRELD:
                    return MmixOpcode.PRELDI;
                case MmixOpcode.PREGO:
                    return MmixOpcode.PREGOI;
                case MmixOpcode.GO:
                    return MmixOpcode.GOI;
                case MmixOpcode.STB:
                    return MmixOpcode.STBI;
                case MmixOpcode.STBU:
                    return MmixOpcode.STBUI;
                case MmixOpcode.STW:
                    return MmixOpcode.STWI;
                case MmixOpcode.STWU:
                    return MmixOpcode.STWUI;
                case MmixOpcode.STT:
                    return MmixOpcode.STTI;
                case MmixOpcode.STTU:
                    return MmixOpcode.STTUI;
                case MmixOpcode.STO:
                    return MmixOpcode.STOI;
                case MmixOpcode.STOU:
                    return MmixOpcode.STOUI;
                case MmixOpcode.STSF:
                    return MmixOpcode.STSFI;
                case MmixOpcode.STHT:
                    return MmixOpcode.STHTI;
                case MmixOpcode.STCO:
                    return MmixOpcode.STCOI;
                case MmixOpcode.STUNC:
                    return MmixOpcode.STUNCI;
                case MmixOpcode.SYNCD:
                    return MmixOpcode.SYNCDI;
                case MmixOpcode.PREST:
                    return MmixOpcode.PRESTI;
                case MmixOpcode.SYNCID:
                    return MmixOpcode.SYNCIDI;
                case MmixOpcode.PUSHGO:
                    return MmixOpcode.PUSHGOI;
                case MmixOpcode.OR:
                    return MmixOpcode.ORI;
                case MmixOpcode.ORN:
                    return MmixOpcode.ORNI;
                case MmixOpcode.NOR:
                    return MmixOpcode.NORI;
                case MmixOpcode.XOR:
                    return MmixOpcode.XORI;
                case MmixOpcode.AND:
                    return MmixOpcode.ANDI;
                case MmixOpcode.ANDN:
                    return MmixOpcode.ANDNI;
                case MmixOpcode.NAND:
                    return MmixOpcode.NANDI;
                case MmixOpcode.NXOR:
                    return MmixOpcode.NXORI;
                case MmixOpcode.BDIF:
                    return MmixOpcode.BDIFI;
                case MmixOpcode.WDIF:
                    return MmixOpcode.WDIFI;
                case MmixOpcode.TDIF:
                    return MmixOpcode.TDIFI;
                case MmixOpcode.ODIF:
                    return MmixOpcode.ODIFI;
                case MmixOpcode.MUX:
                    return MmixOpcode.MUXI;
                case MmixOpcode.SADD:
                    return MmixOpcode.SADDI;
                case MmixOpcode.MOR:
                    return MmixOpcode.MORI;
                case MmixOpcode.MXOR:
                    return MmixOpcode.MXORI;
                case MmixOpcode.PUT:
                    return MmixOpcode.PUTI;
                default:
                    return MmixOpcode.ILLEGAL;
            }
        }

        public static MmixOpcode getBackwardCounterpart(MmixOpcode arg) {
            switch (arg) {
                case MmixOpcode.BN:
                    return MmixOpcode.BNB;
                case MmixOpcode.BZ:
                    return MmixOpcode.BZB;
                case MmixOpcode.BP:
                    return MmixOpcode.BPB;
                case MmixOpcode.BOD:
                    return MmixOpcode.BODB;
                case MmixOpcode.BNN:
                    return MmixOpcode.BNNB;
                case MmixOpcode.BNZ:
                    return MmixOpcode.BNZB;
                case MmixOpcode.BNP:
                    return MmixOpcode.BNPB;
                case MmixOpcode.BEV:
                    return MmixOpcode.BEVB;
                case MmixOpcode.PBN:
                    return MmixOpcode.PBNB;
                case MmixOpcode.PBZ:
                    return MmixOpcode.PBZB;
                case MmixOpcode.PBP:
                    return MmixOpcode.PBPB;
                case MmixOpcode.PBOD:
                    return MmixOpcode.PBODB;
                case MmixOpcode.PBNN:
                    return MmixOpcode.PBNNB;
                case MmixOpcode.PBNZ:
                    return MmixOpcode.PBNZB;
                case MmixOpcode.PBNP:
                    return MmixOpcode.PBNPB;
                case MmixOpcode.PBEV:
                    return MmixOpcode.PBEVB;
                case MmixOpcode.JMP:
                    return MmixOpcode.JMPB;
                case MmixOpcode.PUSHJ:
                    return MmixOpcode.PUSHJB;
                case MmixOpcode.GETA:
                    return MmixOpcode.GETAB;
                default:
                    return MmixOpcode.ILLEGAL;
            }
        }

        /*

  
*/
        public static MmixOpcodeCategory getOpcodeCategory(MmixOpcode arg) {
            switch (arg) {
                case MmixOpcode.FLOT:
                case MmixOpcode.FLOTU:
                case MmixOpcode.SFLOT:
                case MmixOpcode.SFLOTU:
                case MmixOpcode.MUL:
                case MmixOpcode.MULU:
                case MmixOpcode.DIV:
                case MmixOpcode.DIVU:
                case MmixOpcode.ADD:
                case MmixOpcode.ADDU:
                case MmixOpcode.SUB:
                case MmixOpcode.SUBU:
                case MmixOpcode._2ADDU:
                case MmixOpcode._4ADDU:
                case MmixOpcode._8ADDU:
                case MmixOpcode._16ADDU:
                case MmixOpcode.CMP:
                case MmixOpcode.CMPU:
                case MmixOpcode.NEG:
                case MmixOpcode.NEGU:
                case MmixOpcode.SL:
                case MmixOpcode.SLU:
                case MmixOpcode.SR:
                case MmixOpcode.SRU:
                case MmixOpcode.CSN:
                case MmixOpcode.CSZ:
                case MmixOpcode.CSP:
                case MmixOpcode.CSOD:
                case MmixOpcode.CSNN:
                case MmixOpcode.CSNZ:
                case MmixOpcode.CSNP:
                case MmixOpcode.CSEV:
                case MmixOpcode.ZSN:
                case MmixOpcode.ZSZ:
                case MmixOpcode.ZSP:
                case MmixOpcode.ZSOD:
                case MmixOpcode.ZSNN:
                case MmixOpcode.ZSNZ:
                case MmixOpcode.ZSNP:
                case MmixOpcode.ZSEV:
                case MmixOpcode.LDB:
                case MmixOpcode.LDBU:
                case MmixOpcode.LDW:
                case MmixOpcode.LDWU:
                case MmixOpcode.LDT:
                case MmixOpcode.LDTU:
                case MmixOpcode.LDO:
                case MmixOpcode.LDOU:
                case MmixOpcode.LDSF:
                case MmixOpcode.LDHT:
                case MmixOpcode.CSWAP:
                case MmixOpcode.LDUNC:
                case MmixOpcode.LDVTS:
                case MmixOpcode.PRELD:
                case MmixOpcode.PREGO:
                case MmixOpcode.GO:
                case MmixOpcode.STB:
                case MmixOpcode.STBU:
                case MmixOpcode.STW:
                case MmixOpcode.STWU:
                case MmixOpcode.STT:
                case MmixOpcode.STTU:
                case MmixOpcode.STO:
                case MmixOpcode.STOU:
                case MmixOpcode.STSF:
                case MmixOpcode.STHT:
                case MmixOpcode.STCO:
                case MmixOpcode.STUNC:
                case MmixOpcode.SYNCD:
                case MmixOpcode.PREST:
                case MmixOpcode.SYNCID:
                case MmixOpcode.PUSHGO:
                case MmixOpcode.OR:
                case MmixOpcode.ORN:
                case MmixOpcode.NOR:
                case MmixOpcode.XOR:
                case MmixOpcode.AND:
                case MmixOpcode.ANDN:
                case MmixOpcode.NAND:
                case MmixOpcode.NXOR:
                case MmixOpcode.BDIF:
                case MmixOpcode.WDIF:
                case MmixOpcode.TDIF:
                case MmixOpcode.ODIF:
                case MmixOpcode.MUX:
                case MmixOpcode.SADD:
                case MmixOpcode.MOR:
                case MmixOpcode.MXOR:
                    return MmixOpcodeCategory.ThreeRegs;
                case MmixOpcode.FLOTI:
                case MmixOpcode.FLOTUI:
                case MmixOpcode.SFLOTI:
                case MmixOpcode.SFLOTUI:
                case MmixOpcode.MULI:
                case MmixOpcode.MULUI:
                case MmixOpcode.DIVI:
                case MmixOpcode.DIVUI:
                case MmixOpcode.ADDI:
                case MmixOpcode.ADDUI:
                case MmixOpcode.SUBI:
                case MmixOpcode.SUBUI:
                case MmixOpcode._2ADDUI:
                case MmixOpcode._4ADDUI:
                case MmixOpcode._8ADDUI:
                case MmixOpcode._16ADDUI:
                case MmixOpcode.CMPI:
                case MmixOpcode.CMPUI:
                case MmixOpcode.NEGI:
                case MmixOpcode.NEGUI:
                case MmixOpcode.SLI:
                case MmixOpcode.SLUI:
                case MmixOpcode.SRI:
                case MmixOpcode.SRUI:
                case MmixOpcode.CSNI:
                case MmixOpcode.CSZI:
                case MmixOpcode.CSPI:
                case MmixOpcode.CSODI:
                case MmixOpcode.CSNZI:
                case MmixOpcode.CSNPI:
                case MmixOpcode.CSEVI:
                case MmixOpcode.ZSNI:
                case MmixOpcode.ZSZI:
                case MmixOpcode.ZSPI:
                case MmixOpcode.ZSODI:
                case MmixOpcode.ZSNNI:
                case MmixOpcode.ZSNZI:
                case MmixOpcode.ZSNPI:
                case MmixOpcode.ZSEVI:
                case MmixOpcode.LDBI:
                case MmixOpcode.LDBUI:
                case MmixOpcode.LDWI:
                case MmixOpcode.LDWUI:
                case MmixOpcode.LDTI:
                case MmixOpcode.LDTUI:
                case MmixOpcode.LDOI:
                case MmixOpcode.LDOUI:
                case MmixOpcode.LDSFI:
                case MmixOpcode.LDHTI:
                case MmixOpcode.CSWAPI:
                case MmixOpcode.LDUNCI:
                case MmixOpcode.LDVTSI:
                case MmixOpcode.PRELDI:
                case MmixOpcode.PREGOI:
                case MmixOpcode.GOI:
                case MmixOpcode.STBI:
                case MmixOpcode.STBUI:
                case MmixOpcode.STWI:
                case MmixOpcode.STWUI:
                case MmixOpcode.STTI:
                case MmixOpcode.STTUI:
                case MmixOpcode.STOI:
                case MmixOpcode.STOUI:
                case MmixOpcode.STSFI:
                case MmixOpcode.STHTI:
                case MmixOpcode.STCOI:
                case MmixOpcode.STUNCI:
                case MmixOpcode.SYNCDI:
                case MmixOpcode.PRESTI:
                case MmixOpcode.SYNCIDI:
                case MmixOpcode.PUSHGOI:
                case MmixOpcode.ORI:
                case MmixOpcode.ORNI:
                case MmixOpcode.NORI:
                case MmixOpcode.XORI:
                case MmixOpcode.ANDI:
                case MmixOpcode.ANDNI:
                case MmixOpcode.NANDI:
                case MmixOpcode.NXORI:
                case MmixOpcode.BDIFI:
                case MmixOpcode.WDIFI:
                case MmixOpcode.TDIFI:
                case MmixOpcode.ODIFI:
                case MmixOpcode.MUXI:
                case MmixOpcode.SADDI:
                case MmixOpcode.MORI:
                case MmixOpcode.MXORI:
                case MmixOpcode.PUTI:
                    return MmixOpcodeCategory.TwoRegsAndImmediate;
                case MmixOpcode.BN:
                case MmixOpcode.BNB:
                case MmixOpcode.BZ:
                case MmixOpcode.BZB:
                case MmixOpcode.BP:
                case MmixOpcode.BPB:
                case MmixOpcode.BOD:
                case MmixOpcode.BODB:
                case MmixOpcode.BNN:
                case MmixOpcode.BNNB:
                case MmixOpcode.BNZ:
                case MmixOpcode.BNZB:
                case MmixOpcode.BNP:
                case MmixOpcode.BNPB:
                case MmixOpcode.BEV:
                case MmixOpcode.BEVB:
                case MmixOpcode.PBN:
                case MmixOpcode.PBNB:
                case MmixOpcode.PBZ:
                case MmixOpcode.PBZB:
                case MmixOpcode.PBP:
                case MmixOpcode.PBPB:
                case MmixOpcode.PBOD:
                case MmixOpcode.PBODB:
                case MmixOpcode.PBNN:
                case MmixOpcode.PBNNB:
                case MmixOpcode.PBNZ:
                case MmixOpcode.PBNZB:
                case MmixOpcode.PBNP:
                case MmixOpcode.PBNPB:
                case MmixOpcode.PBEV:
                case MmixOpcode.PBEVB:
                case MmixOpcode.PUSHJ:
                case MmixOpcode.PUSHJB:
                case MmixOpcode.GETA:
                case MmixOpcode.GETAB:
                    return MmixOpcodeCategory.Branch;
                case MmixOpcode.JMP:
                case MmixOpcode.JMPB:
                    return MmixOpcodeCategory.Jump;
                case MmixOpcode.SETH:
                case MmixOpcode.SETMH:
                case MmixOpcode.SETML:
                case MmixOpcode.SETL:
                case MmixOpcode.INCH:
                case MmixOpcode.INCMH:
                case MmixOpcode.INCML:
                case MmixOpcode.INCL:
                case MmixOpcode.ORH:
                case MmixOpcode.ORMH:
                case MmixOpcode.ORML:
                case MmixOpcode.ORL:
                case MmixOpcode.ANDNH:
                case MmixOpcode.ANDNMH:
                case MmixOpcode.ANDNML:
                case MmixOpcode.ANDNL:
                    return MmixOpcodeCategory.RegAndWydeImmediate;
                case MmixOpcode.GET:
                    return MmixOpcodeCategory.GetSpecialRegister;
                case MmixOpcode.PUT:
                    return MmixOpcodeCategory.PutSpecialRegister;
                default:
                    return MmixOpcodeCategory.NonStrict;
            }
        }
    }
}
