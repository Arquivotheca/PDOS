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
typedef unsigned int *LPDWORD;
typedef unsigned int BOOL;
typedef void *HANDLE;
typedef void *LPVOID;
typedef char *LPCSTR;
typedef char *LPSTR;
typedef char *LPTSTR;
typedef char **LPTCH;
typedef unsigned int LPSECURITY_ATTRIBUTES;
typedef void *LPSTARTUPINFOA;
typedef void *LPPROCESS_INFORMATION;

#define WINAPI __stdcall
#define STD_INPUT_HANDLE ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE ((DWORD)-12)
#define INVALID_HANDLE_VALUE ((HANDLE)-1)


HANDLE WINAPI GetStdHandle(DWORD d);

BOOL WINAPI WriteFile(HANDLE h, void *buf, DWORD count, DWORD *actual, void *unknown);

void WINAPI ExitProcess(int rc);

BOOL WINAPI CloseHandle(HANDLE h);

#define CreateFile CreateFileA
HANDLE WINAPI CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile);

#define CreateProcess CreateProcessA
BOOL WINAPI CreateProcessA(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation);

#define DeleteFile DeleteFileA
BOOL WINAPI DeleteFileA(LPCSTR lpFileName);

#define GetCommandLine GetCommandLineA
LPTSTR WINAPI GetCommandLineA(void);

LPTCH WINAPI GetEnvironmentStrings(void);

BOOL WINAPI GetExitCodeProcess(HANDLE h, LPDWORD lpExitCode);
