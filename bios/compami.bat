vbccm68k -D__AMIGA__ -cpu=68000 -fpu=68881 -O=991 -I. -o=genstart.s genstart.c
vasmm68k_mot -o genstart.o -Fhunk genstart.s
rm -f genstart.s

vbccm68k -D__AMIGA__ -cpu=68000 -fpu=68881 -O=991 -I. -o=osworld.s osworld.c
vasmm68k_mot -o osworld.o -Fhunk osworld.s
rm -f osworld.s

vbccm68k -D__AMIGA__ -cpu=68000 -fpu=68881 -O=991 -I. -o=osfunc.s osfunc.c
vasmm68k_mot -o osfunc.o -Fhunk osfunc.s
rm -f osfunc.s

vlink -bamigahunk -o osworld.exe genstart.o osworld.o osfunc.o
