rem
rem Used to compile a combined pcomm + pdos that can then be
rem debugged under DOS
rem 

tasm -Dmemodel=large support
tasm -Dmemodel=large lldos
bcc -v -ml -c -DUSING_EXE -I..\pdpclib -I..\ozpd pcomm.c pdos.c bos.c fat.c ..\ozpd\patmat.c ..\ozpd\memmgr.c
tlink -x -v pcomm+pdos+bos+fat+patmat+memmgr+support+lldos,pcomm.exe,,..\pdpclib\borland.lib

