gcc -c -O2 -D__WIN32__ -D__PDPCLIB_DLL -I . dllcrt.c
gcc -c -O2 -D__WIN32__ -D__PDPCLIB_DLL -I . stdio.c
gcc -c -O2 -D__WIN32__ -D__PDPCLIB_DLL -I . string.c
gcc -c -O2 -D__WIN32__ -D__PDPCLIB_DLL -I . stdlib.c
gcc -c -O2 -D__WIN32__ -D__PDPCLIB_DLL -I . start.c
gcc -c -O2 -D__WIN32__ -D__PDPCLIB_DLL -I . time.c
gcc -c -O2 -D__WIN32__ -D__PDPCLIB_DLL -I . errno.c
gcc -c -O2 -D__WIN32__ -D__PDPCLIB_DLL -I . assert.c
gcc -c -O2 -D__WIN32__ -D__PDPCLIB_DLL -I . signal.c
gcc -c -O2 -D__WIN32__ -D__PDPCLIB_DLL -I . locale.c
gcc -c -O2 -D__WIN32__ -D__PDPCLIB_DLL -I . ctype.c
gcc -c -O2 -D__WIN32__ -D__PDPCLIB_DLL -I . setjmp.c
gcc -c -O2 -D__WIN32__ -D__PDPCLIB_DLL -I . math.c
jwasm -c -coff winsupa.asm
gcc -shared -s -nostdlib -o msvcrt.dll -Wl,--kill-at dllcrt.o stdio.o string.o stdlib.o start.o time.o errno.o assert.o signal.o locale.o ctype.o setjmp.o math.o winsupa.obj -lkernel32 -L ../src
dlltool -k -D msvcrt.dll stdio.o string.o stdlib.o start.o time.o errno.o assert.o signal.o locale.o ctype.o setjmp.o math.o -l libmsvcrt.a
