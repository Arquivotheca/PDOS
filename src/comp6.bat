gcc -fno-common -c -O -s -I../pdpclib pcomm.c dostime.c
ld -r -s -o pcomm ../pdpclib/pdosst32.o pcomm.o dostime.o ../pdpclib/pdos.a
if exist pcomm.exe del pcomm.exe
ren pcomm pcomm.exe
