/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards, 3:711/934@fidonet.         */
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

#ifdef __OS2__
#define INCL_DOS
#include <os2.h>
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

int main(int argc, char **argv);

void __exit(int status);
void CTYP __exita(int status);

#ifndef __MVS__
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

#if defined(__MVS__)
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
    static char *argv[20];
    int rc;
#ifdef __OS2__
    ULONG maxFH;
    LONG reqFH;
#endif
#ifdef __MSDOS__
    unsigned char *env;
#endif
#ifdef __MVS__
    int parmLen;
    int progLen;
    char parmbuf[300];
#endif

#ifdef __PDOS__
    p = exep->psp;
    __abscor = exep->abscor;
    __vidptr = ABSADDR(0xb8000);
#endif    
#ifndef __MVS__
    stdin->hfile = 0;
    stdout->hfile = 1;
    stderr->hfile = 2;

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
#else
    stdin = fopen("dd:SYSIN", "r");
    stdout = fopen("dd:SYSPRINT", "w");
    stderr = stdout;
    parmLen = ((unsigned int)p[0] << 8) | (unsigned int)p[1];
    if (parmLen >= sizeof parmbuf - 2)
    {
        parmLen = sizeof parmbuf - 1 - 2;
    }
    /* We copy the parameter into our own area because
       the caller hasn't necessarily allocated room for
       a terminating NUL, nor is it necessarily correct
       to clobber the caller's are with NULs. */
    memcpy(parmbuf, p, parmLen + 2);
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
    if (*p == ' ')
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
        for (x = 1; x < 19; )
        {
            argv[x] = p;
            x++;
            p = strchr(p, ' ');
            if (p == NULL)
            {
                break;
            }
            else
            {
                *p = '\0';
                p++;
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
#ifdef __MVS__
    fclose(stdin);
    fclose(stdout);
#endif        
    __exita(status);
    return;
}
