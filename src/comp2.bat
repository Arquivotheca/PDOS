tasm -Dmemodel=large pdosstrt
tasm -Dmemodel=large support
tasm -Dmemodel=large -DNEED_HANDLER lldos
tasm -Dmemodel=large ..\pdpclib\dossupa
bcc -O -ml -c -I..\pdpclib memmgr.c format.c patmat.c
bcc -O -ml -c -I..\pdpclib bos.c fat.c ..\pdpclib\string.c ..\pdpclib\ctype.c ..\pdpclib\dossupc.c
rem bcc -w- to switch off warnings
bcc -O -j1 -ml -c -I..\pdpclib pdos.c 
tlink -x pdosstrt+pdos+bos+support+fat+dossupa+string+dossupc+lldos+memmgr+format+ctype+patmat,pdos.exe
