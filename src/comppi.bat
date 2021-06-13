rem wasmr -zq -zcm -Dmemodel=large support.asm
rem tcc -O -c -ml -I..\pdpclib portinit.c pos.c bos.c
rem tlink -x portinit+bos+pos+support,portinit.exe,,..\pdpclib\borland.lib

rem wasm -zq -zcm -Dmemodel=large support.asm
rem wcl -q -w -c -I. -ml -zl -D__MSDOS__ -fpi87 -s -zdp -zu -I..\pdpclib portinit.c bos.c
rem wlink File portinit.obj,bos.obj,support.obj Name portinit.exe Form dos Library ..\pdpclib\watcom.lib Option quiet

gccwin -O2 -S -fno-common -ansi -I. -I../pdpclib -D__WIN32__ portinit.c
aswin -o portinit.o portinit.s
del portinit.s

gccwin -O2 -S -fno-common -ansi -I. -I../pdpclib -D__32BIT__ bos.c
aswin -o bos.o bos.s
del bos.s

aswin -o support.o support.s

ldwin -o portinit.exe ../pdpclib/w32start.o portinit.o bos.o support.o ../pdpclib/msvcrt.a kernel32.a
