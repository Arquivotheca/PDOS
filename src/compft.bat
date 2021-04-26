rem wasmr -zq -zcm -Dmemodel=large support.asm
rem tcc -O -c -ml -I..\pdpclib functest.c pos.c bos.c
rem tlink -x functest+bos+pos+support,functest.exe,,..\pdpclib\borland.lib

wasm -zq -zcm -Dmemodel=large support.asm
wcl -q -w -c -I. -ml -zl -D__MSDOS__ -fpi87 -s -zdp -zu -I..\pdpclib functest.c pos.c bos.c
wlink File functest.obj,pos.obj,bos.obj,support.obj Name functest.exe Form dos Library ..\pdpclib\watcom.lib Option quiet
