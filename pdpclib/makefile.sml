CC=smlrc
COPTS=-huge -I .

pdptest.exe: smlstart.obj pdptest.obj stdio.obj string.obj stdlib.obj \
       start.obj time.obj errno.obj assert.obj signal.obj locale.obj \
       ctype.obj setjmp.obj math.obj dossupc.obj
  echo if exist borland.lib del borland.lib
  echo tlib borland +smlstart.obj +stdio.obj +string.obj +stdlib.obj
  echo tlib borland +start.obj +time.obj +errno.obj +assert.obj +signal.obj
  echo tlib borland +locale.obj +ctype.obj +setjmp.obj +math.obj
  echo tlib borland +dossupc.obj
  echo tlink smlstart+pdptest,pdptest.exe,nul.map,borland.lib,
  echo copy smlstart.obj pdptest.exe
  smlrl -huge -entry ___intstart -o pdptest.exe pdptest.obj smlstart.obj stdio.obj string.obj stdlib.obj start.obj time.obj errno.obj assert.obj signal.obj locale.obj ctype.obj setjmp.obj math.obj dossupc.obj

.c.obj:
  pdcc -E -D__SMALLERC__ -I . -o $*.e $<
  $(CC) $(COPTS) $*.e $*.s
  rm -f $*.e
  nasm -f elf32 $*.s -o $*.obj
  rm -f $*.s

.asm.obj:
  jwasm -elf -Dmemodel=large $< -Fo$*.obj
