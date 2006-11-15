/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  stdio.c - implementation of stuff in stdio.h                     */
/*                                                                   */
/*  The philosophy of this module is explained here.                 */
/*  There is a static array containing pointers to file objects.     */
/*  This is required in order to close all the files on program      */
/*  termination.                                                     */
/*                                                                   */
/*  In order to give speed absolute priority, so that people don't   */
/*  resort to calling DosRead themselves, there is a special flag    */
/*  in the FILE object called "quickbin".  If this flag is set to 1  */
/*  it means that it is a binary file and there is nothing in the    */
/*  buffer and there are no errors, so don't stuff around, just call */
/*  DosRead.                                                         */
/*                                                                   */
/*  When a buffer exists, which is most of the time, fbuf will point */
/*  to it.  The size of the buffer is given by szfbuf.  upto will    */
/*  point to the next character to be read.  endbuf will point PAST  */
/*  the last valid character in the buffer.  bufStartR represents    */
/*  the position in the file that the first character in the buffer  */
/*  is at.  This is only updated when a new buffer is read in.       */
/*                                                                   */
/*  After file open, for a file being read, bufStartR will actually  */
/*  be a negative number, which if added to the position of upto     */
/*  will get to 0.  On a file being written, bufStartR will be set   */
/*  to 0, and upto will point to the start of the buffer.  The       */
/*  reason for the difference on the read is in order to tell the    */
/*  difference between an empty buffer and a buffer with data in it, */
/*  but which hasn't been used yet.  The alternative would be to     */
/*  either keep track of a flag, or make fopen read in an initial    */
/*  buffer.  But we want to avoid reading in data that no-one has    */
/*  yet requested.                                                   */
/*                                                                   */
/*  The buffer is organized as follows...                            */
/*  What we have is an internal buffer, which is 8 characters        */
/*  longer than the actually used buffer.  E.g. say BUFSIZ is        */
/*  512 bytes, then we actually allocate 520 bytes.  The first       */
/*  2 characters will be junk, the next 2 characters set to NUL,     */
/*  for protection against some backward-compares.  The fourth-last  */
/*  character is set to '\n', to protect against overscan.  The      */
/*  last 3 characters will be junk, to protect against memory        */
/*  violation.  intBuffer is the internal buffer, but everyone       */
/*  refers to fbuf, which is actually set to the &intBuffer[4].      */
/*  Also, szfbuf is the size of the "visible" buffer, not the        */
/*  internal buffer.  The reason for the 2 junk characters at the    */
/*  beginning is to align the buffer on a 4-byte boundary.           */
/*                                                                   */
/*                                                                   */
/*********************************************************************/

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdarg.h"
#include "ctype.h"
#include "errno.h"
#include "limits.h"

/* PDOS and MSDOS use the same interface most of the time */
#ifdef __PDOS__
#define __MSDOS__
#endif

#ifdef __MSDOS__
#ifdef __WATCOMC__
#define CTYP __cdecl
#else
#define CTYP
#endif
extern int CTYP __open(const char *filename, int mode, int *errind);
extern int CTYP __read(int handle, void *buf, size_t len, int *errind);
extern int CTYP __write(int handle, const void *buf, size_t len, int *errind);
extern void CTYP __seek(int handle, long offset, int whence);
extern void CTYP __close(int handle);
extern void CTYP __remove(const char *filename);
extern void CTYP __rename(const char *old, const char *new);
#endif

#ifdef __OS2__
#include <os2.h>
#endif

#if defined(__MVS__) || defined(__CMS__)
#include "mvssupa.h"
#define FIXED_BINARY 0
#define VARIABLE_BINARY 1
#define FIXED_TEXT 2
#define VARIABLE_TEXT 3
#endif

static FILE permFiles[3];

#define unused(x) ((void)(x))
#define outch(ch) ((fq == NULL) ? *s++ = (char)ch : putc(ch, fq))
#define inch() ((fp == NULL) ? \
    (ch = (unsigned char)*s++) : (ch = getc(fp)))

FILE *stdin = &permFiles[0];
FILE *stdout = &permFiles[1];
FILE *stderr = &permFiles[2];

FILE *__userFiles[__NFILE];
static FILE  *myfile;
static int    spareSpot;
static int    err;

static const char *fnm;
static const char *modus;
static int modeType;

static void dblcvt(double num, char cnvtype, size_t nwidth,
                   size_t nprecision, char *result);
static int vvprintf(const char *format, va_list arg, FILE *fq, char *s);
static int vvscanf(const char *format, va_list arg, FILE *fp, const char *s);
static void fopen2(void);
static void fopen3(void);
static void findSpareSpot(void);
static void checkMode(void);
static void osfopen(void);

#if !defined(__MVS__) && !defined(__CMS__)
static void fwriteSlow(const void *ptr,
                       size_t size,
                       size_t nmemb,
                       FILE *stream,
                       size_t towrite,
                       size_t *elemWritten);
static void fwriteSlowT(const void *ptr,
                        FILE *stream,
                        size_t towrite,
                        size_t *actualWritten);
static void fwriteSlowB(const void *ptr,
                        FILE *stream,
                        size_t towrite,
                        size_t *actualWritten);
static void freadSlowT(void *ptr,
                       FILE *stream,
                       size_t toread,
                       size_t *actualRead);
static void freadSlowB(void *ptr,
                       FILE *stream,
                       size_t toread,
                       size_t *actualRead);
#endif

static int examine(const char **formt, FILE *fq, char *s, va_list *arg,
                   int chcount);

#ifdef __CMS__
static void filedef(char *fdddname, char *fnm, int mymode);
#endif

int printf(const char *format, ...)
{
    va_list arg;
    int ret;

    va_start(arg, format);
    ret = vfprintf(stdout, format, arg);
    va_end(arg);
    return (ret);
}

int fprintf(FILE *stream, const char *format, ...)
{
    va_list arg;
    int ret;

    va_start(arg, format);
    ret = vfprintf(stream, format, arg);
    va_end(arg);
    return (ret);
}

int vfprintf(FILE *stream, const char *format, va_list arg)
{
    int ret;

    ret = vvprintf(format, arg, stream, NULL);
    return (ret);
}

FILE *fopen(const char *filename, const char *mode)
{
    fnm = filename;
    modus = mode;
    err = 0;
    findSpareSpot();
    if (!err)
    {
        myfile = malloc(sizeof(FILE));
        if (myfile == NULL)
        {
            err = 1;
        }
        else
        {
            fopen2();
            if (err)
            {
                free(myfile);
            }
        }
    }
    if (err)
    {
        myfile = NULL;
    }
    return (myfile);
}

static void fopen2(void)
{
    checkMode();
    if (!err)
    {
        strcpy(myfile->modeStr, modus);
        osfopen();
        if (!err)
        {
            __userFiles[spareSpot] = myfile;
            myfile->intFno = spareSpot;
            fopen3();
        }
    }
    return;
}

static void fopen3(void)
{
    myfile->intBuffer = malloc(BUFSIZ + 8);
    if (myfile->intBuffer == NULL)
    {
        err = 1;
    }
    else
    {
        myfile->theirBuffer = 0;
        myfile->fbuf = myfile->intBuffer + 2;
        *myfile->fbuf++ = '\0';
        *myfile->fbuf++ = '\0';
        myfile->szfbuf = BUFSIZ;
#if !defined(__MVS__) && !defined(__CMS__)
        myfile->quickText = 0;
#endif
        myfile->noNl = 0;
        myfile->endbuf = myfile->fbuf + myfile->szfbuf;
        *myfile->endbuf = '\n';
#if defined(__MVS__) || defined(__CMS__)
        myfile->upto = myfile->fbuf;
        myfile->szfbuf = myfile->lrecl;
        myfile->endbuf = myfile->fbuf; /* for read only */
#else
        myfile->upto = myfile->endbuf;
#endif
        myfile->bufStartR = -(long)myfile->szfbuf;
        myfile->errorInd = 0;
        myfile->eofInd = 0;
        myfile->ungetCh = -1;
        myfile->update = 0;
        myfile->permfile = 0;
        myfile->isopen = 1;
#if !defined(__MVS__) && !defined(__CMS__)
        if (!myfile->textMode)
        {
            myfile->quickBin = 1;
        }
        else
        {
            myfile->quickBin = 0;
        }
#endif
        myfile->mode = __READ_MODE;
        switch (modeType)
        {
            case 2:
            case 3:
            case 5:
            case 6:
            case 8:
            case 9:
            case 11:
            case 12:
                myfile->bufStartR = 0;
                myfile->upto = myfile->fbuf;
                myfile->mode = __WRITE_MODE;
#if defined(__MVS__) || defined(__CMS__)
                myfile->endbuf = myfile->fbuf + myfile->szfbuf;
#endif
                break;
        }
        switch (modeType)
        {
            case 7:
            case 8:
            case 10:
            case 11:
            case 12:
                myfile->update = 1;
                break;
        }
    }
    return;
}

static void findSpareSpot(void)
{
    int x;

    for (x = 0; x < __NFILE; x++)
    {
        if (__userFiles[x] == NULL)
        {
            break;
        }
    }
    if (x == __NFILE)
    {
        err = 1;
    }
    else
    {
        spareSpot = x;
    }
    return;
}

/* checkMode - interpret mode string */
/* r = 1 */
/* w = 2 */
/* a = 3 */
/* rb = 4 */
/* wb = 5 */
/* ab = 6 */
/* r+ = 7 */
/* w+ = 8 */
/* a+ = 9 */
/* r+b or rb+ = 10 */
/* w+b or wb+ = 11 */
/* a+b or ab+ = 12 */

static void checkMode(void)
{
    if (strncmp(modus, "r+b", 3) == 0)
    {
        modeType = 10;
    }
    else if (strncmp(modus, "rb+", 3) == 0)
    {
        modeType = 10;
    }
    else if (strncmp(modus, "w+b", 3) == 0)
    {
        modeType = 11;
    }
    else if (strncmp(modus, "wb+", 3) == 0)
    {
        modeType = 11;
    }
    else if (strncmp(modus, "a+b", 3) == 0)
    {
        modeType = 12;
    }
    else if (strncmp(modus, "ab+", 3) == 0)
    {
        modeType = 12;
    }
    else if (strncmp(modus, "r+", 2) == 0)
    {
        modeType = 7;
    }
    else if (strncmp(modus, "w+", 2) == 0)
    {
        modeType = 8;
    }
    else if (strncmp(modus, "a+", 2) == 0)
    {
        modeType = 9;
    }
    else if (strncmp(modus, "rb", 2) == 0)
    {
        modeType = 4;
    }
    else if (strncmp(modus, "wb", 2) == 0)
    {
        modeType = 5;
    }
    else if (strncmp(modus, "ab", 2) == 0)
    {
        modeType = 6;
    }
    else if (strncmp(modus, "r", 1) == 0)
    {
        modeType = 1;
    }
    else if (strncmp(modus, "w", 1) == 0)
    {
        modeType = 2;
    }
    else if (strncmp(modus, "a", 1) == 0)
    {
        modeType = 3;
    }
    else
    {
        err = 1;
        return;
    }
    if ((modeType == 4)
        || (modeType == 5)
        || (modeType == 6)
        || (modeType == 10)
        || (modeType == 11)
        || (modeType == 12))
    {
        myfile->textMode = 0;
    }
    else
    {
        myfile->textMode = 1;
    }
    return;
}

static void osfopen(void)
{
#ifdef __OS2__
    APIRET rc;
    ULONG  action;
    ULONG  newsize = 0;
    ULONG  fileAttr = 0;
    ULONG  openAction = 0;
    ULONG  openMode = 0;

    if ((modeType == 1) || (modeType == 4) || (modeType == 7)
        || (modeType == 10))
    {
        openAction |= OPEN_ACTION_FAIL_IF_NEW;
        openAction |= OPEN_ACTION_OPEN_IF_EXISTS;
    }
    else if ((modeType == 2) || (modeType == 5) || (modeType == 8)
             || (modeType == 11))
    {
        openAction |= OPEN_ACTION_CREATE_IF_NEW;
        openAction |= OPEN_ACTION_REPLACE_IF_EXISTS;
    }
    else if ((modeType == 3) || (modeType == 6) || (modeType == 9)
             || (modeType == 12))
    {
        openAction |= OPEN_ACTION_CREATE_IF_NEW;
        openAction |= OPEN_ACTION_OPEN_IF_EXISTS;
    }
    openMode |= OPEN_SHARE_DENYWRITE;
    if ((modeType == 1) || (modeType == 4))
    {
        openMode |= OPEN_ACCESS_READONLY;
    }
    else if ((modeType == 2) || (modeType == 3) || (modeType == 5)
             || (modeType == 6))
    {
        openMode |= OPEN_ACCESS_WRITEONLY;
    }
    else
    {
        openMode |= OPEN_ACCESS_READWRITE;
    }
    if ((strlen(fnm) == 2)
        && (fnm[1] == ':')
        && (openMode == OPEN_ACCESS_READONLY))
    {
        openMode |= OPEN_FLAGS_DASD;
    }
    rc = DosOpen((PSZ)fnm,
                 &myfile->hfile,
                 &action,
                 newsize,
                 fileAttr,
                 openAction,
                 openMode,
                 NULL);
    if (rc != 0)
    {
        err = 1;
        errno = rc;
    }
#endif
#ifdef __MSDOS__
    int mode;
    int errind;
     
    if ((modeType == 1) || (modeType == 4))
    {
        mode = 0;
    }
    else if ((modeType == 2) || (modeType == 5))
    {
        mode = 1;
    }
    else
    {
        mode = 2;
    }
    myfile->hfile = __open(fnm, mode, &errind);
    if (errind)
    {
        err = 1;
        errno = myfile->hfile;
    }
#endif
#if defined(__MVS__) || defined(__CMS__)
    int mode;
    char *p;
    int len;
#ifdef __CMS__
    char tmpdd[9];
#endif


    if ((modeType == 1) || (modeType == 4))
    {
        mode = 0;
    }
    else if ((modeType == 2) || (modeType == 5))
    {
        mode = 1;
    }
    else
    {
        mode = 2;
    }
/* dw */
/* This code needs changing for VM */
    p = strchr(fnm, ':');
    if ((p != NULL) 
        && ((strncmp(fnm, "dd", 2) == 0)
            || (strncmp(fnm, "DD", 2) == 0)))
    {
        p++;
    }
    else
/* if we are in here then there is no "dd:" on front of file */ 
/* if its CMS generate a ddname and issue a filedef for the file */
#ifdef __CMS__
    {
        char newfnm[FILENAME_MAX];
        
/* create a DD from the handle number */
        strcpy(newfnm, fnm);
        p = newfnm;
        while (*p != '\0')
        {
            *p = toupper((unsigned char)*p);
            p++;
        }
        sprintf(tmpdd, "GCC%03dHD", spareSpot);
        filedef(tmpdd, newfnm, mode);
        p = tmpdd;
    }
#else
    {
        p = (char *)fnm;
    }
#endif
    strcpy(myfile->ddname, "        ");
    len = strcspn(p, "(");
    if (len > 8)
    {
        len = 8;
    }
    memcpy(myfile->ddname, p, len);
    p = myfile->ddname;
    while (*p != '\0')
    {
        *p = toupper((unsigned char)*p);
        p++;
    }

    p = strchr(fnm, '(');
    if (p != NULL)
    {
        p++;
        strcpy(myfile->pdsmem, "        ");
        len = strcspn(p, ")");
        if (len > 8)
        {
            len = 8;
        }
        memcpy(myfile->pdsmem, p, len);
        p = myfile->pdsmem;
        while (*p != '\0')
        {
            *p = toupper((unsigned char)*p);
            p++;
        }
        p = myfile->pdsmem;
    }
    myfile->hfile = 
        __aopen(myfile->ddname, mode, &myfile->recfm, &myfile->lrecl, p);
/* dw mod to return error on open fail */
    if(myfile->hfile == NULL)
    {
        err = 1;
        return;
    }
/* dw end of mod */
    if ((modeType == 4) || (modeType == 5))
    {
        myfile->style = 0; /* binary */
    }
    else
    {
        myfile->style = 2; /* text */
    }
    myfile->style += myfile->recfm;
    if (myfile->style == VARIABLE_TEXT)
    {
        myfile->quickText = 1;
    }
    else
    {
        myfile->quickText = 0;
    }
    if (myfile->style == FIXED_BINARY)
    {
        myfile->quickBin = 1;
    }
    else
    {
        myfile->quickBin = 0;
    }
#endif
    return;
}

int fclose(FILE *stream)
{
#ifdef __OS2__
    APIRET rc;
#endif

    if (!stream->isopen)
    {
        return (EOF);
    }
    fflush(stream);
#ifdef __OS2__
    rc = DosClose(stream->hfile);
#endif
#ifdef __MSDOS__
    __close(stream->hfile);
#endif
#if defined(__MVS__) || defined(__CMS__)
    if ((stream->mode == __WRITE_MODE) && (stream->upto != stream->fbuf))
    {
        if (stream->textMode)
        {
            putc('\n', stream);
        }
        else
        {
            size_t remain;
            size_t x;
            
            remain = stream->endbuf - stream->upto;
            for (x = 0; x < remain; x++)
            {
                putc(0x00, stream);
            }
        }
    }
    __aclose(stream->hfile);
#endif
    if (!stream->theirBuffer)
    {
        free(stream->intBuffer);
    }
    if (!stream->permfile)
    {
        __userFiles[stream->intFno] = NULL;
        free(stream);
    }
    else
    {
        stream->isopen = 0;
    }
#ifdef __OS2__
    if (rc != 0)
    {
        errno = rc;
        return (EOF);
    }
#endif
    return (0);
}

#if !defined(__MVS__) && !defined(__CMS__)
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t toread;
    size_t elemRead;
    size_t actualRead;
#ifdef __OS2__
    APIRET rc;
    ULONG tempRead;
#endif
#ifdef __MSDOS__
    int errind;
    size_t tempRead;
#endif

    if (nmemb == 1)
    {
        toread = size;
    }
    else if (size == 1)
    {
        toread = nmemb;
    }
    else
    {
        toread = size * nmemb;
    }
    if (toread < stream->szfbuf)
    {
        stream->quickBin = 0;
    }
    if (!stream->quickBin)
    {
        if (stream->textMode)
        {
            freadSlowT(ptr, stream, toread, &actualRead);
        }
        else
        {
            if (toread <= (stream->endbuf - stream->upto))
            {
                memcpy(ptr, stream->upto, toread);
                actualRead = toread;
                stream->upto += toread;
            }
            else
            {
                freadSlowB(ptr, stream, toread, &actualRead);
            }
        }
        if (nmemb == 1)
        {
            if (actualRead == size)
            {
                elemRead = 1;
            }
            else
            {
                elemRead = 0;
            }
        }
        else if (size == 1)
        {
            elemRead = actualRead;
        }
        else
        {
            elemRead = actualRead / size;
        }
        return (elemRead);
    }
    else
    {
#ifdef __OS2__
        rc = DosRead(stream->hfile, ptr, toread, &tempRead);
        if (rc != 0)
        {
            actualRead = 0;
            stream->errorInd = 1;
            errno = rc;
        }
        else
        {
            actualRead = tempRead;
        }
#endif
#ifdef __MSDOS__
        tempRead = __read(stream->hfile, ptr, toread, &errind);
        if (errind)
        {
            errno = tempRead;
            actualRead = 0;
            stream->errorInd = 1;
        }
        else
        {
            actualRead = tempRead;
        }
#endif
        if (nmemb == 1)
        {
            if (actualRead == size)
            {
                elemRead = 1;
            }
            else
            {
                elemRead = 0;
                stream->eofInd = 1;
            }
        }
        else if (size == 1)
        {
            elemRead = actualRead;
            if (nmemb != actualRead)
            {
                stream->eofInd = 1;
            }
        }
        else
        {
            elemRead = actualRead / size;
            if (toread != actualRead)
            {
                stream->eofInd = 1;
            }
        }
        stream->bufStartR += actualRead;
        return (elemRead);
    }
}


/*
while toread has not been satisfied
{
    scan stuff out of buffer, replenishing buffer as required
}
*/

static void freadSlowT(void *ptr,
                       FILE *stream,
                       size_t toread,
                       size_t *actualRead)
{
    int finReading = 0;
    size_t avail;
    size_t need;
    char *p;
    size_t got;
#ifdef __OS2__
    ULONG tempRead;
    APIRET rc;
#endif
#ifdef __MSDOS__
    size_t tempRead;
    int errind;
#endif

    *actualRead = 0;
    while (!finReading)
    {
        if (stream->upto == stream->endbuf)
        {
            stream->bufStartR += (stream->upto - stream->fbuf);
#ifdef __OS2__
            rc = DosRead(stream->hfile,
                         stream->fbuf,
                         stream->szfbuf,
                         &tempRead);
            if (rc != 0)
            {
                tempRead = 0;
                stream->errorInd = 1;
                errno = rc;
            }
#endif
#ifdef __MSDOS__
            tempRead = __read(stream->hfile,
                              stream->fbuf,
                              stream->szfbuf,
                              &errind);
            if (errind)
            {
                errno = tempRead;
                tempRead = 0;
                stream->errorInd = 1;
            }
#endif
            if (tempRead == 0)
            {
                stream->eofInd = 1;
                break;
            }
            stream->endbuf = stream->fbuf + tempRead;
            *stream->endbuf = '\n';
            stream->upto = stream->fbuf;
        }
        avail = (size_t)(stream->endbuf - stream->upto) + 1;
        need = toread - *actualRead;
        p = memchr(stream->upto, '\n', avail);
        got = (size_t)(p - stream->upto);
        if (need < got)
        {
            memcpy((char *)ptr + *actualRead, stream->upto, need);
            stream->upto += need;
            *actualRead += need;
        }
        else
        {
            memcpy((char *)ptr + *actualRead, stream->upto, got);
            stream->upto += got;
            *actualRead += got;
            if (p != stream->endbuf)
            {
                if (*(stream->upto - 1) == '\r')
                {
                    *((char *)ptr + *actualRead - 1) = '\n';
                }
                else
                {
                    *((char *)ptr + *actualRead) = '\n';
                    *actualRead += 1;
                }
                stream->upto++;
            }
            else
            {
                if (*(stream->upto - 1) == '\r')
                {
                    *actualRead -= 1;
                }
            }
        }
        if (*actualRead == toread)
        {
            finReading = 1;
        }
    }
    return;
}

static void freadSlowB(void *ptr,
                       FILE *stream,
                       size_t toread,
                       size_t *actualRead)
{
    size_t avail;
#ifdef __OS2__
    ULONG tempRead;
    APIRET rc;
#endif
#ifdef __MSDOS__
    size_t tempRead;
    int errind;
#endif

    avail = (size_t)(stream->endbuf - stream->upto);
    memcpy(ptr, stream->upto, avail);
    *actualRead = avail;
    stream->bufStartR += (stream->endbuf - stream->fbuf);
    if (toread >= stream->szfbuf)
    {
        stream->upto = stream->endbuf;
        stream->quickBin = 1;
#ifdef __OS2__
        rc = DosRead(stream->hfile,
                     (char *)ptr + *actualRead,
                     toread - *actualRead,
                     &tempRead);
        if (rc != 0)
        {
            tempRead = 0;
            stream->errorInd = 1;
            errno = rc;
        }
#endif
#ifdef __MSDOS__
        tempRead = __read(stream->hfile,
                          (char *)ptr + *actualRead,
                          toread - *actualRead,
                          &errind);
        if (errind)
        {
            errno = tempRead;
            tempRead = 0;
            stream->errorInd = 1;
        }
#endif
        else if (tempRead != (toread - *actualRead))
        {
            stream->eofInd = 1;
        }
        *actualRead += tempRead;
        stream->bufStartR += tempRead;
    }
    else
    {
        size_t left;

        stream->upto = stream->fbuf;
#ifdef __OS2__
        rc = DosRead(stream->hfile,
                     stream->fbuf,
                     stream->szfbuf,
                     &tempRead);
        left = toread - *actualRead;
        if (rc != 0)
        {
            tempRead = 0;
            stream->errorInd = 1;
            errno = rc;
        }
#endif
#ifdef __MSDOS__
        tempRead = __read(stream->hfile,
                          stream->fbuf,
                          stream->szfbuf,
                          &errind);
        left = toread - *actualRead;
        if (errind)
        {
            errno = tempRead;
            tempRead = 0;
            stream->errorInd = 1;
        }
#endif
        else if (tempRead < left)
        {
            stream->eofInd = 1;
        }
        stream->endbuf = stream->fbuf + tempRead;
        *stream->endbuf = '\n';
        avail = (size_t)(stream->endbuf - stream->upto);
        if (avail > left)
        {
            avail = left;
        }
        memcpy((char *)ptr + *actualRead,
               stream->upto,
               avail);
        stream->upto += avail;
        *actualRead += avail;
    }
    return;
}
#endif

#if !defined(__MVS__) && !defined(__CMS__)
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t towrite;
    size_t elemWritten;
#ifdef __OS2__
    ULONG actualWritten;
    APIRET rc;
#endif
#ifdef __MSDOS__
    size_t actualWritten;
    int errind;
#endif

    if (nmemb == 1)
    {
        towrite = size;
    }
    else if (size == 1)
    {
        towrite = nmemb;
    }
    else
    {
        towrite = size * nmemb;
    }
    if (towrite < stream->szfbuf)
    {
        stream->quickBin = 0;
        if ((stream->bufTech == _IONBF) && !stream->textMode)
        {
            stream->quickBin = 1;
        }
    }
    if (!stream->quickBin)
    {
        fwriteSlow(ptr, size, nmemb, stream, towrite, &elemWritten);
        return (elemWritten);
    }
    else
    {
#ifdef __OS2__
        rc = DosWrite(stream->hfile, (VOID *)ptr, towrite, &actualWritten);
        if (rc != 0)
        {
            stream->errorInd = 1;
            actualWritten = 0;
            errno = rc;
        }
#endif
#ifdef __MSDOS__
        actualWritten = __write(stream->hfile,
                                ptr,
                                towrite,
                                &errind);
        if (errind)
        {
            stream->errorInd = 1;
            actualWritten = 0;
            errno = actualWritten;
        }
#endif
        if (nmemb == 1)
        {
            if (actualWritten == size)
            {
                elemWritten = 1;
            }
            else
            {
                elemWritten = 0;
            }
        }
        else if (size == 1)
        {
            elemWritten = actualWritten;
        }
        else
        {
            elemWritten = actualWritten / size;
        }
        stream->bufStartR += actualWritten;
        return (elemWritten);
    }
}

static void fwriteSlow(const void *ptr,
                       size_t size,
                       size_t nmemb,
                       FILE *stream,
                       size_t towrite,
                       size_t *elemWritten)
{
    size_t actualWritten;

    /* Normally, on output, there will never be a situation where
       the write buffer is full, but it hasn't been written out.
       If we find this to be the case, then it is because we have
       done an fseek, and didn't know whether we were going to do
       a read or a write after it, so now that we know, we switch
       the buffer to being set up for write.  We could use a flag,
       but I thought it would be better to just put some magic
       code in with a comment */
    if (stream->upto == stream->endbuf)
    {
        stream->bufStartR += (stream->endbuf - stream->fbuf);
        stream->upto = stream->fbuf;
        stream->mode = __WRITE_MODE;
    }
    if ((stream->textMode) || (stream->bufTech == _IOLBF))
    {
        fwriteSlowT(ptr, stream, towrite, &actualWritten);
    }
    else
    {
        fwriteSlowB(ptr, stream, towrite, &actualWritten);
    }
    if (nmemb == 1)
    {
        if (actualWritten == size)
        {
            *elemWritten = 1;
        }
        else
        {
            *elemWritten = 0;
        }
    }
    else if (size == 1)
    {
        *elemWritten = actualWritten;
    }
    else
    {
        *elemWritten = actualWritten / size;
    }
    return;
}


/* can still be called on binary files, if the binary file is
   line buffered  */

static void fwriteSlowT(const void *ptr,
                        FILE *stream,
                        size_t towrite,
                        size_t *actualWritten)
{
    char *p;
    char *tptr;
    char *oldp;
    size_t diffp;
    size_t rem;
    int fin;
#ifdef __OS2__
    ULONG tempWritten;
    APIRET rc;
#endif
#ifdef __MSDOS__
    size_t tempWritten;
    int errind;
#endif

    *actualWritten = 0;
    tptr = (char *)ptr;
    p = tptr;
    oldp = p;
    p = (char *)memchr(oldp, '\n', towrite - (size_t)(oldp - tptr));
    while (p != NULL)
    {
        diffp = (size_t)(p - oldp);
        fin = 0;
        while (!fin)
        {
            rem = (size_t)(stream->endbuf - stream->upto);
            if (diffp < rem)
            {
                memcpy(stream->upto, oldp, diffp);
                stream->upto += diffp;
                *actualWritten += diffp;
                fin = 1;
            }
            else
            {
                memcpy(stream->upto, oldp, rem);
                oldp += rem;
                diffp -= rem;
#ifdef __OS2__
                rc = DosWrite(stream->hfile,
                              stream->fbuf,
                              stream->szfbuf,
                              &tempWritten);
                if (rc != 0)
                {
                    stream->errorInd = 1;
                    return;
                }
#endif
#ifdef __MSDOS__
                tempWritten = __write(stream->hfile,
                                      stream->fbuf,
                                      stream->szfbuf,
                                      &errind);
                if (errind)
                {
                    stream->errorInd = 1;
                    return;
                }
#endif
                else
                {
                    *actualWritten += rem;
                    stream->upto = stream->fbuf;
                    stream->bufStartR += tempWritten;
                }
            }
        }
        rem = (size_t)(stream->endbuf - stream->upto);
        if (rem < 2)
        {
#ifdef __OS2__
            rc = DosWrite(stream->hfile,
                          stream->fbuf,
                          (size_t)(stream->upto - stream->fbuf),
                          &tempWritten);
            if (rc != 0)
            {
                stream->errorInd = 1;
                errno = rc;
                return;
            }
#endif
#ifdef __MSDOS__
            tempWritten = __write(stream->hfile,
                                  stream->fbuf,
                                  (size_t)(stream->upto - stream->fbuf),
                                  &errind);
            if (errind)
            {
                stream->errorInd = 1;
                errno = tempWritten;
                return;
            }
#endif
            stream->upto = stream->fbuf;
            stream->bufStartR += tempWritten;
        }
        if (stream->textMode)
        {
            memcpy(stream->upto, "\r\n", 2);
            stream->upto += 2;
        }
        else
        {
            memcpy(stream->upto, "\n", 1);
            stream->upto += 1;
        }
        *actualWritten += 1;
        oldp = p + 1;
        p = (char *)memchr(oldp, '\n', towrite - (size_t)(oldp - tptr));
    }

    if ((stream->bufTech == _IOLBF)
        && (stream->upto != stream->fbuf)
        && (oldp != tptr))
    {
#ifdef __OS2__
        rc = DosWrite(stream->hfile,
                      stream->fbuf,
                      (size_t)(stream->upto - stream->fbuf),
                      &tempWritten);
        if (rc != 0)
        {
            stream->errorInd = 1;
            errno = rc;
            return;
        }
#endif
#ifdef __MSDOS__
        tempWritten = __write(stream->hfile,
                              stream->fbuf,
                              (size_t)(stream->upto - stream->fbuf),
                              &errind);
        if (errind)
        {
            stream->errorInd = 1;
            errno = tempWritten;
            return;
        }
#endif
        stream->upto = stream->fbuf;
        stream->bufStartR += tempWritten;
    }

    diffp = towrite - *actualWritten;
    while (diffp != 0)
    {
        rem = (size_t)(stream->endbuf - stream->upto);
        if (diffp < rem)
        {
            memcpy(stream->upto, oldp, diffp);
            stream->upto += diffp;
            *actualWritten += diffp;
        }
        else
        {
            memcpy(stream->upto, oldp, rem);
#ifdef __OS2__
            rc = DosWrite(stream->hfile,
                          stream->fbuf,
                          stream->szfbuf,
                          &tempWritten);
            if (rc != 0)
            {
                stream->errorInd = 1;
                errno = rc;
                return;
            }
#endif
#ifdef __MSDOS__
            tempWritten = __write(stream->hfile,
                                  stream->fbuf,
                                  stream->szfbuf,
                                  &errind);
            if (errind)
            {
                stream->errorInd = 1;
                errno = tempWritten;
                return;
            }
#endif
            else
            {
                *actualWritten += rem;
                stream->upto = stream->fbuf;
            }
            stream->bufStartR += tempWritten;
            oldp += rem;
        }
        diffp = towrite - *actualWritten;
    }
    if ((stream->bufTech == _IONBF)
        && (stream->upto != stream->fbuf))
    {
#ifdef __OS2__
        rc = DosWrite(stream->hfile,
                      stream->fbuf,
                      (size_t)(stream->upto - stream->fbuf),
                      &tempWritten);
        if (rc != 0)
        {
            stream->errorInd = 1;
            errno = rc;
            return;
        }
#endif
#ifdef __MSDOS__
        tempWritten = __write(stream->hfile,
                              stream->fbuf,
                              (size_t)(stream->upto - stream->fbuf),
                              &errind);
        if (errind)
        {
            stream->errorInd = 1;
            errno = tempWritten;
            return;
        }
#endif
        stream->upto = stream->fbuf;
        stream->bufStartR += tempWritten;
    }
    return;
}

/* whilst write requests are smaller than a buffer, we do not turn
   on quickbin */

static void fwriteSlowB(const void *ptr,
                        FILE *stream,
                        size_t towrite,
                        size_t *actualWritten)
{
    size_t spare;
#ifdef __OS2__
    ULONG tempWritten;
    APIRET rc;
#endif
#ifdef __MSDOS__
    size_t tempWritten;
    int errind;
#endif

    spare = (size_t)(stream->endbuf - stream->upto);
    if (towrite < spare)
    {
        memcpy(stream->upto, ptr, towrite);
        *actualWritten = towrite;
        stream->upto += towrite;
        return;
    }
    memcpy(stream->upto, ptr, spare);
#ifdef __OS2__
    rc = DosWrite(stream->hfile,
                  stream->fbuf,
                  stream->szfbuf,
                  &tempWritten);
    if (rc != 0)
    {
        stream->errorInd = 1;
        errno = rc;
        return;
    }
#endif
#ifdef __MSDOS__
    tempWritten = __write(stream->hfile,
                          stream->fbuf,
                          stream->szfbuf,
                          &errind);
    if (errind)
    {
        stream->errorInd = 1;
        errno = tempWritten;
        return;
    }
#endif
    *actualWritten = spare;
    stream->upto = stream->fbuf;
    stream->bufStartR += tempWritten;
    if (towrite > stream->szfbuf)
    {
        stream->quickBin = 1;
#ifdef __OS2__
        rc = DosWrite(stream->hfile,
                      (char *)ptr + *actualWritten,
                      towrite - *actualWritten,
                      &tempWritten);
        if (rc != 0)
        {
            stream->errorInd = 1;
            errno = rc;
            return;
        }
#endif
#ifdef __MSDOS__
        tempWritten = __write(stream->hfile,
                              (char *)ptr + *actualWritten,
                              towrite - *actualWritten,
                              &errind);
        if (errind)
        {
            stream->errorInd = 1;
            errno = tempWritten;
            return;
        }
#endif
        *actualWritten += tempWritten;
        stream->bufStartR += tempWritten;
    }
    else
    {
        memcpy(stream->fbuf,
               (char *)ptr + *actualWritten,
               towrite - *actualWritten);
        stream->upto += (towrite - *actualWritten);
        *actualWritten = towrite;
    }
    stream->bufStartR += *actualWritten;
    return;
}
#endif

static int vvprintf(const char *format, va_list arg, FILE *fq, char *s)
{
    int fin = 0;
    int vint;
    double vdbl;
    unsigned int uvint;
    char *vcptr;
    int chcount = 0;
    size_t len;
    char numbuf[50];
    char *nptr;

    while (!fin)
    {
        if (*format == '\0')
        {
            fin = 1;
        }
        else if (*format == '%')
        {
            format++;
            if (*format == 'd')
            {
                vint = va_arg(arg, int);
                if (vint < 0)
                {
                    uvint = -vint;
                }
                else
                {
                    uvint = vint;
                }
                nptr = numbuf;
                do
                {
                    *nptr++ = (char)('0' + uvint % 10);
                    uvint /= 10;
                } while (uvint > 0);
                if (vint < 0)
                {
                    *nptr++ = '-';
                }
                do
                {
                    nptr--;
                    outch(*nptr);
                    chcount++;
                } while (nptr != numbuf);
            }
            else if (*format == 'e' || *format == 'E' )
            {
                vdbl = va_arg(arg, double);
                dblcvt(vdbl , *format  , 0 ,  6 , numbuf);
                fputs(numbuf , fq);
            }
            else if (*format == 'g' || *format == 'G' )
            {
                vdbl = va_arg(arg, double);
                dblcvt(vdbl , *format  , 0 ,  6 , numbuf);
                fputs(numbuf , fq);
            }
            else if (*format == 'f' || *format == 'F' )
            {
                vdbl = va_arg(arg, double);
                dblcvt(vdbl , *format  , 0 ,  6 , numbuf);
                fputs(numbuf , fq);
            }
            else if (*format == 's')
            {
                vcptr = va_arg(arg, char *);
                if (vcptr == NULL)
                {
                    vcptr = "(null)";
                }
                if (fq == NULL)
                {
                    len = strlen(vcptr);
                    memcpy(s, vcptr, len);
                    s += len;
                    chcount += len;
                }
                else
                {
                    fputs(vcptr, fq);
                    chcount += strlen(vcptr);
                }
            }
            else if (*format == 'c')
            {
                vint = va_arg(arg, int);
                outch(vint);
                chcount++;
            }
            else if (*format == '%')
            {
                outch('%');
                chcount++;
            }
            else
            {
                int extraCh;

                extraCh = examine(&format, fq, s, &arg, chcount);
                chcount += extraCh;
                if (s != NULL)
                {
                    s += extraCh;
                }
            }
        }
        else
        {
            outch(*format);
            chcount++;
        }
        format++;
    }
    return (chcount);
}

static int examine(const char **formt, FILE *fq, char *s, va_list *arg,
                   int chcount)
{
    int extraCh = 0;
    int flagMinus = 0;
    int flagPlus = 0;
    int flagSpace = 0;
    int flagHash = 0;
    int flagZero = 0;
    int width = 0;
    int precision = -1;
    int half = 0;
    int lng = 0;
    int specifier = 0;
    int fin;
    long lvalue;
    unsigned long ulvalue;
    double vdbl;
    char *svalue;
    char work[50];
    int x;
    int y;
    int rem;
    const char *format;
    int base;
    int fillCh;
    int neg;
    int length;

    unused(chcount);
    format = *formt;
    /* processing flags */
    fin = 0;
    while (!fin)
    {
        switch (*format)
        {
            case '-': flagMinus = 1;
                      break;
            case '+': flagPlus = 1;
                      break;
            case ' ': flagSpace = 1;
                      break;
            case '#': flagHash = 1;
                      break;
            case '0': flagZero = 1;
                      break;
            default:  fin = 1;
                      break;
        }
        if (!fin)
        {
            format++;
        }
        else
        {
            if (flagSpace && flagPlus)
            {
                flagSpace = 0;
            }
            if (flagMinus)
            {
                flagZero = 0;
            }
        }
    }

    /* processing width */
    if (isdigit((unsigned char)*format))
    {
        while (isdigit((unsigned char)*format))
        {
            width = width * 10 + (*format - '0');
            format++;
        }
    }

    /* processing precision */
    if (*format == '.')
    {
        format++;
        precision = 0;
        while (isdigit((unsigned char)*format))
        {
            precision = precision * 10 + (*format - '0');
            format++;
        }
    }

    /* processing h/l/L */
    if (*format == 'h')
    {
        half = 1;
    }
    else if (*format == 'l')
    {
        lng = 1;
    }
    else if (*format == 'L')
    {
        lng = 1;
    }
    else
    {
        format--;
    }
    format++;

    if (precision < 0)
    {
        precision = 1;
    }
    /* processing specifier */
    specifier = *format;

    if (strchr("dxXuiop", specifier) != NULL)
    {
#if defined(__MSDOS__) && !defined(__PDOS__)
        if (specifier == 'p')
        {
            lng = 1;
        }
#endif
        if (lng)
        {
            lvalue = va_arg(*arg, long);
        }
        else if (half)
        {
            lvalue = va_arg(*arg, short);
        }
        else
        {
            lvalue = va_arg(*arg, int);
        }
        ulvalue = (unsigned long)lvalue;
        if ((lvalue < 0) && ((specifier == 'd') || (specifier == 'i')))
        {
            neg = 1;
            ulvalue = -lvalue;
        }
        else
        {
            neg = 0;
        }
#if defined(__MSDOS__)
        if (!lng)
        {
            ulvalue &= 0xffff;
        }
#endif
        if ((specifier == 'X') || (specifier == 'x') || (specifier == 'p'))
        {
            base = 16;
        }
        else if (specifier == 'o')
        {
            base = 8;
        }
        else
        {
            base = 10;
        }
        if (specifier == 'p')
        {
#if defined(__OS2__) || defined(__PDOS__)
            precision = 8;
#endif
#if defined(__MSDOS__) && !defined(__PDOS__)
            precision = 9;
#endif
        }
        x = 0;
        while (ulvalue > 0)
        {
#if defined(__MVS__) || defined(__CMS__)
            /* this is a hack that should be removed. It is to get
               around a bug in the gcc 3.2.3 MVS stage 101 optimizer. */
            rem = (int)(ulvalue - (ulvalue / base) * base);
#else
            rem = (int)(ulvalue % base);
#endif
            if (rem < 10)
            {
                work[x] = (char)('0' + rem);
            }
            else
            {
                if ((specifier == 'X') || (specifier == 'p'))
                {
                    work[x] = (char)('A' + (rem - 10));
                }
                else
                {
                    work[x] = (char)('a' + (rem - 10));
                }
            }
            x++;
#if defined(__MSDOS__) && !defined(__PDOS__)
            if ((x == 4) && (specifier == 'p'))
            {
                work[x] = ':';
                x++;
            }
#endif
            ulvalue = ulvalue / base;
        }
#if defined(__MSDOS__) && !defined(__PDOS__)
        if (specifier == 'p')
        {
            while (x < 5)
            {
                work[x] = (x == 4) ? ':' : '0';
                x++;
            }
        }
#endif
        while (x < precision)
        {
            work[x] = '0';
            x++;
        }
        if (neg)
        {
            work[x++] = '-';
        }
        if (flagZero)
        {
            fillCh = '0';
        }
        else
        {
            fillCh = ' ';
        }
        y = x;
        if (!flagMinus)
        {
            while (y < width)
            {
                outch(fillCh);
                extraCh++;
                y++;
            }
        }
        if (flagHash && (toupper((unsigned char)specifier) == 'X'))
        {
            outch('0');
            outch('x');
            extraCh += 2;
        }
        x--;
        while (x >= 0)
        {
            outch(work[x]);
            extraCh++;
            x--;
        }
        if (flagMinus)
        {
            while (y < width)
            {
                outch(fillCh);
                extraCh++;
                y++;
            }
        }
    }
    else if (specifier == 'e' || specifier == 'E' )
    {
        vdbl = va_arg(*arg, double);
        dblcvt(vdbl , specifier  , width,  precision , work );
        fputs(work , fq);
    }
    else if (specifier == 'g' || specifier == 'G' )
    {
        vdbl = va_arg(*arg, double);
        dblcvt(vdbl , specifier  , width ,  precision , work );
        fputs(work , fq);
    }
    else if (specifier == 'f' || specifier == 'F' )
    {
        vdbl = va_arg(*arg, double);
        dblcvt(vdbl , specifier  , width ,  precision , work );
        fputs(work , fq);
    }
    else if (specifier == 's')
    {
        svalue = va_arg(*arg, char *);
        fillCh = ' ';
        if (precision > 1)
        {
            char *p;

            p = memchr(svalue, '\0', precision);
            if (p != NULL)
            {
                length = (int)(p - svalue);
            }
            else
            {
                length = precision;
            }
        }
        else
        {
            length = strlen(svalue);
        }
        if (!flagMinus)
        {
            if (length < width)
            {
                extraCh += (width - length);
                for (x = 0; x < (width - length); x++)
                {
                    outch(fillCh);
                }
            }
        }
        for (x = 0; x < length; x++)
        {
            outch(svalue[x]);
        }
        extraCh += length;
        if (flagMinus)
        {
            if (length < width)
            {
                extraCh += (width - length);
                for (x = 0; x < (width - length); x++)
                {
                    outch(fillCh);
                }
            }
        }
    }
    *formt = format;
    return (extraCh);
}

int fputc(int c, FILE *stream)
{
    char buf[1];

#if !defined(__MVS__) && !defined(__CMS__)
    stream->quickBin = 0;
    if ((stream->upto < (stream->endbuf - 2))
        && (stream->bufTech != _IONBF))
    {
        if (stream->textMode)
        {
            if (c == '\n')
            {
                if (stream->bufTech == _IOFBF)
                {
                    *stream->upto++ = '\r';
                    *stream->upto++ = '\n';
                }
                else
                {
                    buf[0] = (char)c;
                    if (fwrite(buf, 1, 1, stream) != 1)
                    {
                        return (EOF);
                    }
                }
            }
            else
            {
                *stream->upto++ = (char)c;
            }
        }
        else
        {
            *stream->upto++ = (char)c;
        }
    }
    else
#endif
    {
        buf[0] = (char)c;
        if (fwrite(buf, 1, 1, stream) != 1)
        {
            return (EOF);
        }
    }
    return (c);
}

#if !defined(__MVS__) && !defined(__CMS__)
int fputs(const char *s, FILE *stream)
{
    size_t len;
    size_t ret;

    len = strlen(s);
    ret = fwrite(s, len, 1, stream);
    if (ret != 1) return (EOF);
    else return (0);
}
#endif

int remove(const char *filename)
{
    int ret;
#ifdef __OS2__
    APIRET rc;
#endif

#ifdef __OS2__
    rc = DosDelete((PSZ)filename);
    if (rc != 0)
    {
        ret = 1;
        errno = rc;
    }
    else
    {
        ret = 0;
    }
#endif
#ifdef __MSDOS__
    __remove(filename);
    ret = 0;
#endif
    return (ret);
}

int rename(const char *old, const char *new)
{
    int ret;
#ifdef __OS2__
    APIRET rc;
#endif

#ifdef __OS2__
    rc = DosMove((PSZ)old, (PSZ)new);
    if (rc != 0)
    {
        ret = 1;
        errno = rc;
    }
    else
    {
        ret = 0;
    }
#endif
#ifdef __MSDOS__
    __rename(old, new);
    ret = 0;
#endif
    return (ret);
}

int sprintf(char *s, const char *format, ...)
{
    va_list arg;
    int ret;

    va_start(arg, format);
    ret = vsprintf(s, format, arg);
    va_end(arg);
    return (ret);
}

int vsprintf(char *s, const char *format, va_list arg)
{
    int ret;

    ret = vvprintf(format, arg, NULL, s);
    if (ret >= 0)
    {
        *(s + ret) = '\0';
    }
    return (ret);
}

/*

In fgets, we have the following possibilites...

1. we found a genuine '\n' that terminated the search.
2. we hit the '\n' at the endbuf.
3. we hit the '\n' sentinel.

*/
#if !defined(__MVS__) && !defined(__CMS__)
char *fgets(char *s, int n, FILE *stream)
{
    char *p;
    register char *t;
    register char *u = s;
    int c;
    int processed;
#ifdef __OS2__
    ULONG actualRead;
    APIRET rc;
#endif
#ifdef __MSDOS__
    size_t actualRead;
    int errind;
#endif

    if (stream->quickText)
    {
        p = stream->upto + n - 1;
        t = stream->upto;
        if (p < stream->endbuf)
        {
            c = *p;
            *p = '\n';
#ifdef __OS2__
            if (n < 8)
            {
#endif
                while ((*u++ = *t++) != '\n') ; /* tight inner loop */
#ifdef __OS2__
            }
            else
            {
                register unsigned int *i1;
                register unsigned int *i2;
                register unsigned int z;

                i1 = (unsigned int *)t;
                i2 = (unsigned int *)u;
                while (1)
                {
                    z = *i1;
                    if ((z & 0xffU) == '\n') break;
                    z >>= 8;
                    if ((z & 0xffU) == '\n') break;
                    z >>= 8;
                    if ((z & 0xffU) == '\n') break;
                    z >>= 8;
                    if ((z & 0xffU) == '\n') break;
                    *i2++ = *i1++;
                }
                t = (char *)i1;
                u = (char *)i2;
                while ((*u++ = *t++) != '\n') ;
            }
#endif
            *p = (char)c;
            if (t <= p)
            {
                if (*(t - 2) == '\r') /* t is protected, u isn't */
                {
                    *(u - 2) = '\n';
                    *(u - 1) = '\0';
                }
                else
                {
                    *u = '\0';
                }
                stream->upto = t;
                return (s);
            }
            else
            {
                processed = (int)(t - stream->upto) - 1;
                stream->upto = t - 1;
                u--;
            }
        }
        else
        {
            while ((*u++ = *t++) != '\n') ; /* tight inner loop */
            if (t <= stream->endbuf)
            {
                if (*(t - 2) == '\r') /* t is protected, u isn't */
                {
                    *(u - 2) = '\n';
                    *(u - 1) = '\0';
                }
                else
                {
                    *u = '\0';
                }
                stream->upto = t;
                return (s);
            }
            else
            {
                processed = (int)(t - stream->upto) - 1;
                stream->upto = t - 1;
                u--;
            }
        }
    }
    else
    {
        processed = 0;
    }

    if (n < 1)
    {
        return (NULL);
    }
    if (n < 2)
    {
        *u = '\0';
        return (s);
    }
    if (stream->ungetCh != -1)
    {
        processed++;
        *u++ = (char)stream->ungetCh;
        stream->ungetCh = -1;
    }
    while (1)
    {
        t = stream->upto;
        p = stream->upto + (n - processed) - 1;
        if (p < stream->endbuf)
        {
            c = *p;
            *p = '\n';
        }
        if (stream->noNl)
        {
            while (((*u++ = *t) != '\n') && (*t++ != '\r')) ;
            if (*(u - 1) == '\n')
            {
                t++;
            }
            else
            {
                u--;
                while ((*u++ = *t++) != '\n') ;
            }
        }
        else
        {
            while ((*u++ = *t++) != '\n') ; /* tight inner loop */
        }
        if (p < stream->endbuf)
        {
            *p = (char)c;
        }
        if (((t <= p) && (p < stream->endbuf))
           || ((t <= stream->endbuf) && (p >= stream->endbuf)))
        {
            if (stream->textMode)
            {
                if (stream->noNl)
                {
                    if ((*(t - 1) == '\r') || (*(t - 1) == '\n'))
                    {
                        *(u - 1) = '\0';
                    }
                    else
                    {
                        *u = '\0';
                    }
                }
                else if (*(t - 2) == '\r') /* t is protected, u isn't */
                {
                    *(u - 2) = '\n';
                    *(u - 1) = '\0';
                }
                else
                {
                    *u = '\0';
                }
            }
            stream->upto = t;
            if (stream->textMode)
            {
                stream->quickText = 1;
            }
            return (s);
        }
        else if (((t > p) && (p < stream->endbuf))
                 || ((t > stream->endbuf) && (p >= stream->endbuf)))
        {
            int leave = 1;

            if (stream->textMode)
            {
                if (t > stream->endbuf)
                {
                    if ((t - stream->upto) > 1)
                    {
                        if (*(t - 2) == '\r') /* t is protected, u isn't */
                        {
                            processed -= 1; /* preparation for add */
                        }
                    }
                    leave = 0;
                }
                else
                {
                    if ((*(t - 2) == '\r') && (*(t - 1) == '\n'))
                    {
                        *(u - 2) = '\n';
                        *(u - 1) = '\0';
                    }
                    else
                    {
                        t--;
                        *(u - 1) = '\0';
                    }
                }
            }
            else if (t > stream->endbuf)
            {
                leave = 0;
            }
            else
            {
                *u = '\0';
            }
            if (leave)
            {
                stream->upto = t;
                if (stream->textMode)
                {
                    stream->quickText = 1;
                }
                return (s);
            }
        }
        processed += (int)(t - stream->upto) - 1;
        u--;
        stream->bufStartR += (stream->endbuf - stream->fbuf);
#ifdef __OS2__
        rc = DosRead(stream->hfile, stream->fbuf, stream->szfbuf, &actualRead);
        if (rc != 0)
        {
            actualRead = 0;
            stream->errorInd = 1;
            errno = rc;
        }
#endif
#ifdef __MSDOS__
        actualRead = __read(stream->hfile,
                            stream->fbuf,
                            stream->szfbuf,
                            &errind);
        if (errind)
        {
            errno = actualRead;
            actualRead = 0;
            stream->errorInd = 1;
        }
#endif
        stream->endbuf = stream->fbuf + actualRead;
        *stream->endbuf = '\n';
        if (actualRead == 0)
        {
            *u = '\0';
            if ((u - s) <= 1)
            {
                stream->eofInd = 1;
                return (NULL);
            }
            else
            {
                return (s);
            }
        }
        stream->upto = stream->fbuf;
    }
}
#endif

int ungetc(int c, FILE *stream)
{
    if ((stream->ungetCh != -1) || (c == EOF))
    {
        return (EOF);
    }
    stream->ungetCh = (unsigned char)c;
    stream->quickText = 0;
    stream->quickBin = 0;
    return ((unsigned char)c);
}

int fgetc(FILE *stream)
{
    unsigned char x[1];
    size_t ret;

    ret = fread(x, 1, 1, stream);
    if (ret == 0)
    {
        return (EOF);
    }
    return ((int)x[0]);
}

int fseek(FILE *stream, long int offset, int whence)
{
    long newpos;
#ifdef __OS2__
    ULONG retpos;
    APIRET rc;
#endif

    if (stream->mode == __WRITE_MODE)
    {
        fflush(stream);
    }
    if (whence == SEEK_SET)
    {
        newpos = offset;
    }
    else if (whence == SEEK_CUR)
    {
        newpos = offset + stream->bufStartR + (stream->upto - stream->fbuf);
    }
    if ((newpos > stream->bufStartR)
        && (newpos < (stream->bufStartR + (stream->endbuf - stream->fbuf)))
        && stream->update)
    {
        stream->upto = stream->fbuf + (size_t)(newpos - stream->bufStartR);
    }
    else
    {
#ifdef __OS2__
        rc = DosSetFilePtr(stream->hfile, newpos, FILE_BEGIN, &retpos);
        if ((rc != 0) || (retpos != newpos))
        {
            errno = rc;
            return (-1);
        }
        else
        {
            stream->endbuf = stream->fbuf + stream->szfbuf;
            stream->upto = stream->endbuf;
            stream->bufStartR = newpos - stream->szfbuf;
        }
#endif
#ifdef __MSDOS__
        __seek(stream->hfile, newpos, whence);
        stream->endbuf = stream->fbuf + stream->szfbuf;
        stream->upto = stream->endbuf;
        stream->bufStartR = newpos - stream->szfbuf;
#endif
#if defined(__MVS__) || defined(__CMS__)
        char fnm[FILENAME_MAX];
        
        strcpy(fnm, "dd:");
        strcat(fnm, stream->ddname);
        freopen(fnm, stream->modeStr, stream);
        while (newpos-- > 0)
        {
            getc(stream);
        }
#endif
    }
    stream->quickBin = 0;
    stream->quickText = 0;
    stream->ungetCh = -1;
    return (0);
}

long int ftell(FILE *stream)
{
    return (stream->bufStartR + (stream->upto - stream->fbuf));
}

int fsetpos(FILE *stream, const fpos_t *pos)
{
    fseek(stream, *pos, SEEK_SET);
    return (0);
}

int fgetpos(FILE *stream, fpos_t *pos)
{
    *pos = ftell(stream);
    return (0);
}

void rewind(FILE *stream)
{
    fseek(stream, 0L, SEEK_SET);
    return;
}

void clearerr(FILE *stream)
{
    stream->errorInd = 0;
    stream->eofInd = 0;
    return;
}

void perror(const char *s)
{
    if ((s != NULL) && (*s != '\0'))
    {
        printf("%s: ", s);
    }
    if (errno == 0)
    {
        printf("No error has occurred\n");
    }
    else
    {
        printf("An error has occurred\n");
    }
    return;
}

/*
NULL + F = allocate, setup
NULL + L = allocate, setup
NULL + N = ignore, return success
buf  + F = setup
buf  + L = setup
buf  + N = ignore, return success
*/

int setvbuf(FILE *stream, char *buf, int mode, size_t size)
{
    char *mybuf;

#if defined(__MVS__) || defined(__CMS__)
    /* don't allow mucking around with buffers on MVS or CMS */
    return (0);
#endif

    if (mode == _IONBF)
    {
        stream->bufTech = mode;
        return (0);
    }
    if (buf == NULL)
    {
        if (size < 2)
        {
            return (-1);
        }
        mybuf = malloc(size + 8);
        if (mybuf == NULL)
        {
            return (-1);
        }
    }
    else
    {
        if (size < 10)
        {
            return (-1);
        }
        mybuf = buf;
        stream->theirBuffer = 1;
        size -= 8;
    }
    free(stream->intBuffer);
    stream->intBuffer = mybuf;
    stream->fbuf = stream->intBuffer + 2;
    *stream->fbuf++ = '\0';
    *stream->fbuf++ = '\0';
    stream->szfbuf = size;
    stream->endbuf = stream->fbuf + stream->szfbuf;
    *stream->endbuf = '\n';
    stream->upto = stream->endbuf;
    stream->bufTech = mode;
    if (!stream->textMode && (stream->bufTech == _IOLBF))
    {
        stream->quickBin = 0;
    }
    return (0);
}

int setbuf(FILE *stream, char *buf)
{
    int ret;

    if (buf == NULL)
    {
        ret = setvbuf(stream, NULL, _IONBF, 0);
    }
    else
    {
        ret = setvbuf(stream, buf, _IOFBF, BUFSIZ);
    }
    return (ret);
}

FILE *freopen(const char *filename, const char *mode, FILE *stream)
{
    int perm;

    perm = stream->permfile;
    stream->permfile = 1;
    fclose(stream);
    
    myfile = stream;
    fnm = filename;
    modus = mode;
    err = 0;
    spareSpot = stream->intFno;
    fopen2();
    if (err && !perm)
    {
        __userFiles[stream->intFno] = NULL;
        free(stream);
    }
    stream->permfile = perm;
    if (err)
    {
        return (NULL);
    }
    return (stream);
}

int fflush(FILE *stream)
{
#if !defined(__MVS__) && !defined(__CMS__)
#ifdef __OS2__
    APIRET rc;
    ULONG actualWritten;
#endif
#ifdef __MSDOS__
    int errind;
    size_t actualWritten;
#endif

    if ((stream->upto != stream->fbuf) && (stream->mode == __WRITE_MODE))
    {
#ifdef __OS2__
        rc = DosWrite(stream->hfile,
                     (VOID *)stream->fbuf,
                     (size_t)(stream->upto - stream->fbuf),
                     &actualWritten);
        if (rc != 0)
        {
            stream->errorInd = 1;
            errno = rc;
            return (EOF);
        }
#endif
#ifdef __MSDOS__
        actualWritten = __write(stream->hfile,
                                stream->fbuf,
                                (size_t)(stream->upto - stream->fbuf),
                                &errind);
        if (errind)
        {
            stream->errorInd = 1;
            errno = actualWritten;
            return (EOF);
        }
#endif
        stream->bufStartR += actualWritten;
        stream->upto = stream->fbuf;
    }
#endif
    return (0);
}

char *tmpnam(char *s)
{
    static char buf[] = "ZZZZZZZA.$$$";

    buf[7]++;
    if (s == NULL)
    {
        return (buf);
    }
    strcpy(s, buf);
    return (s);
}

FILE *tmpfile(void)
{
    return (fopen("ZZZZZZZA.$$$", "wb+"));
}

int fscanf(FILE *stream, const char *format, ...)
{
    va_list arg;
    int ret;

    va_start(arg, format);
    ret = vvscanf(format, arg, stream, NULL);
    va_end(arg);
    return (ret);
}

int scanf(const char *format, ...)
{
    va_list arg;
    int ret;

    va_start(arg, format);
    ret = vvscanf(format, arg, stdin, NULL);
    va_end(arg);
    return (ret);
}

int sscanf(const char *s, const char *format, ...)
{
    va_list arg;
    int ret;

    va_start(arg, format);
    ret = vvscanf(format, arg, NULL, s);
    va_end(arg);
    return (ret);
}

static int vvscanf(const char *format, va_list arg, FILE *fp, const char *s)
{
    int ch;
    int fin = 0;
    int cnt = 0;
    char *cptr;
    int *iptr;

    inch();
    while (!fin)
    {
        if (*format == '\0')
        {
            fin = 1;
        }
        else if (*format == '%')
        {
            format++;
            if (*format == '%')
            {
                if (ch != '%') return (cnt);
                inch();
            }
            else if (*format == 's')
            {
                cptr = va_arg(arg, char *);
                *cptr++ = (char)ch;
                inch();
                while ((ch >= 0) && (!isspace(ch)))
                {
                    *cptr++ = (char)ch;
                    inch();
                }
                *cptr = '\0';
                if (ch < 0)
                {
                    fin = 1;
                }
            }
            else if (*format == 'd')
            {
                iptr = va_arg(arg, int *);
                if (!isdigit(ch)) return (cnt);
                *iptr = ch - '0';
                inch();
                while ((ch >= 0) && (isdigit(ch)))
                {
                    *iptr = *iptr * 10 + (ch - '0');
                    inch();
                }
                if (ch < 0)
                {
                    fin = 1;
                }
            }
        }
        else
        {
            if (ch != *format) return (cnt);
            inch();
        }
    }
    return (cnt);
}

char *gets(char *s)
{
    char *ret;

    stdin->quickText = 0;
    stdin->noNl = 1;
    ret = fgets(s, INT_MAX, stdin);
    stdin->noNl = 0;
    stdin->quickText = 1;
    return (ret);
}

int puts(const char *s)
{
    int ret;

    ret = fputs(s, stdout);
    if (ret == EOF)
    {
        return (ret);
    }
    return (putc('\n', stdout));
}

/* The following functions are implemented as macros */

#undef getchar
#undef putchar
#undef getc
#undef putc
#undef feof
#undef ferror

int getc(FILE *stream)
{
    return (fgetc(stream));
}

int putc(int c, FILE *stream)
{
    return (fputc(c, stream));
}

int getchar(void)
{
    return (getc(stdin));
}

int putchar(int c)
{
    return (putc(c, stdout));
}

int feof(FILE *stream)
{
    return (stream->eofInd);
}

int ferror(FILE *stream)
{
    return (stream->errorInd);
}

#if 0
Design of MVS i/o routines

in/out function rec-type mode   method
in     fread    fixed    bin    loop reading, remember remainder
in     fread    fixed    text   loop reading + truncing, remember rem
in     fread    var      bin    loop reading (+ len), remember remainder
in     fread    var      text   loop reading (+ len), remember remainder
in     fgets    fixed    bin    read, scan, remember remainder
in     fgets    fixed    text   read, trunc, remember remainder
in     fgets    var      bin    read, scan, rr
in     fgets    var      text   read, rr
in     fgetc    fixed    bin    read, rr
in     fgetc    fixed    text   read, trunc, rr
in     fgetc    var      bin    read, rr
in     fgetc    var      text   read, rr

out    fwrite   fixed    bin    loop doing put, rr
out    fwrite   fixed    text   search newline, copy + pad, put, rr
out    fwrite   var      bin    if nelem != 1 copy to max lrecl
out    fwrite   var      text   loop search nl, put, rr
out    fputs    fixed    bin    loop doing put, rr
out    fputs    fixed    text   search newline, copy + pad, put, rr
out    fputs    var      bin    put
out    fputs    var      text   search newline, put, copy rem
out    fputc    fixed    bin    copy to rr until rr == lrecl
out    fputc    fixed    text   copy to rr until newline, then pad
out    fputc    var      bin    copy to rr until rr == lrecl
out    fputc    var      text   copy to rr until newline

optimize for fread on binary files (read matching record length),
especially fixed block files, and fgets on text files, especially
variable blocked files.

binary, variable block files are not a file type supported by this
library as part of the conforming implementation.  Instead, they
are considered to be record-oriented processing, similar to unix
systems reading data from a pipe, where you can read less bytes
than requested, without reaching EOF.  ISO 7.9.8.1 does not give you
the flexibility of calling either of these things conforming.
Basically, the C standard does not have a concept of operating
system maintained length binary records, you have to do that
yourself, e.g. by writing out the lengths yourself.  You can do
this in a fixed block dataset on MVS, and if you are concerned
about null-padding at the end of your data, use a lrecl of 1
(and suffer the consequences!).  You could argue that this
non-conformance should only be initiated if fopen has a parameter
including ",type=record" or whatever.  Another option would
be to make VB binary records include the record size as part of
the stream.  Hmmm, sounds like that is the go actually.

fread: if quickbin, if read elem size == lrecl, doit
fgets: if variable record + no remainder
       if buffer > record size, copy + add newline
#endif

#if defined(__MVS__) || defined(__CMS__)
char *fgets(char *s, int n, FILE *stream)
{
    unsigned char *dptr;
    unsigned char *eptr;
    size_t len;
    int cnt;
    int c;
    if (stream->quickText)
    {
        if (__aread(stream->hfile, &dptr) != 0)
        {
            stream->eofInd = 1;
            stream->quickText = 0;
            return (NULL);
        }
        len = ((dptr[0] << 8) | dptr[1]) - 4;
        if (n > (len + 1))
        {
            memcpy(s, dptr + 4, len);
            memcpy(s + len, "\n", 2);
            stream->bufStartR += len + 1;
            return (s);
        }
        else
        {
            memcpy(stream->fbuf, dptr + 4, len);
            stream->upto = stream->fbuf;
            stream->endbuf = stream->fbuf + len;
            *(stream->endbuf++) = '\n';
            stream->quickText = 0;
        }
    }
    
    if (stream->eofInd)
    {
        return (NULL);
    }

    switch (stream->style)
    {
        case FIXED_TEXT:
            if ((stream->endbuf == stream->fbuf)
                && (n > (stream->lrecl + 2)))
            {
                if (__aread(stream->hfile, &dptr) != 0)
                {
                    stream->eofInd = 1;
                    return (NULL);
                }
                eptr = dptr + stream->lrecl - 1;
                while ((*eptr == ' ') && (eptr >= dptr))
                {
                    eptr--;
                }
                eptr++;
                memcpy(s, dptr, eptr - dptr);
                memcpy(s + (eptr - dptr), "\n", 2);
                stream->bufStartR += (eptr - dptr) + 1;
                return (s);
            }
            break;

        default:
            break;

    }
    
    /* Ok, the obvious optimizations have been done,
       so now we switch to the slow generic version */
    
    n--;
    cnt = 0;
    while (cnt < n)
    {
        c = getc(stream);
        if (c == EOF) break;
        s[cnt] = c;
        if (c == '\n') break;
        cnt++;
    }
    if (cnt < n) s[cnt++] = '\n';
    s[cnt] = '\0';
    return (s);
}

int fputs(const char *s, FILE *stream)
{
    const char *p;
    size_t len;
    char *dptr;

    if (stream->quickText)
    {
        p = strchr(s, '\n');
        if (p != NULL)
        {
            len = p - s;
            if (len > stream->lrecl)
            {
                len = stream->lrecl;
            }
            __awrite(stream->hfile, &dptr);
            memcpy(dptr + 4, s, len);
            dptr[0] = (len + 4) >> 8;
            dptr[1] = (len + 4) & 0xff;
            dptr[2] = 0;
            dptr[3] = 0;
            stream->bufStartR += (len + 1);
            if (*(p + 1) == '\0')
            {
                return (len + 1);
            }
            s = p + 1;
        }
    }
    switch (stream->style)
    {
        case FIXED_TEXT:
            len = strlen(s);
            if (len > 0)
            {
                len--;
                if (((strchr(s, '\n') - s) == len)
                    && (stream->upto == stream->fbuf)
                    && (len <= stream->lrecl))
                {
                    __awrite(stream->hfile, &dptr);
                    memcpy(dptr, s, len);
                    memset(dptr + len, ' ', stream->szfbuf - len);
                    stream->bufStartR += len;
                }
                else
                {
                    fwrite(s, len + 1, 1, stream);
                }
            }
            break;

        default:
            len = strlen(s);
            fwrite(s, len, 1, stream);
    }
    return (0);
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t bytes;
    size_t sz;
    char *p;
    int x;
    char *dptr;

    if (stream->quickBin)
    {
        if ((nmemb == 1) && (size == stream->lrecl))
        {
            __awrite(stream->hfile, &dptr);
            memcpy(dptr, ptr, size);
            stream->bufStartR += size;
            return (1);
        }
        else
        {
            stream->quickBin = 0;
        }
    }
    switch (stream->style)
    {
        case FIXED_BINARY:
            bytes = nmemb * size;
            if ((stream->endbuf - stream->upto) > bytes)
            {
                memcpy(stream->upto, ptr, bytes);
            }
            else
            {
                if (stream->upto != stream->fbuf)
                {
                    sz = stream->endbuf - stream->upto;
                    memcpy(stream->upto, ptr, sz);
                    ptr = (char *)ptr + sz;
                    bytes -= sz;
                    __awrite(stream->hfile, &dptr);
                    memcpy(dptr, stream->fbuf, sz);
                    stream->upto = stream->fbuf;
                    stream->bufStartR += sz;
                }
            }
            while (bytes > stream->szfbuf)
            {
                __awrite(stream->hfile, &dptr);
                memcpy(dptr, ptr, stream->szfbuf);
                ptr = (char *)ptr + stream->szfbuf;
                bytes -= stream->szfbuf;
                stream->bufStartR += stream->szfbuf;
            }
            memcpy(stream->upto, ptr, bytes);
            stream->upto += bytes;
            break;

        case VARIABLE_BINARY:
            for (x = 0; x < nmemb; x++)
            {
                memcpy(stream->fbuf + 4, ptr, size);
                stream->fbuf[0] = (size + 4) >> 8;
                stream->fbuf[1] = (size + 4) & 0xff;
                stream->fbuf[2] = 0;
                stream->fbuf[3] = 0;
                __awrite(stream->hfile, &dptr);
                memcpy(dptr, stream->fbuf, size + 4);
                ptr = (char *)ptr + size;
                stream->bufStartR += size;
            }
            break;

        case FIXED_TEXT:
            bytes = nmemb * size;
            p = memchr(ptr, '\n', bytes);
            if (p != NULL)
            {
                sz = p - (char *)ptr;
                bytes -= sz + 1;
                if (stream->upto == stream->fbuf)
                {
                    if (sz > stream->lrecl)
                    {
                        sz = stream->lrecl;
                    }
                    __awrite(stream->hfile, &dptr);
                    memcpy(dptr, ptr, sz);
                    memset(dptr + sz, ' ', stream->szfbuf - sz);
                    stream->bufStartR += sz;
                }
                else
                {
                    if (((stream->upto - stream->fbuf) + sz) > stream->lrecl)
                    {
                        sz = stream->lrecl - (stream->upto - stream->fbuf);
                    }
                    memcpy(stream->upto, ptr, sz);
                    sz += (stream->upto - stream->fbuf);
                    __awrite(stream->hfile, &dptr);
                    memcpy(dptr, stream->fbuf, sz);
                    memset(dptr + sz, ' ', stream->lrecl - sz);
                    stream->bufStartR += sz;
                    stream->upto = stream->fbuf;
                }
                ptr = (char *)p + 1;
                if (bytes > 0)
                {
                    p = memchr(ptr, '\n', bytes);
                    while (p != NULL)
                    {
                        sz = p - (char *)ptr;
                        bytes -= sz + 1;
                        if (sz > stream->lrecl)
                        {
                            sz = stream->lrecl;
                        }
                        __awrite(stream->hfile, &dptr);
                        memcpy(dptr, ptr, sz);
                        memset(dptr + sz, ' ', stream->szfbuf - sz);
                        stream->bufStartR += sz;
                        ptr = p + 1;
                        p = memchr(ptr, '\n', bytes);
                    }
                    if (bytes > 0)
                    {
                        sz = bytes;
                        if (sz > stream->lrecl)
                        {
                            sz = stream->lrecl;
                        }
                        memcpy(stream->upto, ptr, sz);
                        stream->upto += sz;
                        bytes = 0;
                    }
                }
            }
            else /* p == NULL */
            {
                if (((stream->upto - stream->fbuf) + bytes) > stream->lrecl)
                {
                    bytes = stream->lrecl - (stream->upto - stream->fbuf);
                }
                memcpy(stream->upto, ptr, bytes);
                stream->upto += bytes;
            }
            break;

        case VARIABLE_TEXT:
            bytes = nmemb * size;
            p = memchr(ptr, '\n', bytes);
            if (p != NULL)
            {
                sz = p - (char *)ptr;
                bytes -= sz + 1;
                if (stream->upto == stream->fbuf)
                {
                    if (sz > stream->lrecl)
                    {
                        sz = stream->lrecl;
                    }
                    __awrite(stream->hfile, &dptr);
                    if(sz == 0)
                    {
                        dptr[0] = 0;
                        dptr[1] = 5;
                        dptr[2] = 0;
                        dptr[3] = 0;
                        dptr[4] = ' ';
                        stream->bufStartR += 2;
                    }
                    else 
                    {
                        dptr[0] = (sz + 4) >> 8;
                        dptr[1] = (sz + 4) & 0xff;
                        dptr[2] = 0;
                        dptr[3] = 0;
                        memcpy(dptr + 4, ptr, sz);
                        stream->bufStartR += (sz + 1);
                    }
                }
                else
                {
                    if (((stream->upto - stream->fbuf) + sz) > stream->lrecl)
                    {
                        sz = stream->lrecl - (stream->upto - stream->fbuf);
                    }
                    memcpy(stream->upto, p, sz);
                    sz += (stream->upto - stream->fbuf);
                    __awrite(stream->hfile, &dptr);
                    if(sz == 0)
                    {
                        dptr[0] = 0;
                        dptr[1] = 5;
                        dptr[2] = 0;
                        dptr[3] = 0;
                        dptr[4] = ' ';
                        stream->bufStartR += 2;
                    }
                    else
                    {  
                        dptr[0] = (sz + 4) >> 8;
                        dptr[1] = (sz + 4) & 0xff;
                        dptr[2] = 0;
                        dptr[3] = 0;
                        memcpy(dptr + 4, stream->fbuf, sz);
                        stream->bufStartR += (sz + 1);
                    }
                    stream->upto = stream->fbuf;
                }
                ptr = (char *)p + 1;
                if (bytes > 0)
                {
                    p = memchr(ptr, '\n', bytes);
                    while (p != NULL)
                    {
                        sz = p - (char *)ptr;
                        bytes -= sz + 1;
                        if (sz > stream->lrecl)
                        {
                            sz = stream->lrecl;
                        }
                        __awrite(stream->hfile, &dptr);
                        if(sz == 0)
                        {
                            dptr[0] = 0;
                            dptr[1] = 5;
                            dptr[2] = 0;
                            dptr[3] = 0;
                            dptr[4] = ' ';
                            stream->bufStartR += 2;
                        }
                        else 
                        {
                            dptr[0] = (sz + 4) >> 8;
                            dptr[1] = (sz + 4) & 0xff;
                            dptr[2] = 0;
                            dptr[3] = 0;
                            memcpy(dptr + 4, ptr, sz);
                            stream->bufStartR += (sz + 1);
                        }
                        ptr = p + 1;
                        p = memchr(ptr, '\n', bytes);
                    }
                    if (bytes > 0)
                    {
                        sz = bytes;
                        if (sz > stream->lrecl)
                        {
                            sz = stream->lrecl;
                        }
                        memcpy(stream->upto, ptr, sz);
                        stream->upto += sz;
                        bytes = 0;
                    }
                }
            }
            else /* p == NULL */
            {
                if (((stream->upto - stream->fbuf) + bytes) > stream->lrecl)
                {
                    bytes = stream->lrecl - (stream->upto - stream->fbuf);
                }
                memcpy(stream->upto, ptr, bytes);
                stream->upto += bytes;
            }
            break;
    }
    return (nmemb);
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t bytes;
    size_t read;
    size_t totalread;
    size_t extra;
    unsigned char *dptr;
    unsigned char *eptr;

    if (stream->quickBin)
    {
        if ((nmemb == 1) && (size == stream->lrecl))
        {
            if (__aread(stream->hfile, &dptr) != 0)
            {
                stream->eofInd = 1;
                stream->quickBin = 0;
                return (0);
            }
            memcpy(ptr, dptr, read);
            return (1);
        }
        else
        {
            stream->quickBin = 0;
        }
    }
    if (stream->eofInd)
    {
        return (0);
    }
    
    /* If we have an unget character, then write it into
       the buffer in advance */
    if (stream->ungetCh != -1)
    {
        stream->upto--;
        *stream->upto = stream->ungetCh;
        stream->ungetCh = -1;
    }

    switch (stream->style)
    {
        case FIXED_TEXT:
            bytes = nmemb * size;
            read = stream->endbuf - stream->upto;
            if (read > bytes)
            {
                memcpy(ptr, stream->upto, bytes);
                stream->upto += bytes;
                totalread = bytes;
            }
            else
            {
                memcpy(ptr, stream->upto, read);
                stream->upto = stream->endbuf = stream->fbuf;
                totalread = read;
            }

            while (totalread < bytes)
            {
                if (__aread(stream->hfile, &dptr) != 0)
                {
                    stream->eofInd = 1;
                    break;
                }
                
                eptr = dptr + stream->lrecl - 1;
                while ((*eptr == ' ') && (eptr >= dptr))
                {
                    eptr--;
                }
    
                read = eptr + 1 - dptr;
                
                if ((totalread + read) >= bytes)
                {
                    extra = (totalread + read) - bytes;
                    read -= extra;
                    memcpy(stream->fbuf, dptr + read, extra);
                    stream->endbuf = stream->fbuf + extra;
                    *stream->endbuf++ = '\n';
                }
                
                memcpy((char *)ptr + totalread, dptr, read);
                totalread += read;
                stream->bufStartR += read;
                if (totalread < bytes)
                {
                    *((char *)ptr + totalread) = '\n';
                    totalread++;
                    stream->bufStartR++;
                }
            }
            return (totalread / size);
            break;

        case FIXED_BINARY:
            bytes = nmemb * size;
            read = stream->endbuf - stream->upto;
            if (read > bytes)
            {
                memcpy(ptr, stream->upto, bytes);
                stream->upto += bytes;
                totalread = bytes;
            }
            else
            {
                memcpy(ptr, stream->upto, read);
                stream->upto = stream->endbuf = stream->fbuf;
                totalread = read;
            }

            while (totalread < bytes)
            {
                if (__aread(stream->hfile, &dptr) != 0)
                {
                    stream->eofInd = 1;
                    break;
                }
                
                read = stream->lrecl;
                
                if ((totalread + read) > bytes)
                {
                    extra = (totalread + read) - bytes;
                    read -= extra;
                    memcpy(stream->fbuf, dptr + read, extra);
                    stream->endbuf = stream->fbuf + extra;
                }
                
                memcpy((char *)ptr + totalread, dptr, read);
                totalread += read;
                stream->bufStartR += read;
            }
            return (totalread / size);
            break;

        case VARIABLE_TEXT:
            bytes = nmemb * size;
            read = stream->endbuf - stream->upto;
            if (read > bytes)
            {
                memcpy(ptr, stream->upto, bytes);
                stream->upto += bytes;
                totalread = bytes;
            }
            else
            {
                memcpy(ptr, stream->upto, read);
                stream->upto = stream->endbuf = stream->fbuf;
                totalread = read;
            }

            while (totalread < bytes)
            {
                if (__aread(stream->hfile, &dptr) != 0)
                {
                    stream->eofInd = 1;
                    break;
                }
                
                read = (dptr[0] << 8) | dptr[1];
                read -= 4;
                dptr += 4;
                
                if ((totalread + read) >= bytes)
                {
                    extra = (totalread + read) - bytes;
                    read -= extra;
                    memcpy(stream->fbuf, dptr + read, extra);
                    stream->endbuf = stream->fbuf + extra;
                    *stream->endbuf++ = '\n';
                }
                
                memcpy((char *)ptr + totalread, dptr, read);
                totalread += read;
                stream->bufStartR += read;
                if (totalread < bytes)
                {
                    *((char *)ptr + totalread) = '\n';
                    totalread++;
                    stream->bufStartR++;
                }
            }
            return (totalread / size);
            break;

        case VARIABLE_BINARY:
            bytes = nmemb * size;
            read = stream->endbuf - stream->upto;
            if (read > bytes)
            {
                memcpy(ptr, stream->upto, bytes);
                stream->upto += bytes;
                totalread = bytes;
            }
            else
            {
                memcpy(ptr, stream->upto, read);
                stream->upto = stream->endbuf = stream->fbuf;
                totalread = read;
            }

            while (totalread < bytes)
            {
                if (__aread(stream->hfile, &dptr) != 0)
                {
                    stream->eofInd = 1;
                    break;
                }
                
                read = (dptr[0] << 8) | dptr[1];
                
                if ((totalread + read) > bytes)
                {
                    extra = (totalread + read) - bytes;
                    read -= extra;
                    memcpy(stream->fbuf, dptr + read, extra);
                    stream->endbuf = stream->fbuf + extra;
                }
                
                memcpy((char *)ptr + totalread, dptr, read);
                totalread += read;
                stream->bufStartR += read;
            }
            return (totalread / size);
            break;

        default:
            break;
    }
    return (0);
}

#endif

/* 
   Following code issues a FILEDEF for CMS
*/

#ifdef __CMS__
static void filedef(char *fdddname, char *fnm, int mymode)
{
    char s202parm [800];

    int code;
    int parm;
    char *fname;
    char *ftype;
    char *fmode;
    char *p;

/*
    first parse the file name
*/
    while ( p = strchr(fnm, '.') ) *p=' '; /*  replace all . with blank */
    fname =  strtok(fnm, " ");
    ftype =  strtok(NULL, " ");
    if (ftype == NULL) ftype = "TYPE";
    fmode =  strtok(NULL, " ");  

/*
 Now build the SVC 202 string
*/
    memcpy ( &s202parm[0] , "FILEDEF ", 8);
    memcpy ( &s202parm[8] , fdddname, 8);
    memcpy ( &s202parm[16] , "DISK    ", 8);
/* 
  Clear PARMS area
*/
    memcpy ( &s202parm[24] , "        " , 8);
    memcpy ( &s202parm[32] , "        " , 8);
    if (mymode)
    {
        memcpy ( &s202parm[40] , "A1      " , 8);
    }
    else
    {
        memcpy ( &s202parm[40] , "*       " , 8);
    }

    memcpy ( &s202parm[24] , fname , 
             ( strlen(fname) > 8 ) ? 8 : strlen(fname)  );
    memcpy ( &s202parm[32] , ftype , 
             ( strlen(ftype) >8 ) ? 8 : strlen(ftype) );
/*  memcpy ( &s202parm[40] , fmode , strlen(fmode) ); */

   if ( mymode ) 
   {
        memcpy ( &s202parm[48] , "(       " , 8 );
        memcpy ( &s202parm[56] , "RECFM   " , 8 );
        memcpy ( &s202parm[64] , "V       " , 8 );
        memcpy ( &s202parm[72] , "LRECL   " , 8 );
        memcpy ( &s202parm[80] , "2000    " , 8 );
        s202parm[88]=s202parm[89]=s202parm[90]=s202parm[91]=
            s202parm[92]=s202parm[93]=s202parm[94]=s202parm[95]=0xff;
   }
   else
   {
        s202parm[48]=s202parm[49]=s202parm[50]=s202parm[51]=
            s202parm[52]=s202parm[53]=s202parm[54]=s202parm[55]=0xff;
   }

   __SVC202 ( s202parm, &code, &parm );
}
#endif


/*

 The truely cludged piece of code was concocted by Dave Wade

 His erstwhile tutors are probably turning in their graves.

 It is however placed in the Public Domain so that any one 
 who wishes to improve is free to do so

*/ 

static void dblcvt(double num, char cnvtype, size_t nwidth,
            size_t nprecision, char *result)
{
    double b;
    int i,exp,pdigits,format;
    char sign, work[45];

    /* save original data & set sign */

    if ( num < 0 )
    {
        b = -num;
        sign = '-';
    }
    else
    {
        b = num;
        sign = ' ';
    }

    /*
      Now scale to get exponent
    */

    exp = 0;
    if( b > 1.0 )
    {
        while(b >= 10.0)
        {
            ++exp;
            b=b / 10.0;
        }
    }
    else if ( b == 0.0 )
    {
        exp=0;
    }
    else if ( b < 1.0 )
    {
        while(b < 1.0)
        {
            --exp;
            b=b*10.0;
        }
    }

    /*
      now decide how to print and save in FORMAT.
         -1 => we need leading digits
          0 => print in exp
         +1 => we have digits before dp.
    */

    switch (cnvtype)
    {
        case 'E':
        case 'e':
            format = 0;
            break;
        case 'f':
        case 'F':
            if ( exp >= 0 ) format = 1; else format = -1;
            break;
        default: 
            /* Style e is used if the exponent from its 
               conversion is less than -4 or greater than 
               or equal to the precision.
            */
            if ( exp >= 0 )
            {
                if ( nprecision > exp )
                {
                    format=1;
                }
                else
                {
                    format=0;
                }
            }
            else
            {
                /*  if ( nprecision > (-(exp+1) ) ) { */
                if ( exp >= -4)
                {
                    format=-1;
                }
                else
                {
                    format=0;
                }
            }
            break;
    }
    /*
       Now extract the requisite number of digits
    */

    if (format==-1)
    {
        /*
             Number < 1.0 so we need to print the "0." 
             and the leading zeros...
        */
        result[0]=sign;
        result[1]='0';
        result[2]='.';
        result[3]=0x00;
        while (++exp)
        {
            --nprecision;
            strcat(result,"0");
        }
        i=b;
        --nprecision;
        work[0] = (char)('0' + i % 10);
        work[1] = 0x00;
        strcat(result,work);

        pdigits = nprecision;

        while (pdigits-- > 0)
        {
            b = b - i;
            b = b * 10.0;
            i = b;
            work[0] = (char)('0' + i % 10);
            work[1] = 0x00;
            strcat(result,work);
        }
    }
    /*
       Number >= 1.0 just print the first digit
    */
    else if (format==+1)
    {
        i = b;
        result[0] = sign;
        result[1] = '\0';
        work[0] = (char)('0' + i % 10);
        work[1] = 0x00;
        strcat(result,work);
        nprecision = nprecision + exp;
        pdigits = nprecision ;

        while (pdigits-- > 0)
        {
            if ( ((nprecision-pdigits-1)==exp)  )
            {
                strcat(result,".");
            }
            b = b - i;
            b = b * 10.0;
            /* the following test needs to be adjusted to 
               allow for numeric fuzz */
            if ( ( (nprecision-pdigits-1) > exp) && (b < 0.1E-15 ) )
            { 
                break;
            }
            i = b;
            work[0] = (char)('0' + i % 10);
            work[1] = 0x00;
            strcat(result,work);
        }
    }
    /*
       printing in standard form
    */
    else
    {
        i = b;
        result[0] = sign;
        result[1] = '\0';
        work[0] = (char)('0' + i % 10);
        work[1] = 0x00;
        strcat(result,work);
        strcat(result,".");
 
        pdigits = nprecision;

        while (pdigits-- > 0)
        {
            b = b - i;
            b = b * 10.0;
            i = b;
            work[0] = (char)('0' + i % 10);
            work[1] = 0x00;
            strcat(result,work);
        }
    }

    if (format==0)
    { /* exp format - put exp on end */
        work[0] = 'E';
        if ( exp < 0 )
        {
            exp = -exp;
            work[1]= '-';
        }
        else
        {
            work[1]= '+';
        }      
        work[2] = (char)('0' + (exp/10) % 10);
        work[3] = (char)('0' + exp % 10);     
        work[4] = 0x00;
        strcat(result, work);
    }
    /* printf(" Final Answer = <%s> fprintf goves=%g\n", 
                result,num); */
    return;
}
