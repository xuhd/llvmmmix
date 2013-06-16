		LOC		Data_Segment
		GREG	@
Const	OCTA	10
Buff	BYTE	"0",#a,0
argv	IS		$1					The argument vector
		LOC		#100
Counter	IS		$3
Limit	IS		$4
BuffRef	IS		$7
Main	NEG		Counter,0
		LDA		Limit,Const
		LDO		Limit,Limit,0
		LDA		BuffRef,Buff
Zero	IS		#30
Char	IS		$5
Loop	SETL	Char,Zero
		ADDU	Char,Char,Counter
		STB		Char,BuffRef,0
		NEG		$255,0
		ADDU	$255,$255,BuffRef
		TRAP	0,Fputs,StdOut
Cond	IS		$6		
		ADDU	Counter,Counter,1
		CMP		Cond,Counter,Limit
		PBNZ	Cond,Loop
Exit	TRAP	0,Halt,0
