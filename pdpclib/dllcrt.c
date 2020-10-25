/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  dllcrt.c - entry point for WIN32 DLLs                            */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <locale.h>
#include <setjmp.h>

#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved);

BOOL WINAPI DllMainCRTStartup(HINSTANCE hinstDll,
                              DWORD fdwReason,
                              LPVOID lpvReserved)
{
    BOOL bRet;

    /* DllMain() is optional, so it would be good to handle that case too. */
    bRet = DllMain(hinstDll, fdwReason, lpvReserved);

    return (bRet);
}

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved)
{
#ifdef NEED_START
    /* define dummy calls to get the whole of PDPCLIB linked in */
    static jmp_buf jmpenv;

    __start(0);
    time(NULL);
    setlocale(LC_ALL, "");
    setjmp(jmpenv);
#endif

    return (TRUE);
}
