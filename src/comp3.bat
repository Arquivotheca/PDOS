tasm -Dmemodel=large support.asm
tcc -O -c -ml -I..\pdpclib pcomm.c pos.c dostime.c
tlink -x pcomm+pos+dostime+support,pcomm.exe,,..\pdpclib\borland.lib

