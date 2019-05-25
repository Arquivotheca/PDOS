gcc386 -S -Os -fno-common -I../pdpclib pcomm.c
gcc386 -S -Os -fno-common -I../pdpclib dostime.c

as386 -o pcomm.o pcomm.s
del pcomm.s
as386 -o dostime.o dostime.s
del dostime.s

ld386 -s -e ___pdosst32 -o pcomm.exe ../pdpclib/pdosst32.o pcomm.o dostime.o ../pdpclib/pdos.a
ld386 -r -s -e ___pdosst32 -o pcomm.exe ../pdpclib/pdosst32.o pcomm.o dostime.o ../pdpclib/pdos.a
strip386 --strip-unneeded pcomm.exe
