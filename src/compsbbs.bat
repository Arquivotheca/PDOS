rem wasmr -zq -zcm -Dmemodel=large support.asm
rem tcc -O -c -ml -I..\pdpclib simpbbs.c pos.c bos.c
rem tlink -x simpbbs+bos+pos+support,simpbbs.exe,,..\pdpclib\borland.lib

rem wasm -zq -zcm -Dmemodel=large support.asm
rem wcl -q -w -c -I. -ml -zl -D__MSDOS__ -fpi87 -s -zdp -zu -I..\pdpclib simpbbs.c bos.c
rem wlink File simpbbs.obj,bos.obj,support.obj Name simpbbs.exe Form dos Library ..\pdpclib\watcom.lib Option quiet

gccwin -O2 -S -fno-common -ansi -I. -I../pdpclib -D__WIN32__ -D__NOBIVA__ simpbbs.c
aswin -o simpbbs.o simpbbs.s
del simpbbs.s

gccwin -O2 -S -fno-common -ansi -I. -I../pdpclib -D__32BIT__ bos.c
aswin -o bos.o bos.s
del bos.s

aswin -o support.o support.s

ldwin -o simpbbs.exe ../pdpclib/w32start.o simpbbs.o bos.o support.o ../pdpclib/msvcrt.a kernel32.a
