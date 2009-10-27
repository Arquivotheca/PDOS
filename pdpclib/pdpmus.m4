/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE PDPASM REPL
/inc rexx
/* pdpasm - used to assemble pdpclib */
/* written by Paul Edwards based on work by Dave Edwards */
/* released to the public domain */

parse arg name

say "pdpasm: Assembling" name".asm to" name".obj"
queue "/file syspunch n("name".obj) new(repl) sp(50) secsp(100%)"
queue "/etc sp(100) secsp(100%)"
queue "/file syslib"
a="/etc pds(@BLD000:*.M,CCDE:MVS.*.M,$GCC:*.M,CCDE:OS.*.M,$MCU:*.M)"
queue a
queue "/etc def"
queue "/load asm"
queue "/job nogo"
queue "/opt deck"
queue "/opt list"
queue "/inc" name".asm"
"EXEC"

exit rc
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE PDPTOP.M REPL
undivert(pdptop.mac)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE PDPPRLG.M REPL
undivert(pdpprlg.mac)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE PDPEPIL.M REPL
undivert(pdpepil.mac)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE MUSSTART.ASM REPL
undivert(musstart.asm)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE MUSSUPA.ASM REPL
undivert(mussupa.asm)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE START.ASM REPL
undivert(start.s)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE STDIO.ASM REPL
undivert(stdio.s)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE STDLIB.ASM REPL
undivert(stdlib.s)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE CTYPE.ASM REPL
undivert(ctype.s)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE STRING.ASM REPL
undivert(string.s)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE TIME.ASM REPL
undivert(time.s)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE ERRNO.ASM REPL
undivert(errno.s)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE ASSERT.ASM REPL
undivert(assert.s)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE LOCALE.ASM REPL
undivert(locale.s)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE MATH.ASM REPL
undivert(math.s)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE SETJMP.ASM REPL
undivert(setjmp.s)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE SIGNAL.ASM REPL
undivert(signal.s)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE __MEMMGR.ASM REPL
undivert(__memmgr.s)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE PDPTEST.ASM REPL
undivert(pdptest.s)/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/inc rexx
'pdpasm start'
'pdpasm stdio'
'pdpasm stdlib'
'pdpasm ctype'
'pdpasm string'
'pdpasm time'
'pdpasm errno'
'pdpasm assert'
'pdpasm locale'
'pdpasm math'
'pdpasm setjmp'
'pdpasm signal'
'pdpasm __memmgr'
'pdpasm mussupa'
'pdpasm musstart'
'pdpasm pdptest'
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/sys region=9999
/file lmod n(pdptest.lmod) new(repl) lr(128) recfm(f) sp(100) shr
/load lked
/job map,nogo,print,stats,mode=os,name=gcclked
.org 4a00
/inc pdptest.obj
/INC MUSSTART.OBJ     GCCMU RUN-TIME LIBRARY MODULES
/INC MUSSUPA.OBJ
/INC __MEMMGR.OBJ
/INC ASSERT.OBJ
/INC CTYPE.OBJ
/INC ERRNO.OBJ
/INC LOCALE.OBJ
/INC MATH.OBJ
/INC SETJMP.OBJ
/INC SIGNAL.OBJ
/INC START.OBJ
/INC STDIO.OBJ
/INC STDLIB.OBJ
/INC STRING.OBJ
/INC TIME.OBJ
 ENTRY @@CRT0
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/sys region=3000,xregion=64m
/parm *
/inc dir
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/sys region=9999,xregion=64m
/file sysprint prt osrecfm(f) oslrecl(256)
/parm Hi there DeeeeeFerDog
/load xmon
gcclked n(pdptest.lmod) lcparm v(256)
/END
