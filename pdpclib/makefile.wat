CC=wcl386
COPTS=-wx -c -I. -y -fpi87 -s -zq -3s -zm -zl -oneatx

pdptest.exe: startup.obj pdptest.obj stdio.obj string.obj stdlib.obj \
       start.obj time.obj errno.obj assert.obj signal.obj locale.obj \
       ctype.obj setjmp.obj math.obj fpfuncsw.obj
#Debug All
  if exist watcom.lib del watcom.lib
  wlib -q watcom startup.obj stdio.obj string.obj stdlib.obj \
       start.obj time.obj errno.obj assert.obj signal.obj locale.obj \
       ctype.obj setjmp.obj math.obj fpfuncsw.obj
  wlink File pdptest.obj \
        Name pdptest.exe \
        Library watcom.lib Library os2386.lib
  
startup.obj: startup.asm
  wasm -zq startup.asm

.c.obj:
  $(CC) $(COPTS) $<
