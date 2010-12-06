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
#define CARDLEN 80
#define MAXCDATA 56  /* maximum data bytes on TXT record */
#define MAXRLEN (MAXCDATA * 10 - 4) /* maximum length of a single record */
    /* the 4 is to ensure the length is never on a card by itself */

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
    size_t subtot;

    if (argc <= 2)
    {
        fprintf(stderr, "usage: tocil <binary file> <object file>\n");
        fprintf(stderr, "will read binary file and write to object file\n");
        return (EXIT_FAILURE);
    }

    buf = malloc(MAXBUF + 4);
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
    memset(buf + tot, 0x00, 4);
    tot += 4;

    fq = fopen(*(argv + 2), "wb");
    if (fq == NULL)
    {
        fprintf(stderr, "failed to open %s for output\n", *(argv + 2));
        return (EXIT_FAILURE);
    }

    memset(card, ' ', sizeof card);
    sprintf(card, "\x02" "ESD");
    *(short *)(card + 10) = 0x20; /* length of this ESD is the minimal 0x20 */
    *(short *)(card + 14) = 1; /* CSECT 1 */
    memset(card + 16, ' ', 8); /* name is blank */

    *(int *)(card + 24) = 0; /* assembled origin = 0 */
    *(card + 24) = 0x04; /* PC for some reason */
    *(int *)(card + 28) = 0; /* AMODE + length - for some reason we
        don't need to set this properly. */

#if 0
    /* is this required? */
    *(int *)(card + 28) = tot + 
        (tot/MAXRLEN + ((tot % MAXRLEN) != 0)) * sizeof(int);
#endif

    memcpy(card + 32, "TOTO    ", 8); /* total? */
    
    /* is this required? */
    *(int *)(card + 44) = tot + 
        (tot/MAXRLEN + ((tot % MAXRLEN) != 0)) * sizeof(int);
    fwrite(card, 1, sizeof card, fq);

    subtot = 0;
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
            *(int *)(card + 4) = subtot; /* origin */
            *(int *)(card + 8) = r + ((x == 0) ? sizeof(int) : 0);
                /* byte count */
            *(int *)(card + 12) = 1; /* CSECT 1 - was 2 */
            if (x == 0)
            {
                *(int *)(card + 16) = rem;
                if ((upto + rem) >= tot)
                {
                    *(int *)(card + 16) -= 4;
                }
                memcpy(card + 16 + sizeof(int), buf + upto, r);
                subtot += (r + sizeof(int));
            }
            else
            {
                memcpy(card + 16, buf + upto + x, r);
                subtot += r;
            }
            fwrite(card, 1, sizeof card, fq);
        }
    }
    memset(card, ' ', sizeof card);
    memcpy(card, "\x02" "END", 4);
#if 0
    /* is this required? */
    *(int *)(card + 24) = tot + 
        (tot/MAXRLEN + ((tot % MAXRLEN) != 0)) * sizeof(int);
#endif
    fwrite(card, 1, sizeof card, fq);

    return (0);
}
