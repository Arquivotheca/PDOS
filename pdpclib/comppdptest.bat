gcc -c -O2 -D__WIN32__ -I . pdptest.c
gcc -c -O2 -D__WIN32__ -I . w32start.c
gcc -s -nostdlib -o pdptest.exe w32start.o pdptest.o -lmsvcrt -L .
