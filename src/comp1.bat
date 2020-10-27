@echo off
jwasmr -q -zcm -Dmemodel=tiny ploadst.asm
jwasmr -q -zcm -Dmemodel=tiny int13x.asm
jwasmr -q -zcm -Dmemodel=tiny near.asm

rem these two files are shared between modules
jwasmr -q -zcm -Dmemodel=tiny support.asm 
jwasmr -q -zcm -Dmemodel=tiny lldos.asm
tcc -O -c -mt -DNEED_DUMP -j1 -I..\pdpclib pload.c minifat.c bos.c pdosload.c ..\pdpclib\string.c ..\pdpclib\dossupc.c
tcc -O -c -mt -j1 -I..\pdpclib ..\pdpclib\ctype.c

if exist borland.lib del borland.lib
tlib borland +support.obj +bos.obj +dossupc.obj +minifat.obj
tlib borland +string.obj +pdosload.obj +lldos.obj +ctype.obj

rem the order of the following stuff is critical so that the first 3*512
rem bytes have sufficient stuff to do the rest of the load of itself!!!
rem it is ploadst, pload, int13x and near that need to be in the first 3*512
rem and only the first 3 functions of near
tlink -t -x ploadst+pload+int13x+near,pload.com,,borland.lib,
