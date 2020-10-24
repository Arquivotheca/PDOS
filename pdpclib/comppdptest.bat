gcc -c -O2 -D__WIN32__ -I . -I../src pdptest.c
gcc -s -nostdlib -o pdptest.exe w32start.o pdptest.o msvcrt.a
