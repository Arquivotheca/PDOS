gcc -c -O0 -D__WIN32__ -I ../pdpclib -I . ../pdpclib/dllcrt.c
gcc -c -O0 -D__WIN32__ -I ../pdpclib -I . ntdll.c
gcc -c -O0 -D__WIN32__ -D__32BIT__ -I . pos.c
gcc -c -O0 -D__WIN32__ -I . support.s
gcc -shared -s -nostdlib -o ntdll.dll -Wl,--kill-at dllcrt.o ntdll.o pos.o support.o
dlltool -k --export-all-symbols -D ntdll.dll ntdll.o -l libntdll.a
