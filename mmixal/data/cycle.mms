		LOC		Data_Segment
		GREG	@
Const	OCTA	10
SrRef	OCTA	Print
Buff	BYTE	"0",#a,0

		LOC		#100
Counter	IS		$0
Arg		IS		$1
Cond	IS		$2
Limit	GREG
BuffRef	GREG
Subr	GREG
Main	AND		Counter,Counter,0
		LDA		Limit,Const
		LDO		Limit,Limit,0
		LDA		BuffRef,Buff
		LDA		Subr,SrRef
		LDO		Subr,Subr,0
Loop	SET		Arg,Counter
		PUSHGO	Counter,Subr,0
		ADDU	Counter,Counter,1
		CMP		Cond,Counter,Limit
		PBNZ	Cond,Loop
Exit	TRAP	0,Halt,0	

Zero	IS		#30
Char	IS		$0
TrapArg	IS		$255
Print	ADDU	Char,Char,Zero
		STB		Char,BuffRef,0
		SET		TrapArg,BuffRef
		TRAP	0,Fputs,StdOut
		POP		0
