@echo off
wasmr -zq -zcm -Dmemodel=tiny support.asm 
wasmr -zq -zcm -Dmemodel=tiny ploadst.asm
wasmr -zq -zcm -Dmemodel=tiny near.asm
wasmr -zq -zcm -Dmemodel=tiny lldos.asm
wasmr -zq -zcm -Dmemodel=tiny protinta.asm
wasmr -zq -zcm -Dmemodel=tiny int13x.asm
tcc -O -c -mt -DPDOS32 -I..\pdpclib pload.c protint.c file.c
tcc -O -c -mt -DNEED_DUMP -DPDOS32 -I..\pdpclib fat.c bos.c pdosload.c ..\pdpclib\string.c ..\pdpclib\dossupc.c
tcc -O -c -mt -DNEED_DUMP -DPDOS32 -I..\pdpclib ..\pdpclib\ctype.c
if exist borland.lib del borland.lib
tlib borland +support.obj +bos.obj +dossupc.obj +fat.obj +string.obj +pdosload.obj
tlib borland +lldos.obj +protinta.obj +file.obj +protint.obj +ctype.obj
rem the order of the following stuff is critical so that the first 3*512
rem bytes have sufficient stuff to do the rest of the load of itself!!!
rem it is ploadst, pload, int13x and near that need to be in the first 3*512
rem and only the first 3 functions of near
tlink -t -x -3 ploadst+pload+int13x+near,pload.com,,borland.lib,
