@echo off
wasmr -zq -zcm -Dmemodel=tiny support.asm 
wasmr -zq -zcm -Dmemodel=tiny ploadst.asm
wasmr -zq -zcm -Dmemodel=tiny near.asm
wasmr -zq -zcm -Dmemodel=tiny lldos.asm
wasmr -zq -zcm -Dmemodel=tiny protinta.asm
wasmr -zq -zcm -Dmemodel=tiny int13x.asm
tcc -O -c -mt -DPDOS32 -I..\pdpclib pload.c protint.c file.c
tcc -O -c -mt -DNEED_DUMP -DPDOS32 -I..\pdpclib fat.c bos.c pdosload.c ..\pdpclib\string.c ..\pdpclib\dossupc.c
rem the order of the following stuff is critical so that the first 3*512
rem bytes have sufficient stuff to do the rest of the load of itself!!!
tlink -t -x -3 ploadst+pload+int13x+near+support+bos+dossupc+fat+string+pdosload+lldos+protinta+file+protint,pload.com,,,
