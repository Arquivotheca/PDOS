del *.o
del osworld.exe

gcc386 -S -Os -fno-common -I. -o osworld.s osworld.c
as386 -o osworld.o osworld.s
del osworld.s

gcc386 -S -Os -fno-common -I. -o genstart.s genstart.c
as386 -o genstart.o genstart.s
del genstart.s

gcc386 -S -Os -fno-common -I. -o osfunc.s osfunc.c
as386 -o osfunc.o osfunc.s
del osfunc.s

ld386 -N -s -e ___crt0 -o osworld.exe genstart.o osworld.o osfunc.o
