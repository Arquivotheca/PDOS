rem wasmr -zq -zcm -Dmemodel=large support.asm
rem tcc -O -c -ml -I..\pdpclib terminal.c pos.c bos.c
rem tlink -x terminal+bos+pos+support,terminal.exe,,..\pdpclib\borland.lib

wasm -zq -zcm -Dmemodel=large support.asm
wcl -q -w -c -I. -ml -zl -D__MSDOS__ -fpi87 -s -zdp -zu -I..\pdpclib terminal.c bos.c
wlink File terminal.obj,bos.obj,support.obj Name terminal.exe Form dos Library ..\pdpclib\watcom.lib Option quiet

rem gccwin -O2 -S -fno-common -ansi -I. -I../pdpclib -D__WIN32__ terminal.c
rem aswin -o terminal.o terminal.s
rem del terminal.s

rem gccwin -O2 -S -fno-common -ansi -I. -I../pdpclib -D__32BIT__ bos.c
rem aswin -o bos.o bos.s
rem del bos.s

rem aswin -o support.o support.s

rem ldwin -o terminal.exe ../pdpclib/w32start.o terminal.o bos.o support.o ../pdpclib/msvcrt.a kernel32.a
