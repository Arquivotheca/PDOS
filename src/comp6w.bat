gcc386 -S -Os -fno-common -I../pdpclib -D__32BIT__ pcomm.c
gcc386 -S -Os -fno-common -I../pdpclib -D__32BIT__ pcommrt.c
gcc386 -S -Os -fno-common -I../pdpclib -D__32BIT__ dostime.c

as386 -o pcomm.o pcomm.s
del pcomm.s
as386 -o pcommrt.o pcommrt.s
del pcommrt.s
as386 -o dostime.o dostime.s
del dostime.s

ld386 -s -e ___pdosst32 -o pcomm.exe ../pdpclib/pdosst32.o pcomm.o pcommrt.o dostime.o ../pdpclib/pdos.a
ld386 -r -s -e ___pdosst32 -o pcomm.exe ../pdpclib/pdosst32.o pcomm.o pcommrt.o dostime.o ../pdpclib/pdos.a
strip386 --strip-unneeded pcomm.exe