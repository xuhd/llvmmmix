		LOC		Data_Segment
		GREG	@
Y		OCTA	#1122334455667788
Z		OCTA	#0102040810204080
Main	LOC		#120
		GREG	@
		LDA		$1,Y
		LDO		$1,$1,0
		LDA		$2,Z
		LDO		$2,$2,0
		MOR		$0,$1,$2
		GO		$3,$3,0
Entry	LOC		#100
		LDA		$3,Main
		GO		$3,$3,0
		TRAP	0,Halt,0
		
