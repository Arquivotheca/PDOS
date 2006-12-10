gcc -s -c -I../pdpclib world.c
ld -s -o world ../pdpclib/pdosst32.o world.o ../pdpclib/pdos.a
if exist world.exe del world.exe
ren world world.exe
