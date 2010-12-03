/*********************************************************************/
/*                                                                   */
/*  This Program Written By Paul Edwards.                            */
/*  Released to the public domain.                                   */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  tocil - punch object code given a binary input file              */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXBUF 2000000L
#define MAXRLEN 1000 /* maximum length of a single record */
#define CARDLEN 80
#define MAXCDATA 56  /* maximum data bytes on TXT record */

int main(int argc, char **argv)
{
    char *buf;
    char card[CARDLEN];
    FILE *fp;
    FILE *fq;
    size_t cnt;
    size_t tot;
    size_t rem;
    size_t upto;
    size_t x;
    size_t r;

    if (argc <= 3)
    {
        fprintf(stderr, "usage: tocil <binary file> <object file> "
                "<phase name>\n");
        fprintf(stderr, "will read binary file and write to object file\n");
        return (EXIT_FAILURE);
    }

    buf = malloc(MAXBUF);
    if (buf == NULL)
    {
        fprintf(stderr, "out of memory error\n");
        exit(EXIT_FAILURE);
    }

    fp = fopen(*(argv + 1), "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "failed to open %s for input\n", *(argv + 1));
        return (EXIT_FAILURE);
    }

    tot = fread(buf, 1, MAXBUF, fp);
    if (tot >= MAXBUF)
    {
        fprintf(stderr, "file too big (%ld or more) to process\n", MAXBUF);
        return (EXIT_FAILURE);
    }

    fq = fopen(*(argv + 2), "wb");
    if (fq == NULL)
    {
        fprintf(stderr, "failed to open %s for output\n", *(argv + 2));
        return (EXIT_FAILURE);
    }

    memset(card, ' ', sizeof card);
    sprintf(card, " PHASE %.8s,*", *(argv + 3));
    *(card + strlen(card)) = ' ';
    fwrite(card, 1, sizeof card, fq);

    memset(card, 0x00, sizeof card);
    sprintf(card, "\x02" "ESD");
    *(short *)(card + 10) = 0x20; /* length of this ESD is the minimal 0x20 */
    *(short *)(card + 14) = 1; /* CSECT 1 */
    memset(card + 16, ' ', 8); /* name is blank */
    *(card + 24) = 0x04; /* PC for some reason */
    memcpy(card + 32, "TOTO    ", 8); /* total? */
    *(int *)(card + 44) = tot - 12; /* total minus 12 for some reason */
    fwrite(card, 1, sizeof card, fq);

    for (upto = 0; upto < tot; upto += rem)
    {
        rem = tot - upto;
        if (rem > MAXRLEN)
        {
            rem = MAXRLEN;
        }
        for (x = 0; x < rem; x += r)
        {
            r = rem - x;
            if (r > MAXCDATA)
            {
                r = MAXCDATA;
            }
            if ((x == 0) && (r > (MAXCDATA - sizeof(int))))
            {
                r -= sizeof(int);
            }
            memset(card, 0x00, sizeof card);
            sprintf(card, "\x02" "TXT");
            *(int *)(card + 4) = x; /* origin */
            *(int *)(card + 8) = r; /* byte count */
            *(int *)(card + 12) = 2; /* CSECT 2 */
            memcpy(card + 16, buf + upto + x, r);
            fwrite(card, 1, sizeof card, fq);
        }
    }
    memset(card, 0x00, sizeof card);
    sprintf(card, "\x02" "END");
    fwrite(card, 1, sizeof card, fq);

    return (0);
}
