CC=smlrc
COPTS=-huge -I .

all: pload.exe pdos.exe pcomm.exe

pload.exe: pload.obj minifat.obj bos.obj pdosload.obj \
  ../pdpclib/string.o ../pdpclib/ctype.o
  copy pload.obj pload.exe

pdos.exe: memmgr.obj format.obj patmat.obj process.obj int21.obj \
       log.obj helper.obj bos.obj fat.obj pdos.obj
  copy pdos.obj pdos.exe

pcomm.exe: pcomm.obj pos.obj dostime.obj
  copy pcomm.obj pcomm.exe

.c.obj:
  pdcc -E -D__SMALLERC__ -I . -I ../pdpclib -o $*.e $<
  $(CC) $(COPTS) $*.e $*.s
  rm -f $*.e
  nasm -f obj $*.s -o $*.obj
  rm -f $*.s

.asm.obj:
  wasm -q -zcm $<
