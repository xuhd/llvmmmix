argv	IS		$1					The argument vector
		LOC		#100
Main	LDOU	$255,argv,0		    $255 <- address of program name
		TRAP	0,Fputs,StdOut		Print that name
		GETA	$255,String			$255 <- address of ", world"
		TRAP	0,Fputs,StdOut		Print that string
		TRAP	0,Halt,0			Stop
String	BYTE	", world",#a,0		String of characters with new line terminator
		LOC		Data_Segment
		BYTE	"Some data",#a,0