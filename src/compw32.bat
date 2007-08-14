gcc -s -c -I../pdpclib world.c
gcc -nostdinc -nostdlib -s -o world.exe ../pdpclib/pdosst32.o world.o ../pdpclib/pdos.a
