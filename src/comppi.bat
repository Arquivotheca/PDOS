rem wasmr -zq -zcm -Dmemodel=large support.asm
rem tcc -O -c -ml -I..\pdpclib functest.c pos.c bos.c
rem tlink -x functest+bos+pos+support,functest.exe,,..\pdpclib\borland.lib

wasm -zq -zcm -Dmemodel=large support.asm
wcl -q -w -c -I. -ml -zl -D__MSDOS__ -fpi87 -s -zdp -zu -I..\pdpclib portinit.c bos.c
wlink File portinit.obj,bos.obj,support.obj Name portinit.exe Form dos Library ..\pdpclib\watcom.lib Option quiet
