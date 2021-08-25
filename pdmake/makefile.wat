# Use Open Watcom to produce Windows executable
# Build with wmake -u -f makefile.wat

CC=wcl386
CFLAGS=
COPTS=-c -q

all: pdmake.exe

pdmake.exe: main.obj read.obj variable.obj xmalloc.obj
  wcl386 -q -fe=pdmake.exe main.obj read.obj variable.obj xmalloc.obj

.c.obj:
  $(CC) $(COPTS) $<
