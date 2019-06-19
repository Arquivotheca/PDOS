gcc -c -O0 -D__WIN32__ -I ../pdpclib -I . ../pdpclib/dllcrt.c
gcc -c -O0 -D__WIN32__ -I ../pdpclib -I . kernel32.c
gcc -c -O0 -D__WIN32__ -D__32BIT__ -I . pos.c
gcc -c -O0 -D__WIN32__ -I . support.s
gcc -shared -s -nostdlib -o kernel32.dll -Wl,--kill-at dllcrt.o kernel32.o pos.o support.o
dlltool -k --export-all-symbols -D kernel32.dll kernel32.o -l libkernel32.a

