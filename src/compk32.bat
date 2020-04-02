gccwin -S -Os -D__WIN32__ -I ../pdpclib -I . ../pdpclib/dllcrt.c
aswin -o dllcrt.o dllcrt.s
del dllcrt.s

gccwin -S -Os -D__WIN32__ -I ../pdpclib -I . kernel32.c
aswin -o kernel32.o kernel32.s
del kernel32.s

rem for some reason gccwin is not generating the correct
rem assembler code, so we override with gcc 4.8.2 from Cygwin
gcc -nostdinc -c -Os -D__WIN32__ -I ../pdpclib -I . kernel32.c

gccwin -S -Os -D__WIN32__ -D__32BIT__ -I ../pdpclib -I . pos.c
aswin -o pos.o pos.s
del pos.s

aswin -o support.o support.s

ldwin -s -o kernel32.dll --shared --kill-at dllcrt.o kernel32.o pos.o support.o

dlltwin -S aswin -k --export-all-symbols -D kernel32.dll kernel32.o -l libkernel32.a
