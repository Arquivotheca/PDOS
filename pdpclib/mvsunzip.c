/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards                             */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  mvsunzip - unzip files compressed with -0                        */
/*  e.g. mvsunzip pdpclib.czip pdpclib.c                             */
/*  or on cms, just "mvsunzip pdpclib.czip".                         */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAXBUF 2000000L

static void usage(void);
static int onefile(FILE *infile);
static char *ascii2l(char *buf);
static int fasc(int asc);
static int ins_strcmp(char *one, char *two);

/* for MVS, pdsn is the optional PDS name. */
#ifdef __MVS__
static char *pdsn = NULL;
#endif
static int binary = 0;
#ifdef __CMS__
static int disk = '\0';
#endif
#ifdef __VSE__
/* this is required for VSE */
static char *outdd = NULL;
static char *zipm = NULL;
#endif

int main(int argc, char **argv)
{
    FILE *infile;
    int aupto = 2;

    if (argc <= 1) usage();

#if defined(__CMS__)
/* check to see if we have a CMS minidisk provided */

    if (argc > aupto)
    {
        if (strlen(argv[aupto]) == 1)
        {
            disk = argv[aupto][0];
            aupto++;
        }
    }

#elif defined(__VSE__)
/* VSE requires an output DDNAME, even for library output */

    if (argc > aupto)
    {
        outdd = argv[aupto++];
    }
    else
    {
        usage();
    }

#elif defined(__MVS__)
/* MVS has an optional pds for output */
    if (argc > aupto)
    {
        if (ins_strcmp(argv[aupto], "binary") != 0)
        {
            pdsn = argv[aupto];
            aupto++;
        }
    }

#else
    /* Normal systems like Windows don't have a mandatory unusual argument */
#endif

/* VSE may request just a single file - should probably enable
   for other systems too */
#ifdef __VSE__
    if (argc > aupto)
    {
        if (ins_strcmp(argv[aupto], "binary") != 0)
        {
            zipm = argv[aupto];
            aupto++;
        }
    }
#endif

/* all systems - see if this is a binary file */
    if (argc > aupto)
    {
        if (ins_strcmp(argv[aupto], "binary") == 0)
        {
            binary = 1;
            aupto++;
        }
    }
    
    if (argc > aupto) usage();

    printf("processing data from %s\n", *(argv + 1));
    infile = fopen(*(argv+1), "rb");
    if (infile == NULL)
    {
       printf("Open Failed code %d\n", errno);
       return (EXIT_FAILURE);
    }
    while (onefile(infile)) ;
    return (0);
}

static void usage(void)
{
#if defined(__CMS__)
    printf("usage: mvsunzip <infile> [disk] [BINARY]\n");
    printf("where infile is a sequential file\n");
    printf("e.g. mvsunzip dd:input\n");
#elif defined(__VSE__)
    printf("usage: mvsunzip <infile> <outdd> [file] [binary]\n");
    printf("where outdd can be either a sequential file or library\n");
    printf("and file is the name of the file in the zip archive\n");
#elif defined(__MVS__)
    printf("usage: mvsunzip <infile> [outpds] [binary]\n");
    printf("where infile is a sequential file\n");
    printf("and outpds is a PDS (not supported on all systems)\n");
    printf("e.g. mvsunzip dd:input dd:output\n");
#else
    printf("usage: mvsunzip <infile> [binary]\n");
    printf("where infile is an uncompressed zip file\n");
    printf("note that for text unzips, data is assumed to be ASCII\n");
    printf("and will be converted to the local character set if that\n");
    printf("is not already ASCII\n");
    printf("e.g. mvsunzip input.zip binary\n");
#endif
    exit(EXIT_FAILURE);
}

static int onefile(FILE *infile)
{
    int c;
    int x;
    long size;
    long size2;
    int fnmlen;
    char fnm[FILENAME_MAX];
    static char *buf = NULL;
    char newfnm[FILENAME_MAX];
    FILE *newf;
    int extra;
    char *p;
    int skip = 0;

    if (buf == NULL)
    {
        buf = malloc(MAXBUF);
        if (buf == NULL)
        {
            printf("out of memory error\n");
            exit(EXIT_FAILURE);
        }
    }
    if (fgetc(infile) != 0x50)
    {
        return (0);
    }
    if (fgetc(infile) != 0x4b)
    {
        return (0);
    }
    if (fgetc(infile) != 0x03)
    {
        return (0);
    }
    if (fgetc(infile) != 0x04)
    {
        return (0);
    }
    for (x = 0; x < 14; x++)
    {
        fgetc(infile);
    }
    c = fgetc(infile);
    size = c;
    c = fgetc(infile);
    size = (c << 8) | size;
    c = fgetc(infile);
    size = (c << 16) | size;
    c = fgetc(infile);
    size = (c << 24) | size;
    if ((size > MAXBUF) && !binary)
    {
        printf("warning - file is too big (%d) at pos %d - ending early\n",
               size, ftell(infile));
        return (0);
    }
    c = fgetc(infile);
    size2 = c;
    c = fgetc(infile);
    size2 = (c << 8) | size2;
    c = fgetc(infile);
    size2 = (c << 16) | size2;
    c = fgetc(infile);
    size2 = (c << 24) | size2;
    if (size != size2)
    {
        printf("warning - compressed file found - ending early\n");
        return (0);
    }
    c = fgetc(infile);
    fnmlen = c;
    c = fgetc(infile);
    fnmlen = (c << 8) | fnmlen;
    c = fgetc(infile);
    extra = c;
    c = fgetc(infile);
    extra = (c << 8) | extra;
    if (fnmlen == 0) return (0);
    fread(fnm, fnmlen, 1, infile);
    fnm[fnmlen] = '\0';
    fread(buf, extra, 1, infile);
    ascii2l(fnm);
    printf("fnm is %s\n", fnm);
    if (!binary)
    {
        fread(buf, size, 1, infile);
        buf[size] = '\0';
    }
    p = strrchr(fnm, '/');
    if (p != NULL)
    {
        p++;
    }
    else
    {
        p = fnm;
    }

    if (strrchr(p, '\\') != NULL)
    {
        p = strrchr(p, '\\') + 1;
    }

#if defined(__VSE__)
    if (zipm != NULL)
    {
        if (ins_strcmp(p, zipm) != 0)
        {
            if (!binary) return (1);
            for (x = 0; x < size; x += MAXBUF)
            {
                size_t y;

                y = size - x;
                if (y > MAXBUF)
                {
                    y = MAXBUF;
                }
                fread(buf, y, 1, infile);
            }
            return (1); /* skip this file */
        }
    }
#endif

/* for MVS, if no PDS name was provided, we use the zip member
   unchanged, via dynamic allocation. Otherwise, needs to be mangled.
   for VSE, we do not have dynamic allocation, and always have
   an output DD. But we only need to construct a member
   of a library when we have no zip member requested */

#if defined(__MVS__)
    if (pdsn != NULL)
#elif defined(__VSE__)
    if (zipm == NULL)
#else
    if (0)
#endif
    {
        if (strchr(p, '.') != NULL) *strchr(p, '.') = '\0';
        while (strchr(p, '-') != NULL) *strchr(p, '-') = '@';
        while (strchr(p, '_') != NULL) *strchr(p, '_') = '@';
    }

#if defined(__CMS__)
    if (strchr(p, '.') != NULL) *strchr(p, '.') = ' ';
    if (disk != '\0')
    {
        if (strchr(p, ' ') != NULL)
        {
            sprintf(newfnm, "%s %c", p, disk);
        }
        else
        {
            sprintf(newfnm, "%s FILE %c", p, disk);
        }
    }
    else
    {
        sprintf(newfnm, "%s", p);
    }

#elif defined(__VSE__)
    if (zipm == NULL)
    {
        sprintf(newfnm, "%s(%s)", outdd, p);
    }
    else
    {
        strcpy(newfnm, outdd);
    }

#elif defined(__MVS__)
    if (pdsn != NULL)
    {
        sprintf(newfnm, "%s(%s)", pdsn, p);
    }
    else
    {
        /* MVS will do dynamic allocation of the unchanged zip
           file member. Note that PDOS relies heavily on this
           style */
        strcpy(newfnm, p);
#if defined(MUSIC)
        /* automatically truncate filenames down to MUSIC length */
        if (strlen(newfnm) > 17)
        {
            p = strrchr(newfnm, '.');
            if (p != NULL)
            {
                memmove(p - (strlen(newfnm) - 17), p, strlen(p) + 1);
            }
        }
#endif
    }

#else
    sprintf(newfnm, "%s", p);
#endif


    if (binary)
    {
        size_t x;

        newf = fopen(newfnm, "wb");
        if (newf == NULL)
        {
            printf("file open failure on %s\n", newfnm);
            exit(EXIT_FAILURE);
        }
        for (x = 0; x < size; x += MAXBUF)
        {
            size_t y;

            y = size - x;
            if (y > MAXBUF)
            {
                y = MAXBUF;
            }
            fread(buf, y, 1, infile);
            fwrite(buf, y, 1, newf);
        }
    }
    else
    {
        newf = fopen(newfnm, "w");
        if (newf == NULL)
        {
            printf("file open failure on %s\n", newfnm);
            exit(EXIT_FAILURE);
        }
        ascii2l(buf);
        fwrite(buf, strlen(buf), 1, newf);
    }
    fclose(newf);
    return (1);
}

static char *ascii2l(char *buf)
{
    char *p;
    char *q;
    int c;

    p = buf;
    q = buf;
    while (*p != '\0')
    {
        c = fasc(*p);
        if (c == '\0')
        {
            printf("warning - converting x'%02X' to space\n", *p);
            c = ' ';
        }
        if (c != '\r')
        {
            *q++ = (char)c;
        }
        p++;
    }
    *q = '\0';
    return (buf);
}

static int fasc(int asc)
{
  switch (asc)
  {
    case 0x09 : return('\t');
    case 0x0a : return('\n');
    case 0x0c : return('\f');
    case 0x0d : return('\r');
    case 0x20 : return(' ');
    case 0x21 : return('!');
    case 0x22 : return('\"');
    case 0x23 : return('#');
    case 0x24 : return('$');
    case 0x25 : return('%');
    case 0x26 : return('&');
    case 0x27 : return('\'');
    case 0x28 : return('(');
    case 0x29 : return(')');
    case 0x2a : return('*');
    case 0x2b : return('+');
    case 0x2c : return(',');
    case 0x2d : return('-');
    case 0x2e : return('.');
    case 0x2f : return('/');
    case 0x30 : return('0');
    case 0x31 : return('1');
    case 0x32 : return('2');
    case 0x33 : return('3');
    case 0x34 : return('4');
    case 0x35 : return('5');
    case 0x36 : return('6');
    case 0x37 : return('7');
    case 0x38 : return('8');
    case 0x39 : return('9');
    case 0x3a : return(':');
    case 0x3b : return(';');
    case 0x3c : return('<');
    case 0x3d : return('=');
    case 0x3e : return('>');
    case 0x3f : return('?');
    case 0x40 : return('@');
    case 0x41 : return('A');
    case 0x42 : return('B');
    case 0x43 : return('C');
    case 0x44 : return('D');
    case 0x45 : return('E');
    case 0x46 : return('F');
    case 0x47 : return('G');
    case 0x48 : return('H');
    case 0x49 : return('I');
    case 0x4a : return('J');
    case 0x4b : return('K');
    case 0x4c : return('L');
    case 0x4d : return('M');
    case 0x4e : return('N');
    case 0x4f : return('O');
    case 0x50 : return('P');
    case 0x51 : return('Q');
    case 0x52 : return('R');
    case 0x53 : return('S');
    case 0x54 : return('T');
    case 0x55 : return('U');
    case 0x56 : return('V');
    case 0x57 : return('W');
    case 0x58 : return('X');
    case 0x59 : return('Y');
    case 0x5a : return('Z');
    case 0x5b : return('[');
    case 0x5c : return('\\');
    case 0x5d : return(']');
    case 0x5e : return('^');
    case 0x5f : return('_');
    case 0x60 : return('`');
    case 0x61 : return('a');
    case 0x62 : return('b');
    case 0x63 : return('c');
    case 0x64 : return('d');
    case 0x65 : return('e');
    case 0x66 : return('f');
    case 0x67 : return('g');
    case 0x68 : return('h');
    case 0x69 : return('i');
    case 0x6a : return('j');
    case 0x6b : return('k');
    case 0x6c : return('l');
    case 0x6d : return('m');
    case 0x6e : return('n');
    case 0x6f : return('o');
    case 0x70 : return('p');
    case 0x71 : return('q');
    case 0x72 : return('r');
    case 0x73 : return('s');
    case 0x74 : return('t');
    case 0x75 : return('u');
    case 0x76 : return('v');
    case 0x77 : return('w');
    case 0x78 : return('x');
    case 0x79 : return('y');
    case 0x7a : return('z');
    case 0x7b : return('{');
    case 0x7c : return('|');
    case 0x7d : return('}');
    case 0x7e : return('~');
    default   : return(0);
  }
}

static int ins_strcmp(char *one, char *two)
{
    while (toupper(*one) == toupper(*two))
    {
        if (*one == '\0')
        {
            return (0);
        }
        one++;
        two++;
    }
    if (toupper(*one) < toupper(*two))
    {
        return (-1);
    }
    return (1);
}

