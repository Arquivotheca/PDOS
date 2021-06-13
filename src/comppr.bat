rem wasmr -zq -zcm -Dmemodel=large support.asm
rem tcc -O -c -ml -I..\pdpclib portread.c pos.c bos.c
rem tlink -x portread+bos+pos+support,portread.exe,,..\pdpclib\borland.lib

rem wasm -zq -zcm -Dmemodel=large support.asm
rem wcl -q -w -c -I. -ml -zl -D__MSDOS__ -fpi87 -s -zdp -zu -I..\pdpclib portread.c bos.c
rem wlink File portread.obj,bos.obj,support.obj Name portread.exe Form dos Library ..\pdpclib\watcom.lib Option quiet

gccwin -O2 -S -fno-common -ansi -I. -I../pdpclib -D__WIN32__ portread.c
aswin -o portread.o portread.s
del portread.s

gccwin -O2 -S -fno-common -ansi -I. -I../pdpclib -D__32BIT__ bos.c
aswin -o bos.o bos.s
del bos.s

aswin -o support.o support.s

ldwin -o portread.exe ../pdpclib/w32start.o portread.o bos.o support.o ../pdpclib/msvcrt.a kernel32.a
