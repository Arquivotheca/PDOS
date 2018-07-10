@echo off
wasmr -zq -zcm -Dmemodel=tiny ploadst.asm
wasmr -zq -zcm -Dmemodel=tiny int13x.asm
wasmr -zq -zcm -Dmemodel=tiny near.asm

rem these two files are shared between modules
wasmr -zq -zcm -Dmemodel=tiny support.asm 
wasmr -zq -zcm -Dmemodel=tiny lldos.asm
tcc -O -c -mt -DNEED_DUMP -j1 -I..\pdpclib pload.c fat.c bos.c pdosload.c ..\pdpclib\string.c ..\pdpclib\dossupc.c

if exist borland.lib del borland.lib
tlib borland +support.obj +bos.obj +dossupc.obj +fat.obj
tlib borland +string.obj +pdosload.obj +lldos.obj

rem the order of the following stuff is critical so that the first 3*512
rem bytes have sufficient stuff to do the rest of the load of itself!!!
rem it is ploadst, pload, int13x and near that need to be in the first 3*512
rem and only the first 3 functions of near
tlink -t -x ploadst+pload+int13x+near,pload.com,,borland.lib,
