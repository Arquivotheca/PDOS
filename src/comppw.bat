rem wasmr -zq -zcm -Dmemodel=large support.asm
rem tcc -O -c -ml -I..\pdpclib portwrit.c pos.c bos.c
rem tlink -x portwrit+bos+pos+support,portwrit.exe,,..\pdpclib\borland.lib

rem wasm -zq -zcm -Dmemodel=large support.asm
rem wcl -q -w -c -I. -ml -zl -D__MSDOS__ -fpi87 -s -zdp -zu -I..\pdpclib portwrit.c bos.c
rem wlink File portwrit.obj,bos.obj,support.obj Name portwrit.exe Form dos Library ..\pdpclib\watcom.lib Option quiet

gccwin -O2 -S -fno-common -ansi -I. -I../pdpclib -D__WIN32__ portwrit.c
aswin -o portwrit.o portwrit.s
del portwrit.s

gccwin -O2 -S -fno-common -ansi -I. -I../pdpclib -D__32BIT__ bos.c
aswin -o bos.o bos.s
del bos.s

aswin -o support.o support.s

ldwin -o portwrit.exe ../pdpclib/w32start.o portwrit.o bos.o support.o ../pdpclib/msvcrt.a kernel32.a
