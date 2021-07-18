rem uses PDPCLIB's makefile.wcd

wasm -zq -zcm -Dmemodel=large support.asm
wcl -q -w -c -I. -ml -zl -D__MSDOS__ -fpi87 -s -zdp -zu -I..\pdpclib instmbr.c bos.c
wlink File instmbr.obj,bos.obj,support.obj Name instmbr.exe Form dos Library ..\pdpclib\watcom.lib Option quiet
