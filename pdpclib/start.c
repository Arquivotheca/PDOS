/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  start.c - startup/termination code                               */
/*                                                                   */
/*********************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#if USE_MEMMGR
#include "__memmgr.h"
#endif

#define MAXPARMS 50 /* maximum number of arguments we can handle */

#ifdef __OS2__
#define INCL_DOS
#include <os2.h>
#endif

#ifdef __WIN32__
#include <windows.h>
#endif

#if (defined(__MSDOS__) && defined(__WATCOMC__))
#define CTYP __cdecl
#else
#define CTYP
#endif

#ifdef __MSDOS__
/* Must be unsigned as it is used for array index */
extern unsigned char *__envptr;
extern unsigned short __osver;
#endif

#ifdef __VSE__
#undef __CMS__
#endif

#ifdef __MVS__
int __tso = 0; /* is this a TSO environment? */
#endif

int main(int argc, char **argv);

void __exit(int status);
void CTYP __exita(int status);

#if !defined(__MVS__) && !defined(__CMS__) && !defined(__VSE__)
static char buffer1[BUFSIZ + 8];
static char buffer2[BUFSIZ + 8];
static char buffer3[BUFSIZ + 8];
#endif

#ifdef __PDOS__
#include <support.h>
#include <pos.h>
int __abscor;
unsigned char *__envptr;
char *__vidptr;
#endif

#if USE_MEMMGR
extern void *__lastsup; /* last thing supplied to memmgr */
#endif

char **__ep;

#if defined(__CMS__)
int __start(char *p, char *pgmname, char **ep)
#elif defined(__VSE__)
int __start(char *p, char *pgmname, char **ep)
#elif defined(__MVS__)
int __start(char *p, char *pgmname, int tso)
#elif defined(__PDOS__)
int __start(int *i1, int *i2, int *i3, POS_EPARMS *exep)
#else
int CTYP __start(char *p)
#endif
{
#ifdef __PDOS__
    char *p;
#endif
    int x;
    int argc;
    static char *argv[MAXPARMS + 1];
    int rc;
#ifdef __OS2__
    ULONG maxFH;
    LONG reqFH;
#endif
#ifdef __MSDOS__
    unsigned char *env;
#endif
#if defined(__MVS__) || defined(__CMS__) || defined(__VSE__)
    int parmLen;
    int progLen;
    char parmbuf[300];
#endif

#ifdef __PDOS__
    p = exep->psp;
    __abscor = exep->abscor;
    __vidptr = ABSADDR(0xb8000);
#endif

#ifdef __WIN32__
    p = GetCommandLine();
#endif

#if !defined(__MVS__) && !defined(__CMS__) && !defined(__VSE__)
#ifdef __WIN32__
    stdin->hfile = GetStdHandle(STD_INPUT_HANDLE);
    stdout->hfile = GetStdHandle(STD_OUTPUT_HANDLE);
    stderr->hfile = GetStdHandle(STD_ERROR_HANDLE);
#else
    stdin->hfile = 0;
    stdout->hfile = 1;
    stderr->hfile = 2;
#endif

    stdin->quickBin = 0;
    stdin->quickText = 0;
    stdin->textMode = 1;
    stdin->intFno = 0;
    stdin->bufStartR = 0;
    stdin->bufTech = _IOLBF;
    stdin->intBuffer = buffer1;
    stdin->fbuf = stdin->intBuffer + 2;
    *stdin->fbuf++ = '\0';
    *stdin->fbuf++ = '\0';
    stdin->szfbuf = BUFSIZ;
    stdin->endbuf = stdin->fbuf + stdin->szfbuf;
    *stdin->endbuf = '\n';
    stdin->noNl = 0;
    stdin->upto = stdin->endbuf;
    stdin->bufStartR = -stdin->szfbuf;
    stdin->mode = __READ_MODE;
    stdin->ungetCh = -1;
    stdin->update = 0;
    stdin->theirBuffer = 0;
    stdin->permfile = 1;
    stdin->isopen = 1;

    stdout->quickBin = 0;
    stdout->quickText = 0;
    stdout->textMode = 1;
    stdout->bufTech = _IOLBF;
    stdout->intBuffer = buffer2;
    stdout->fbuf = stdout->intBuffer;
    *stdout->fbuf++ = '\0';
    *stdout->fbuf++ = '\0';
    stdout->szfbuf = BUFSIZ;
    stdout->endbuf = stdout->fbuf + stdout->szfbuf;
    *stdout->endbuf = '\n';
    stdout->noNl = 0;
    stdout->upto = stdout->fbuf;
    stdout->bufStartR = 0;
    stdout->mode = __WRITE_MODE;
    stdout->update = 0;
    stdout->theirBuffer = 0;
    stdout->permfile = 1;
    stdout->isopen = 1;

    stderr->quickBin = 0;
    stderr->quickText = 0;
    stderr->textMode = 1;
    stderr->bufTech = _IOLBF;
    stderr->intBuffer = buffer3;
    stderr->fbuf = stderr->intBuffer;
    *stderr->fbuf++ = '\0';
    *stderr->fbuf++ = '\0';
    stderr->szfbuf = BUFSIZ;
    stderr->endbuf = stderr->fbuf + stderr->szfbuf;
    *stderr->endbuf = '\n';
    stderr->noNl = 0;
    stderr->upto = stderr->fbuf;
    stderr->bufStartR = 0;
    stderr->mode = __WRITE_MODE;
    stderr->update = 0;
    stderr->theirBuffer = 0;
    stderr->permfile = 1;
    stderr->isopen = 1;
#else
#if defined(__CMS__)
/*
  This code checks to see if DDs exist for SYSIN, SYSPRINT & SYSTERM
  if not it issues FD to the terminal
*/
    char s202parm [800]; /* svc 202 buffer */
    int code;
    int parm;
    int ret;

/*
 Now build the SVC 202 string for sysprint
*/
    memcpy ( &s202parm[0] ,  "FILEDEF ", 8);
    memcpy ( &s202parm[8] ,  "SYSPRINT", 8);
    memcpy ( &s202parm[16] , "(       ", 8);
    memcpy ( &s202parm[24] , "NOCHANGE", 8);
    s202parm[32]=s202parm[33]=s202parm[34]=s202parm[35]=
        s202parm[36]=s202parm[37]=s202parm[38]=s202parm[39]=0xff;
/*
  and issue the SVC
*/
    ret = __SVC202 ( s202parm, &code, &parm );
    if (ret == 24)
    { /* we need to issue filedef */
        memcpy ( &s202parm[16] , "TERM    ", 8);
        memcpy ( &s202parm[24] , "(       ", 8);
        memcpy ( &s202parm[32] , "LRECL   ", 8);
        memcpy ( &s202parm[40] , "80      ", 8);
        memcpy ( &s202parm[48] , "RECFM   ", 8);
        memcpy ( &s202parm[56] , "F       ", 8);
        s202parm[64]=s202parm[65]=s202parm[66]=s202parm[67]=
            s202parm[68]=s202parm[69]=s202parm[70]=s202parm[71]=0xff;

        ret = __SVC202 ( s202parm, &code, &parm );
    }

/*
 Now build the SVC 202 string for systerm
*/
    memcpy ( &s202parm[0] ,  "FILEDEF ", 8);
    memcpy ( &s202parm[8] ,  "SYSTERM ", 8);
    memcpy ( &s202parm[16] , "(       ", 8);
    memcpy ( &s202parm[24] , "NOCHANGE", 8);
    s202parm[32]=s202parm[33]=s202parm[34]=s202parm[35]=
        s202parm[36]=s202parm[37]=s202parm[38]=s202parm[39]=0xff;
/*
  and issue the SVC
*/
    ret = __SVC202 ( s202parm, &code, &parm );
    if (ret == 24)
    { /* we need to issue filedef */
        memcpy ( &s202parm[16] , "TERM    ", 8);
        memcpy ( &s202parm[24] , "(       ", 8);
        memcpy ( &s202parm[32] , "LRECL   ", 8);
        memcpy ( &s202parm[40] , "80      ", 8);
        memcpy ( &s202parm[48] , "RECFM   ", 8);
        memcpy ( &s202parm[56] , "F       ", 8);
        s202parm[64]=s202parm[65]=s202parm[66]=s202parm[67]=
            s202parm[68]=s202parm[69]=s202parm[70]=s202parm[71]=0xff;

        ret = __SVC202 ( s202parm, &code, &parm );
    }

/*
 Now build the SVC 202 string for sysin
*/
    memcpy ( &s202parm[0] ,  "FILEDEF ", 8);
    memcpy ( &s202parm[8] ,  "SYSIN   ", 8);
    memcpy ( &s202parm[16] , "(       ", 8);
    memcpy ( &s202parm[24] , "NOCHANGE", 8);
    s202parm[32]=s202parm[33]=s202parm[34]=s202parm[35]=
        s202parm[36]=s202parm[37]=s202parm[38]=s202parm[39]=0xff;
/*
  and issue the SVC
*/
    ret = __SVC202 ( s202parm, &code, &parm );

    if (ret == 24)
    { /* we need to issue filedef */
        memcpy ( &s202parm[16] , "TERM    ", 8);
        memcpy ( &s202parm[24] , "(       ", 8);
        memcpy ( &s202parm[32] , "LRECL   ", 8);
        memcpy ( &s202parm[40] , "80      ", 8);
        memcpy ( &s202parm[48] , "RECFM   ", 8);
        memcpy ( &s202parm[56] , "F       ", 8);
        s202parm[64]=s202parm[65]=s202parm[66]=s202parm[67]=
            s202parm[68]=s202parm[69]=s202parm[70]=s202parm[71]=0xff;

        ret = __SVC202 ( s202parm, &code, &parm );
    }

#endif
#if USE_MEMMGR
    memmgrDefaults(&__memmgr);
    memmgrInit(&__memmgr);
#endif
    stdout = fopen("dd:SYSPRINT", "w");
    if (stdout == NULL)
    {
        __exita(EXIT_FAILURE);
    }
    stdout->permfile = 1;

    stderr = fopen("dd:SYSTERM", "w");
    if (stderr == NULL)
    {
        printf("SYSTERM DD not defined\n");
        __exita(EXIT_FAILURE);
    }
    stderr->permfile = 1;

    stdin = fopen("dd:SYSIN", "r");
    if (stdin == NULL)
    {
        fprintf(stderr, "SYSIN DD not defined\n");
        fclose(stdout);
        __exita(EXIT_FAILURE);
    }
    stdin->permfile = 1;
#if defined(__CMS__)
    __ep = ep;
    /* if no parameters are provided, the tokenized
       plist will start with x'ff' */
    if (p[0] == 0xff)
    {
        parmLen = 0;
    }
    else if ((p[0] == '?') && (p[1] == ' '))
    {
        /* if the user starts the program with a single '?'
           then that is a signal to read the parameter string
           from dd:SYSPARM */
        FILE *pf;

        pf = fopen("dd:SYSPARM", "r");
        if (pf != NULL)
        {
            fgets(parmbuf + 2, sizeof parmbuf - 2, pf);
            fclose(pf);
            p = strchr(parmbuf + 2, '\n');
            if (p != NULL)
            {
                *p = '\0';
            }
            parmLen = strlen(parmbuf + 2);
        }
        else
        {
            parmLen = 0;
        }
    }
    else
    {
        /* copy across the tokenized plist, which
           consists of 8 character chunks, space-padded,
           and terminated by x'ff' */
        for (x = 0; x < sizeof parmbuf / 9 - 1; x++)
        {
            if (p[x * 8] == 0xff) break;
            memcpy(parmbuf + 2 + x * 9, p + x * 8, 8);
            parmbuf[2 + x * 9 + 8] = ' ';
        }
        parmbuf[2 + x * 9] = '\0';
        parmLen = strlen(parmbuf + 2);
    }
#elif defined(__VSE__)
    parmLen = 0;
#else
    parmLen = ((unsigned int)p[0] << 8) | (unsigned int)p[1];
    if (parmLen >= sizeof parmbuf - 2)
    {
        parmLen = sizeof parmbuf - 1 - 2;
    }
    /* We copy the parameter into our own area because
       the caller hasn't necessarily allocated room for
       a terminating NUL, nor is it necessarily correct
       to clobber the caller's area with NULs. */
    memcpy(parmbuf, p, parmLen + 2);
#endif
    p = parmbuf;
    if ((parmLen > 0) && (p[2] == 0))     /* assume TSO */
    {
        progLen = ((unsigned int)p[2] << 8) | (unsigned int)p[3];
        parmLen -= (progLen + 4);
        argv[0] = p + 4;
        p += (progLen + 4);
        if (parmLen > 0)
        {
            *(p - 1) = '\0';
        }
        else
        {
            *p = '\0';
        }
        p[parmLen] = '\0';
#ifdef __MVS__
        __tso = 1;
#endif
    }
    else         /* batch or tso "call" */
    {
        progLen = 0;
        p += 2;
        argv[0] = pgmname;
        pgmname[8] = '\0';
        pgmname = strchr(pgmname, ' ');
        if (pgmname != NULL)
        {
            *pgmname = '\0';
        }
        if (parmLen > 0)
        {
            p[parmLen] = '\0';
        }
        else
        {
            p = "";
        }
    }
#endif

#ifdef __OS2__
    reqFH = 0;
    DosSetRelMaxFH(&reqFH, &maxFH);
    if (maxFH < (FOPEN_MAX + 10))
    {
        reqFH = FOPEN_MAX - maxFH + 10;
        DosSetRelMaxFH(&reqFH, &maxFH);
    }
#endif
    for (x=0; x < __NFILE; x++)
    {
        __userFiles[x] = NULL;
    }
#ifdef __OS2__
    argv[0] = p;
    p += strlen(p) + 1;
#endif
#ifdef __WIN32__
    argv[0] = p;
    p = strchr(p, ' ');
    if (p == NULL)
    {
        p = "";
    }
    else
    {
        *p = '\0';
        p++;
    }
#endif
#if defined(__MSDOS__) || defined(__PDOS__)
    argv[0] = "";

#ifdef __MSDOS__
    if(__osver > 0x300)
    {
        env=__envptr;
        while (1)
        {
            if (*env++ == '\0' && *env++ == '\0')
            {
                if (*(unsigned short *)env != 0)
                {
                    argv[0] = (char *)env + 2;
                }
                break;
            }
        }
    }
#endif
    p = p + 0x80;
    p[*p + 1] = '\0';
    p++;
#endif
    while (*p == ' ')
    {
        p++;
    }
    if (*p == '\0')
    {
        argv[1] = NULL;
        argc = 1;
    }
    else
    {
        for (x = 1; x < MAXPARMS; )
        {
            char srch = ' ';

            if (*p == '"')
            {
                p++;
                srch = '"';
            }
            argv[x] = p;
            x++;
            p = strchr(p, srch);
            if (p == NULL)
            {
                break;
            }
            else
            {
                *p = '\0';
                p++;
                while (*p == ' ') p++;
                if (*p == '\0') break; /* strip trailing blanks */
            }
        }
        argv[x] = NULL;
        argc = x;
    }
#ifdef PDOS_MAIN_ENTRY
    *i1 = argc;
    *i2 = (int)argv;
    return (0);
#else
    rc = main(argc, argv);
    __exit(rc);
    return (rc);
#endif
}

void __exit(int status)
{
    int x;

#if 0
    for (x = __NATEXIT - 1; x >= 0; x--)
    {
        if (__userExit[x] != 0)
        {
            (__userExit[x])();
        }
    }
#endif
    for (x = 0; x < __NFILE; x++)
    {
        if (__userFiles[x] != NULL)
        {
            fclose(__userFiles[x]);
        }
    }
    fflush(stdout);
    fflush(stderr);
#if defined(__MVS__) || defined(__CMS__)
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
#endif


#if USE_MEMMGR
    memmgrTerm(&__memmgr);

/* release memory for most circumstances, although a
   better solution will be required eventually */
#if defined(__MVS__) || defined(__CMS__)
    if (__lastsup != NULL)
    {
        __freem(__lastsup);
    }
#endif
#endif /* USE_MEMMGR */


#ifdef __WIN32__
    ExitProcess(status);
#else
    __exita(status);
#endif
    return;
}
