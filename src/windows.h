/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  windows prototypes                                               */
/*                                                                   */
/*********************************************************************/

typedef unsigned int DWORD;
typedef unsigned int BOOL;
typedef void *HANDLE;
#define WINAPI __stdcall
#define STD_INPUT_HANDLE ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE ((DWORD)-12)
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
typedef char *LPCSTR;
typedef unsigned int LPSECURITY_ATTRIBUTES;

HANDLE WINAPI GetStdHandle(DWORD d);
BOOL WINAPI WriteFile(HANDLE h, void *buf, DWORD count, DWORD *actual, void *unknown);
void WINAPI ExitProcess(int rc);
BOOL WINAPI CloseHandle(HANDLE h);
HANDLE WINAPI CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile);
