/* replace DCC macros with inline code */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char buf[200];
    char *d;
    char *e;
    char *f;
    int frame;

    while (fgets(buf, sizeof buf, stdin) != NULL)
    {
        d = strstr(buf, "DCCPRLG");
        if (d != NULL)
        {
            frame = 0;
            e = strstr(buf, "ENTRY");
            f = strstr(buf, "FRAME");
            if (f != NULL)
            {
                frame = atoi(f + 6);
            }
            if (e != NULL)
            {
                strcpy(d, "ENTRY\n");
            }
            else
            {
                strcpy(d, "EQU\t*\n");
            }
            fputs(buf, stdout);
            printf("\tUSING\t*,15\n");
            printf("\tSAVE\t(14,12)\n");
            printf("\tL\t12,76(13)\n");
            printf("\tST\t13,4(12)\n");
            printf("\tST\t12,8(13)\n");
            printf("\tLR\t13,12\n");
            printf("\tA\t12,=F'%d'\n", frame);
            printf("\tST\t12,76(13)\n");
        }
        else if ((d = strstr(buf, "DCCEPIL")) != NULL)
        {
            strcpy(d, "RETURN (12,14),RC=(15)\n");
            fputs(buf, stdout);
        }
        else
        {
            fputs(buf, stdout);
        }
    }
    return (0);
}
