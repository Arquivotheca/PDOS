gcc -c -O0 -D__WIN32__ -I ../pdpclib -I . kernel32.c
gcc -c -O0 -D__WIN32__ -D__32BIT__ -I . pos.c
gcc -c -O0 -D__WIN32__ -I . support.s
gcc -shared -s -nostdlib -o kernel32.dll -Wl,--kill-at kernel32.o pos.o support.o
dlltool -k -d kernel32.def -l libkernel32.a

