wasm -zq -zcm -Dmemodel=large -DWATCOM pdosstrt.asm
wasm -zq -zcm -Dmemodel=large support.asm
wasm -zq -zcm -Dmemodel=large lldos.asm
wasm -zq -zcm -Dmemodel=large handlers.asm
wasm -zq -zcm -Dmemodel=large -DWATCOM ..\pdpclib\dossupa.asm
wcl -ecc -q -w -c -ml -zl -fpi87 -s -zdp -zu -D__MSDOS__ -I..\pdpclib memmgr.c format.c patmat.c process.c
wcl -ecc -q -w -c -ml -zl -fpi87 -s -zdp -zu -D__MSDOS__ -I..\pdpclib int21.c log.c helper.c
wcl -ecc -q -w -c -ml -zl -fpi87 -s -zdp -zu -D__MSDOS__ -I..\pdpclib bos.c fat.c ..\pdpclib\string.c ..\pdpclib\ctype.c ..\pdpclib\dossupc.c
rem bcc -w- to switch off warnings
wcl -ecc -q -w -c -ml -zl -fpi87 -s -zdp -zu -D__MSDOS__ -I..\pdpclib pdos.c

if exist os.lib del os.lib
wlib -b -q os +bos.obj
wlib -b -q os +support.obj
wlib -b -q os +fat.obj
wlib -b -q os +dossupa.obj
wlib -b -q os +string.obj
wlib -b -q os +dossupc.obj
wlib -b -q os +lldos.obj
wlib -b -q os +handlers.obj
wlib -b -q os +memmgr.obj
wlib -b -q os +format.obj
wlib -b -q os +ctype.obj
wlib -b -q os +patmat.obj
wlib -b -q os +process.obj
wlib -b -q os +int21.obj
wlib -b -q os +log.obj
wlib -b -q os +helper.obj

wlink File pdosstrt.obj,pdos.obj Name pdos.exe Form dos Library os.lib Option quiet,map
