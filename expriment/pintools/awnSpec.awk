#! /bin/awk 

#lastStat
#0 last record not counted
#1 last record counted

BEGIN{ last=0xffffffffffff; inc=0; lines=0; lastStat=0}

/Pin|Copyright/{
	print $0 " drop"
	next
}

/-/{
#	print $0 " sep"
	last=0xffffffffffff
 	lastStat = 0;
	lines++
}

/^[ ]*0x[0-9a-f ]*$/ {
	if($1>=last){
		inc++ 
		if(lastStat == 0){
			inc++ 
		}
 		lastStat = 1;
#		print $1 " +"
	}else{
#		print $1 
 		lastStat = 0;
	}
	last=$1; 
	lines++
}

END{printf "\nline: %d\ninc: %d\npercentage:%.2f\%\n", lines, inc, inc/lines*100}
#/^[ ]*0x[0-9a-f ]*$/ {print $1 ; ) } END{print inc}'
