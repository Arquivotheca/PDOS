/* written by Paul Edwards */
/* released to the public domain */
/* patchver - patch version of executable to put in current MSDOS version */

#include <stdio.h>
#include <stdlib.h>

unsigned int getversion(void);

int main(int argc, char **argv)
{
    FILE *fu;
    unsigned int version;
    
    if (argc < 2)
    {
        printf("usage: patchver <filename>\n");
        return (EXIT_FAILURE);
    }
    fu = fopen(*(argv + 1), "rb+");
    if (fu == NULL)
    {
        printf("unable to open file %s\n", *(argv + 1));
        return (EXIT_FAILURE);
    }
    version = getversion();
    fseek(fu, 3, SEEK_SET);
    fputc(version >> 8, fu);
    fputc(version & 0xff, fu);
    fclose(fu);
    return (0);
}

#include <dos.h>

unsigned int getversion(void)
{
    union REGS regsin;
    union REGS regsout;
    
    regsin.h.ah = 0x30;
    int86(0x21, &regsin, &regsout);
    return (((unsigned int)regsout.h.al << 8) | regsout.h.ah);
}
