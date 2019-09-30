cd pdpclib
call compmsvcrt
cd ..\src
call compk32
call compntd
cd ..
zip -9Xj dll pdpclib\libmsvcrt.a pdpclib\msvcrt.dll src\libkernel32.a src\kernel32.dll src\libntdll.a src\ntdll.dll
