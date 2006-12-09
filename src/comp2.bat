wasmr -zq -zcm -Dmemodel=large pdosstrt.asm
wasmr -zq -zcm -Dmemodel=large support.asm
wasmr -zq -zcm -Dmemodel=large lldos.asm
wasmr -zq -zcm -Dmemodel=large handlers.asm
wasmr -zq -zcm -Dmemodel=large ..\pdpclib\dossupa.asm
tcc -O -c -ml -I..\pdpclib memmgr.c format.c patmat.c
tcc -O -c -ml -I..\pdpclib bos.c fat.c ..\pdpclib\string.c ..\pdpclib\ctype.c ..\pdpclib\dossupc.c
rem bcc -w- to switch off warnings
tcc -O -j1 -c -ml -I..\pdpclib pdos.c 
tlink -x pdosstrt+pdos+bos+support+fat+dossupa+string+dossupc+lldos+handlers+memmgr+format+ctype+patmat,pdos.exe
