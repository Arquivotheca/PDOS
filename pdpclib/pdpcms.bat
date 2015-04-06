del pdpcms.zip
gcccms -Os -DUSE_MEMMGR -S -I . start.c
gcccms -Os -DUSE_MEMMGR -S -I . stdio.c
gcccms -Os -DUSE_MEMMGR -S -I . stdlib.c
gcccms -Os -DUSE_MEMMGR -S -I . ctype.c
gcccms -Os -DUSE_MEMMGR -S -I . string.c
gcccms -Os -DUSE_MEMMGR -S -I . time.c
gcccms -Os -DUSE_MEMMGR -S -I . errno.c
gcccms -Os -DUSE_MEMMGR -S -I . assert.c
gcccms -Os -DUSE_MEMMGR -S -I . locale.c
gcccms -Os -DUSE_MEMMGR -S -I . math.c
gcccms -Os -DUSE_MEMMGR -S -I . setjmp.c
gcccms -Os -DUSE_MEMMGR -S -I . signal.c
gcccms -Os -DUSE_MEMMGR -S -I . __memmgr.c
gcccms -Os -DUSE_MEMMGR -S -I . pdptest.c
zip -0X pdpcms *.s *.exec *.asm *.mac

rem Useful for VM/380
call runcms pdpcms.exec output.txt pdpcms.zip

rem Useful for z/VM
rem mvsendec encb pdpcms.zip pdpcms.dat
rem loc2ebc pdpcms.dat xfer.card 80
