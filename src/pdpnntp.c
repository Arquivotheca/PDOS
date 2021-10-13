/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards                             */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pdpnntp - a nntp newsgroup reader                                */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>

static FILE *comm;
static char buf[1000];

static char *getline(FILE *comm, char *buf, size_t szbuf);
static char *putline(FILE *comm, char *buf);

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("usage: pdpnntp <comm channel>\n");
        return (EXIT_FAILURE);
    }

    comm = fopen(*(argv + 1), "r+b");
    if (comm == NULL)
    {
        printf("can't open %s\n", *(argv + 1));
        return (EXIT_FAILURE);
    }

    setvbuf(comm, NULL, _IONBF, 0);

    getline(comm, buf, sizeof buf); 
    printf("%s\n", buf);

    fseek(comm, 0, SEEK_CUR);

    putline(comm, "LIST");

    while (1)
    {
        getline(comm, buf, sizeof buf);
        if (buf[0] == '.') break;
        printf("%s\n", buf);
    }

    fclose(comm);
    return (0);
}

static char *getline(FILE *comm, char *buf, size_t szbuf)
{
    size_t x;
    int c;

    for (x = 0; x < szbuf - 2; x++)
    {
        c = getc(comm);
        if (c == 0x0d)
        {
            getc(comm);
            break;
        }
        buf[x] = c;
    }
    buf[x] = '\0';
    return (buf);
}

static char *putline(FILE *comm, char *buf)
{
    size_t x;

    x = 0;
    while (buf[x] != '\0')
    {
        putc(buf[x], comm);
        x++;
    }
    putc(0x0d, comm);
    putc(0x0a, comm);
    return (buf);
}
