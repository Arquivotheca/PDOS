/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards                             */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  mvsunzip - unzip files for use by pdpclib                        */
/*  e.g. mvsunzip pdpclib.czip pdpclib.c                             */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXBUF 500000

static int fasc(int asc);
static int onefile(FILE *infile);
static char *ascii2l(char *buf);
static char *outn;

int main(int argc, char **argv)
{
    FILE *infile;
    int c;

    if (argc <= 2)
    {
        printf("usage: mvsunzip <infile> <outfile>\n");
        printf("where infile is a sequential file\n");
        printf("and outfile is a PDS\n");
        printf("e.g. mvsunzip dd:input dd:output\n");
        return (0);
    }
    outn = *(argv+2);
    infile = fopen(*(argv+1), "rb");
    while (onefile(infile)) ;
    return (0);
}

static int onefile(FILE *infile)
{
    int c;
    int x;
    long size;
    int fnmlen;
    char fnm[FILENAME_MAX];
    static char *buf = NULL;
    char newfnm[FILENAME_MAX];
    FILE *newf;

    if (buf == NULL)
    {
        buf = malloc(MAXBUF);
    }
    if (fgetc(infile) != 0x50)
    {
        return (0);
    }
    if (fgetc(infile) != 0x4b)
    {
        return (0);
    }
    for (x = 0; x < 16; x++)
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
    if (size > MAXBUF)
    {
        return (0);
    }
    for (x = 0; x < 4; x++)
    {
        fgetc(infile);
    }
    c = fgetc(infile);
    fnmlen = c;
    c = fgetc(infile);
    fnmlen = (c << 8) | fnmlen;
    fgetc(infile);
    fgetc(infile);
    fread(fnm, fnmlen, 1, infile);
    fnm[fnmlen] = '\0';
    ascii2l(fnm);
    printf("fnm is %s\n", fnm);
    fread(buf, size, 1, infile);
    buf[size] = '\0';
    ascii2l(buf);
    if (strchr(fnm, '.') != NULL) *strchr(fnm, '.') = '\0';
    while (strchr(fnm, '-') != NULL) *strchr(fnm, '-') = '@';
    while (strchr(fnm, '_') != NULL) *strchr(fnm, '_') = '@';
    sprintf(newfnm, "%s(%s)", outn, fnm);
    newf = fopen(newfnm, "w");
    fwrite(buf, size, 1, newf);
    fclose(newf);
    return (1);
}

static char *ascii2l(char *buf)
{
    char *p;

    p = buf;
    while (*p != '\0')
    {
        *p = fasc(*p);
        p++;
    }
    return (buf);
}

static int fasc(int asc)
{
  switch (asc)
  {
    case 0x0a : return('\n');
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
