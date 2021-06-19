  rm -f *.o pdmake.exe
  gccwin -S -O2 -fno-common -ansi -I. -I../pdpclib -D__WIN32__ -o main.s main.c
  aswin -o main.o main.s
  rm -f main.s
  gccwin -S -O2 -fno-common -ansi -I. -I../pdpclib -D__WIN32__ -o read.s read.c
  aswin -o read.o read.s
  rm -f read.s
  gccwin -S -O2 -fno-common -ansi -I. -I../pdpclib -D__WIN32__ -o variable.s variable.c
  aswin -o variable.o variable.s
  rm -f variable.s
  gccwin -S -O2 -fno-common -ansi -I. -I../pdpclib -D__WIN32__ -o xmalloc.s xmalloc.c
  aswin -o xmalloc.o xmalloc.s
  rm -f xmalloc.s
  ldwin  -s -o pdmake.exe ../pdpclib/w32start.o main.o read.o variable.o xmalloc.o ../pdpclib/msvcrt.a ../src/kernel32.a
