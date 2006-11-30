@echo off
tasm -Dmemodel=tiny ploadst.asm
tasm -Dmemodel=tiny int13x.asm
tasm -Dmemodel=tiny near.asm

rem these two files are shared between modules
tasm -Dmemodel=tiny support.asm 
tasm -Dmemodel=tiny lldos.asm
tcc -O -c -mt -DNEED_DUMP -I..\pdpclib pload.c fat.c bos.c pdosload.c ..\pdpclib\string.c ..\pdpclib\dossupc.c
rem the order of the following stuff is critical so that the first 3*512
rem bytes have sufficient stuff to do the rest of the load of itself!!!
tlink -t -x ploadst+pload+int13x+near+support+bos+dossupc+fat+string+pdosload+lldos,pload.com,,,
