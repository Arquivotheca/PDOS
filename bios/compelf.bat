wcl386 -q -s -3s -zl -fpi87 -c -I. genstart.c osworld.c osfunc.c
wlink File genstart.obj,osworld.obj,osfunc.obj Name osworld.exe Form elf Option quiet,start=__crt0
