@echo off
tasm -mx -Dmemodel=tiny support.asm 
tasm -mx -Dmemodel=tiny iostart.asm
tasm -mx -Dmemodel=tiny near.asm
tcc -mt -c -I..\pdpclib io.c fat.c bos.c ..\pdpclib\string.c ..\pdpclib\dossupc.c
rem the order of the following stuff is critical so that the first 3*512
rem bytes have sufficient stuff to do the rest of the load of itself!!!
tlink -t iostart.obj+io.obj+support.obj+near.obj+bos.obj+dossupc.obj+fat.obj+string.obj,io.com,io.map,,

