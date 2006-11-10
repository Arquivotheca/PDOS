tasm -Dmemodel=large support
tcc -O -ml -c -I..\pdpclib pcomm.c pos.c dostime.c
tlink -x pcomm+pos+dostime+support,pcomm.exe,,..\pdpclib\borland.lib

