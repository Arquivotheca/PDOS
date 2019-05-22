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

HANDLE WINAPI GetStdHandle(DWORD d);
BOOL WINAPI WriteFile(HANDLE h, void *buf, DWORD count, DWORD *actual, void *unknown);
void WINAPI ExitProcess(int rc);
