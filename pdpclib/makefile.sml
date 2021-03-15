CC=smlrc
COPTS=-huge -I .

pdptest.exe: dosstart.obj pdptest.obj stdio.obj string.obj stdlib.obj \
       start.obj time.obj errno.obj assert.obj signal.obj locale.obj \
       ctype.obj setjmp.obj math.obj dossupc.obj dossupa.obj
  echo if exist borland.lib del borland.lib
  echo tlib borland +dosstart.obj +stdio.obj +string.obj +stdlib.obj
  echo tlib borland +start.obj +time.obj +errno.obj +assert.obj +signal.obj
  echo tlib borland +locale.obj +ctype.obj +setjmp.obj +math.obj
  echo tlib borland +dossupc.obj +dossupa.obj
  echo tlink dosstart+pdptest,pdptest.exe,nul.map,borland.lib,
  copy dosstart.obj pdptest.exe

.c.obj:
  smlrpp -D__SMALLERC__ -zI -I . -o $*.e $<
  $(CC) $(COPTS) $*.e $*.s
  rm -f $*.e
  nasm -f obj $*.s -o $*.obj
  rm -f $*.s

.asm.obj:
  wasm -q -zcm $<
