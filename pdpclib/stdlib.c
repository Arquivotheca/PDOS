/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  stdlib.c - implementation of stuff in stdlib.h                   */
/*                                                                   */
/*********************************************************************/

#include "stdlib.h"
#include "signal.h"
#include "string.h"
#include "ctype.h"

/* PDOS and MSDOS use the same interface most of the time */
#ifdef __PDOS__
#define __MSDOS__
#endif

#ifdef __OS2__
#define INCL_DOSMISC
#define INCL_DOSPROCESS
#include <os2.h>
#endif

#if defined(__MVS__) || defined(__CMS__)
#include "mvssupa.h"
#endif

#ifdef __MSDOS__
#ifdef __WATCOMC__
#define CTYP __cdecl
#else
#define CTYP
#endif
void CTYP __allocmem(size_t size, void **ptr);
void CTYP __freemem(void *ptr);
extern unsigned char *__envptr;
void CTYP __exec(char *cmd, void *env);
#endif

void (*__userExit[__NATEXIT])(void);

void *malloc(size_t size)
{
#ifdef __OS2__
    PVOID BaseAddress;
    ULONG ulObjectSize;
    ULONG ulAllocationFlags;
    APIRET rc;

    ulObjectSize = size + sizeof(size_t);
    ulAllocationFlags = PAG_COMMIT | PAG_WRITE | PAG_READ;
    rc = DosAllocMem(&BaseAddress, ulObjectSize, ulAllocationFlags);
    if (rc != 0) return (NULL);
    *(size_t *)BaseAddress = size;
    BaseAddress = (char *)BaseAddress + sizeof(size_t);
    return ((void *)BaseAddress);
#endif
#ifdef __MSDOS__
    void *ptr;

    __allocmem(size, &ptr);
    return (ptr);
#endif
#if defined(__MVS__) || defined(__CMS__)
    return (__getm(size));
#endif
}

void *calloc(size_t nmemb, size_t size)
{
    void *ptr;
    size_t total;

    if (nmemb == 1)
    {
        total = size;
    }
    else if (size == 1)
    {
        total = nmemb;
    }
    else
    {
        total = nmemb * size;
    }
    ptr = malloc(total);
    if (ptr != NULL)
    {
        memset(ptr, '\0', total);
    }
    return (ptr);
}

void *realloc(void *ptr, size_t size)
{
    char *newptr;
    size_t oldsize;

    newptr = malloc(size);
    if (newptr == NULL)
    {
        return (NULL);
    }
    if (ptr != NULL)
    {
#if defined(__MVS__) || defined(__CMS__)
        oldsize = *(size_t *)((char *)ptr - 16);
        oldsize -= 16;
#else
        oldsize = *(size_t *)((char *)ptr - 4);
#endif
        if (oldsize < size)
        {
            size = oldsize;
        }
        memcpy(newptr, ptr, size);
        free(ptr);
    }
    return (newptr);
}

void free(void *ptr)
{
#ifdef __OS2__
    if (ptr != NULL)
    {
        ptr = (char *)ptr - sizeof(size_t);
        DosFreeMem((PVOID)ptr);
    }
#endif
#ifdef __MSDOS__
    if (ptr != NULL)
    {
        __freemem(ptr);
    }
#endif
#if defined(__MVS__) || defined(__CMS__)
    if (ptr != NULL)
    {
        __freem(ptr);
    }
#endif
    return;
}

void abort(void)
{
    raise(SIGABRT);
    exit(EXIT_FAILURE);
#ifndef __EMX__
    return;
#endif
}

#ifndef __EMX__
void __exit(int status);
#else
void __exit(int status) __attribute__((noreturn));
#endif

void exit(int status)
{
    __exit(status);
#ifndef __EMX__
    return;
#endif
}

/******************************************************************/
/* qsort.c  --  Non-Recursive ISO C qsort() function              */
/*                                                                */
/* Public domain by Raymond Gardner, Englewood CO  February 1991  */
/* Minor mods by Paul Edwards also public domain                  */
/*                                                                */
/* Usage:                                                         */
/*     qsort(base, nbr_elements, width_bytes, compare_function);  */
/*        void *base;                                             */
/*        size_t nbr_elements, width_bytes;                       */
/*        int (*compare_function)(const void *, const void *);    */
/*                                                                */
/* Sorts an array starting at base, of length nbr_elements, each  */
/* element of size width_bytes, ordered via compare_function,     */
/* which is called as  (*compare_function)(ptr_to_element1,       */
/* ptr_to_element2) and returns < 0 if element1 < element2,       */
/* 0 if element1 = element2, > 0 if element1 > element2.          */
/* Most refinements are due to R. Sedgewick. See "Implementing    */
/* Quicksort Programs", Comm. ACM, Oct. 1978, and Corrigendum,    */
/* Comm. ACM, June 1979.                                          */
/******************************************************************/

/* prototypes */
static void swap_chars(char *, char *, size_t);

/*
** Compile with -DSWAP_INTS if your machine can access an int at an
** arbitrary location with reasonable efficiency.  (Some machines
** cannot access an int at an odd address at all, so be careful.)
*/

#ifdef   SWAP_INTS
 void swap_ints(char *, char *, size_t);
 #define  SWAP(a, b)  (swap_func((char *)(a), (char *)(b), width))
#else
 #define  SWAP(a, b)  (swap_chars((char *)(a), (char *)(b), size))
#endif

#define  COMP(a, b)  ((*comp)((void *)(a), (void *)(b)))

#define  T           7    /* subfiles of T or fewer elements will */
                          /* be sorted by a simple insertion sort */
                          /* Note!  T must be at least 3          */

void qsort(void *basep, size_t nelems, size_t size,
                            int (*comp)(const void *, const void *))
{
   char *stack[40], **sp;       /* stack and stack pointer        */
   char *i, *j, *limit;         /* scan and limit pointers        */
   size_t thresh;               /* size of T elements in bytes    */
   char *base;                  /* base pointer as char *         */

#ifdef   SWAP_INTS
   size_t width;                /* width of array element         */
   void (*swap_func)(char *, char *, size_t); /* swap func pointer*/

   width = size;                /* save size for swap routine     */
   swap_func = swap_chars;      /* choose swap function           */
   if ( size % sizeof(int) == 0 ) {   /* size is multiple of ints */
      width /= sizeof(int);           /* set width in ints        */
      swap_func = swap_ints;          /* use int swap function    */
   }
#endif

   base = (char *)basep;        /* set up char * base pointer     */
   thresh = T * size;           /* init threshold                 */
   sp = stack;                  /* init stack pointer             */
   limit = base + nelems * size;/* pointer past end of array      */
   for ( ;; ) {                 /* repeat until break...          */
      if ( limit - base > thresh ) {  /* if more than T elements  */
                                      /*   swap base with middle  */
         SWAP(((((size_t)(limit-base))/size)/2)*size+base, base);
         i = base + size;             /* i scans left to right    */
         j = limit - size;            /* j scans right to left    */
         if ( COMP(i, j) > 0 )        /* Sedgewick's              */
            SWAP(i, j);               /*    three-element sort    */
         if ( COMP(base, j) > 0 )     /*        sets things up    */
            SWAP(base, j);            /*            so that       */
         if ( COMP(i, base) > 0 )     /*      *i <= *base <= *j   */
            SWAP(i, base);            /* *base is pivot element   */
         for ( ;; ) {                 /* loop until break         */
            do                        /* move i right             */
               i += size;             /*        until *i >= pivot */
            while ( COMP(i, base) < 0 );
            do                        /* move j left              */
               j -= size;             /*        until *j <= pivot */
            while ( COMP(j, base) > 0 );
            if ( i > j )              /* if pointers crossed      */
               break;                 /*     break loop           */
            SWAP(i, j);       /* else swap elements, keep scanning*/
         }
         SWAP(base, j);         /* move pivot into correct place  */
         if ( j - base > limit - i ) {  /* if left subfile larger */
            sp[0] = base;             /* stack left subfile base  */
            sp[1] = j;                /*    and limit             */
            base = i;                 /* sort the right subfile   */
         } else {                     /* else right subfile larger*/
            sp[0] = i;                /* stack right subfile base */
            sp[1] = limit;            /*    and limit             */
            limit = j;                /* sort the left subfile    */
         }
         sp += 2;                     /* increment stack pointer  */
      } else {      /* else subfile is small, use insertion sort  */
         for ( j = base, i = j+size; i < limit; j = i, i += size )
            for ( ; COMP(j, j+size) > 0; j -= size ) {
               SWAP(j, j+size);
               if ( j == base )
                  break;
            }
         if ( sp != stack ) {         /* if any entries on stack  */
            sp -= 2;                  /* pop the base and limit   */
            base = sp[0];
            limit = sp[1];
         } else                       /* else stack empty, done   */
            break;
      }
   }
}

/*
**  swap nbytes between a and b
*/

static void swap_chars(char *a, char *b, size_t nbytes)
{
   char tmp;
   do {
      tmp = *a; *a++ = *b; *b++ = tmp;
   } while ( --nbytes );
}

#ifdef   SWAP_INTS

/*
**  swap nints between a and b
*/

static void swap_ints(char *ap, char *bp, size_t nints)
{
   int *a = (int *)ap, *b = (int *)bp;
   int tmp;
   do {
      tmp = *a; *a++ = *b; *b++ = tmp;
   } while ( --nints );
}

#endif

static unsigned long myseed = 1;

void srand(unsigned int seed)
{
    myseed = seed;
    return;
}

int rand(void)
{
    int ret;

    myseed = myseed * 1103515245UL + 12345;
    ret = (int)((myseed >> 16) & 0x8fff);
    return (ret);
}

double atof(const char *nptr)
{
    return (strtod(nptr, (char **)NULL));
}

double strtod(const char *nptr, char **endptr)
{
    double x = 0.0;
    double xs= 1.0;
    double es = 1.0;
    double xf = 0.0;
    double xd = 1.0;
    int dp = 0;


 /* while(isspace(*nptr))++nptr; */
    if(*nptr == '-')
    {
        xs = -1;
        nptr++;
    }

    while (1)
    {
        if (isdigit(*nptr))
        {
            x = x * 10 + (*nptr - '0');
            nptr++;
        }
        else
        {
            x = x * xs;
            break;
        }
    }
    if (*nptr == '.')
    {
        dp = 1;
        nptr++;
        while (1)
        {
            if (isdigit(*nptr))
            {
                xf = xf * 10 + (*nptr - '0');
                xd = xd * 10;
            }
            else
            {
                x = x + xs * (xf / xd);
                break;
            }
            nptr++;
        }
    }
    if ((*nptr == 'e') || (*nptr == 'E'))
    {
        nptr++;
        if (*nptr == '-')
        {
            es = -1;
            nptr++;
        }
        dp = 0;
        xd = 1;
        xf = 0;
        while (1)
        {
            if (isdigit(*nptr))
            {
                xf = xf * 10 + (*nptr - '0');
                nptr++;
            }
            else
            {
                while (xf > 0)
                {
                    xd *= 10;
                    xf--;
                }
                if (es < 0.0)
                {
                    x = x / xd;
                }
                else
                {
                    x = x * xd;
                }
                break;
            }
        }
    }
    if (endptr != NULL)
    {
        *endptr = (char *)nptr;
    }
    return (x);
}

int atoi(const char *nptr)
{
    return ((int)strtol(nptr, (char **)NULL, 10));
}

long int atol(const char *nptr)
{
    return (strtol(nptr, (char **)NULL, 10));
}

long int strtol(const char *nptr, char **endptr, int base)
{
    long x = 0;
    int undecided = 0;

    if (base == 0)
    {
        undecided = 1;
    }
    while (1)
    {
        if (isdigit(*nptr))
        {
            if (base == 0)
            {
                if (*nptr == '0')
                {
                    base = 8;
                }
                else
                {
                    base = 10;
                    undecided = 0;
                }
            }
            x = x * base + (*nptr - '0');
            nptr++;
        }
        else if (isalpha(*nptr))
        {
            if ((*nptr == 'X') || (*nptr == 'x'))
            {
                if ((base == 0) || ((base == 8) && undecided))
                {
                    base = 16;
                    undecided = 0;
                }
                else
                {
                    break;
                }
            }
            else
            {
                x = x * base + (toupper((unsigned char)*nptr) - 'A') + 10;
                nptr++;
            }
        }
        else
        {
            break;
        }
    }
    if (endptr != NULL)
    {
        *endptr = (char *)nptr;
    }
    return (x);
}

unsigned long int strtoul(const char *nptr, char **endptr, int base)
{
    unsigned long x = 0;

    while (1)
    {
        if (isdigit(*nptr))
        {
            x = x * base + (*nptr - '0');
            nptr++;
        }
        else if (isalpha(*nptr) && (base > 10))
        {
            x = x * base + (toupper((unsigned char)*nptr) - 'A') + 10;
            nptr++;
        }
        else
        {
            break;
        }
    }
    if (endptr != NULL)
    {
        *endptr = (char *)nptr;
    }
    return (x);
}

int mblen(const char *s, size_t n)
{
    if (s == NULL)
    {
        return (0);
    }
    if (n == 1)
    {
        return (1);
    }
    else
    {
        return (-1);
    }
}

int mbtowc(wchar_t *pwc, const char *s, size_t n)
{
    if (s == NULL)
    {
        return (0);
    }
    if (n == 1)
    {
        if (pwc != NULL)
        {
            *pwc = *s;
        }
        return (1);
    }
    else
    {
        return (-1);
    }
}

int wctomb(char *s, wchar_t wchar)
{
    if (s != NULL)
    {
        *s = wchar;
        return (1);
    }
    else
    {
        return (0);
    }
}

size_t mbstowcs(wchar_t *pwcs, const char *s, size_t n)
{
    strncpy((char *)pwcs, s, n);
    if (strlen(s) >= n)
    {
        return (n);
    }
    return (strlen((char *)pwcs));
}

size_t wcstombs(char *s, const wchar_t *pwcs, size_t n)
{
    strncpy(s, (const char *)pwcs, n);
    if (strlen((const char *)pwcs) >= n)
    {
        return (n);
    }
    return (strlen(s));
}

#ifdef abs
#undef abs
#endif
int abs(int j)
{
    if (j < 0)
    {
        j = -j;
    }
    return (j);
}

div_t div(int numer, int denom)
{
    div_t x;

    x.quot = numer / denom;
    x.rem = numer % denom;
    return (x);
}

#ifdef labs
#undef labs
#endif
long int labs(long int j)
{
    if (j < 0)
    {
        j = -j;
    }
    return (j);
}

ldiv_t ldiv(long int numer, long int denom)
{
    ldiv_t x;

    x.quot = numer / denom;
    x.rem = numer % denom;
    return (x);
}

int atexit(void (*func)(void))
{
    int x;

    for (x = 0; x < __NATEXIT; x++)
    {
        if (__userExit[x] == 0)
        {
            __userExit[x] = func;
            return (0);
        }
    }
    return (-1);
}

char *getenv(const char *name)
{
#ifdef __OS2__
    PSZ result;

    if (DosScanEnv((void *)name, (void *)&result) == 0)
    {
        return ((char *)result);
    }
#endif
#ifdef __MSDOS__
    char *env;
    size_t lenn;

    env = (char *)__envptr;
    lenn = strlen(name);
    while (*env != '\0')
    {
        if (strncmp(env, name, lenn) == 0)
        {
            if (env[lenn] == '=')
            {
                return (&env[lenn + 1]);
            }
        }
        env = env + strlen(env) + 1;
    }
#endif
    return (NULL);
}

/* The following code was taken from Paul Markham's "EXEC" program,
   and adapted to create a system() function.  The code is all
   public domain */

int system(const char *string)
{
#ifdef __OS2__
    char err_obj[100];
	APIRET rc;
	RESULTCODES results;

    if (string == NULL)
    {
        return (1);
    }
    rc = DosExecPgm(err_obj, sizeof err_obj, EXEC_SYNC,
                    (PSZ)string, NULL, &results, (PSZ)string);
    if (rc != 0)
    {
        return (rc);
    }
    return ((int)results.codeResult);
#endif
#ifdef __MSDOS__
    static unsigned char cmdt[140];
    static struct {
        int env;
        unsigned char *cmdtail;
        char *fcb1;
        char *fcb2;
    } parmblock = { 0, cmdt, NULL, NULL };
    size_t len;
    char *cmd;

    len = strlen(string);
    cmdt[0] = (unsigned char)(len + 3);
    memcpy(&cmdt[1], "/c ", 3);
    memcpy(&cmdt[4], string, len);
    memcpy(&cmdt[len + 4], "\r", 2);
    cmd = getenv("COMSPEC");
    if (cmd == NULL)
    {
        cmd = "\\command.com";
    }
    __exec(cmd, &parmblock);
    return (0);
#endif
}

void *bsearch(const void *key, const void *base,
              size_t nmemb, size_t size,
              int (*compar)(const void *, const void *))
{
    size_t try;
    int res;
    const void *ptr;

    while (nmemb > 0)
    {
        try = nmemb / 2;
        ptr = (void *)((char *)base + try * size);
        res = compar(ptr, key);
        if (res == 0)
        {
            return ((void *)ptr);
        }
        else if (res < 0)
        {
            nmemb = nmemb - try - 1;
            base = (const void *)((const char *)ptr + size);
        }
        else
        {
            nmemb = try;
        }
    }
    return (NULL);
}
