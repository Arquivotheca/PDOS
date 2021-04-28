del *.o
del osworld.exe

gccwin -S -Os -fno-common -I. -o osworld.s osworld.c
aswin -o osworld.o osworld.s
del osworld.s

gccwin -S -Os -fno-common -I. -o genstart.s genstart.c
aswin -o genstart.o genstart.s
del genstart.s

gccwin -S -Os -fno-common -I. -o osfunc.s osfunc.c
aswin -o osfunc.o osfunc.s
del osfunc.s

rem I don't know why I need to use "pie" to stop relocations
rem from being stripped
ldwin -s -e ___crt0 -o osworld.exe genstart.o osworld.o osfunc.o
