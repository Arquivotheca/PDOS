wasmr -zq -zcm -Dmemodel=large support.asm
tcc -O -c -ml -I..\pdpclib functest.c pos.c bos.c
tlink -x functest+bos+pos+support,functest.exe,,..\pdpclib\borland.lib

