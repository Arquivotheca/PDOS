gcc -c -O -s -I../pdpclib pcomm.c pcommrt.c
ld -s -o pcomm ../pdpclib/pdosst32.o pcomm.o pcommrt.o ../pdpclib/pdos.a
if exist pcomm.exe del pcomm.exe
ren pcomm pcomm.exe
