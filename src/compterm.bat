rem wasmr -zq -zcm -Dmemodel=large support.asm
rem tcc -O -c -ml -I..\pdpclib terminal.c pos.c bos.c
rem tlink -x terminal+bos+pos+support,terminal.exe,,..\pdpclib\borland.lib

rem wasm -zq -zcm -Dmemodel=large support.asm
rem wcl -q -w -c -I. -ml -zl -D__MSDOS__ -fpi87 -s -zdp -zu -I..\pdpclib terminal.c bos.c
rem wlink File terminal.obj,bos.obj,support.obj Name terminal.exe Form dos Library ..\pdpclib\watcom.lib Option quiet

gccwin -O2 -S -fno-common -ansi -I. -I../pdpclib -D__WIN32__ terminal.c
aswin -o terminal.o terminal.s
del terminal.s

gccwin -O2 -S -fno-common -ansi -I. -I../pdpclib -D__32BIT__ bos.c
aswin -o bos.o bos.s
del bos.s

aswin -o support.o support.s

ldwin -o terminal.exe ../pdpclib/w32start.o terminal.o bos.o support.o ../pdpclib/msvcrt.a kernel32.a
