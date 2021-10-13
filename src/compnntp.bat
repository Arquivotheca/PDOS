gccwin -O2 -S -fno-common -ansi -I. -I../pdpclib -D__WIN32__ pdpnntp.c
aswin -o pdpnntp.o pdpnntp.s
del pdpnntp.s

ldwin -o pdpnntp.exe ../pdpclib/w32start.o pdpnntp.o ../pdpclib/msvcrt.a
