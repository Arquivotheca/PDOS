CC=gcc
COPTS=-c -I../pdpclib -I../ozpd
TASM=tasm
TCC=bcc
TCOPTS=-O -w -mt -c -DPDOS32 -I..\pdpclib
TAOPTS=-ml -Dmemodel=tiny

HEADERS=a_out.h bos.h dump.h fat.h pdos.h pos.h protint.h support.h unused.h

OBJS=strt32.o pdos.o bos.o fat.o patmat.o format.o support.o \
     memmgr.o protintp.o protints.o pdoss.o

PCOBJS=../pdpclib/pdosst32.o pcomm.o pcommrt.o

TOBJS=support.obj ploadst.obj near.obj lldos.obj protinta.obj int13x.obj \
    pload.obj protint.obj file.obj fat.obj bos.obj pdosload.obj \
    string.obj dossupc.obj

all: pload.com pdos.exe pcomm.exe

pdos.exe: $(OBJS)
  ld -s -o pdos $(OBJS) ../pdpclib/pdos.a
  if exist pdos.exe del pdos.exe
  ren pdos pdos.exe

pcomm.exe: $(PCOBJS)
  ld -s -o pcomm $(PCOBJS) ../pdpclib/pdos.a
  if exist pcomm.exe del pcomm.exe
  ren pcomm pcomm.exe

pload.com: $(TOBJS)
  tlink -t -x -3 ploadst+pload+int13x+near+support+bos+dossupc+\
    fat+string+pdosload+lldos+protinta+file+protint,pload.com,,,

patmat.o: ..\ozpd\patmat.c
  $(CC) $(COPTS) ..\ozpd\patmat.c

memmgr.o: ..\ozpd\memmgr.c
  $(CC) $(COPTS) ..\ozpd\memmgr.c

.c.o:
  $(CC) $(COPTS) $<

.s.o:
  $(CC) $(COPTS) $<

.asm.obj:
  $(TASM) $(TAOPTS) $<

.c.obj:
  $(TCC) $(TCOPTS) $<

string.obj: ..\pdpclib\string.c
  $(TCC) $(TCOPTS) ..\pdpclib\string.c

dossupc.obj: ..\pdpclib\dossupc.c
  $(TCC) $(TCOPTS) ..\pdpclib\dossupc.c
