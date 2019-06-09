wasmr -zq -zcm -Dmemodel=large pdosstrt.asm
wasmr -zq -zcm -Dmemodel=large support.asm
wasmr -zq -zcm -Dmemodel=large lldos.asm
wasmr -zq -zcm -Dmemodel=large handlers.asm
wasmr -zq -zcm -Dmemodel=large ..\pdpclib\dossupa.asm
tcc -w- -O -c -ml -j1 -I..\pdpclib memmgr.c format.c patmat.c process.c
tcc -w- -O -c -ml -j1 -I..\pdpclib int21.c log.c helper.c
tcc -w- -O -c -ml -j1 -I..\pdpclib bos.c fat.c ..\pdpclib\string.c ..\pdpclib\ctype.c ..\pdpclib\dossupc.c
rem bcc -w- to switch off warnings
tcc -w- -O -j1 -c -ml -j1 -I..\pdpclib pdos.c

if exist os.lib del os.lib
tlib os +bos.obj +support.obj +fat.obj +dossupa.obj
tlib os +string.obj +dossupc.obj +lldos.obj +handlers.obj
tlib os +memmgr.obj +format.obj +ctype.obj +patmat.obj
tlib os +process.obj +int21.obj +log.obj +helper.obj

tlink -x pdosstrt+pdos,pdos.exe,,os.lib,
