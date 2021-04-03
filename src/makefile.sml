CC=smlrc
COPTS=-huge -I .

pdos.exe: memmgr.obj format.obj patmat.obj process.obj int21.obj \
       log.obj helper.obj bos.obj fat.obj pdos.obj
  copy pdos.obj pdos.exe

.c.obj:
  smlrpp -D__SMALLERC__ -D__32BIT__ -zI -I . -I ../pdpclib -o $*.e $<
  $(CC) $(COPTS) $*.e $*.s
  rm -f $*.e
  nasm -f obj $*.s -o $*.obj
  rm -f $*.s

.asm.obj:
  wasm -q -zcm $<
