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
#include <math.h>

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

int __needdum = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved)
{
#ifdef NEED_START

    __start(0); /* this needs to be called */
    if (__needdum)
    {
        /* define dummy calls to get the whole of PDPCLIB linked in */
        jmp_buf jmpenv;

        time(NULL);
        localeconv();
        setjmp(jmpenv);
        sin(0.0);
    }
#endif

    return (TRUE);
}
