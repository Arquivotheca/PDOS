gcc -fno-common -c -O -s -I../pdpclib pcomm.c pcommrt.c dostime.c
ld -r -s -o pcomm ../pdpclib/pdosst32.o pcomm.o pcommrt.o dostime.o ../pdpclib/pdos.a
if exist pcomm.exe del pcomm.exe
ren pcomm pcomm.exe
