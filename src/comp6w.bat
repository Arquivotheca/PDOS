gcc386 -S -Os -fno-common -I../pdpclib -D__32BIT__ pcomm.c
gcc386 -S -Os -fno-common -I../pdpclib -D__32BIT__ pcommrt.c
gcc386 -S -Os -fno-common -I../pdpclib -D__32BIT__ dostime.c

as386 -o pcomm.o pcomm.s
as386 -o pcommrt.o pcommrt.s
as386 -o dostime.o dostime.s

ld386 -r -s -e ___pdosst32 --oformat a.out-i386 -o pcomm.exe ../pdpclib/pdosst32.o pcomm.o pcommrt.o dostime.o ../pdpclib/pdos.a
strip386 --strip-unneeded pcomm.exe
