cd pdpclib
del pdptest.exe pdptesta.exe pdptestv.exe pdptestw.exe
del *.s *.o *.a
make -f makefile.pdw
ren pdptest.exe pdptesta.exe
del *.o *.a
call compmsvcrt
call comppdptest
ren pdptest.exe pdptestv.exe
del *.o *.a
make -f makefile.w32
ren pdptest.exe pdptestw.exe
cd ..
zip -9Xj pdptest pdpclib\pdptesta.exe pdpclib\pdptestv.exe pdpclib\pdptestw.exe
