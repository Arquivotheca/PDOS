#include <stdio.h>
#include <ctype.h>

#include "lldos.h"

int main(void)
{
    FARADDR faddr;
    unsigned int x;
    unsigned int y;
    DREGS in;
    DREGS out;
    char buf[3000];
    
    gfarmem(&faddr, 512);
/*    printf("addr is %lX\n", (unsigned long)faddr); */
    splitfar(&faddr, &x, &y);
    in.ax = 0x0201U;
    in.es = x;
    in.bx = y;
/*    in.es = FP_SEG((char far *)buf);
    in.bx = FP_OFF((char far *)buf); */
    joinfar(&faddr, x, y);
/*    printf("addr is %lX\n", (unsigned long)faddr); */
    in.cx = 0x0001U;
    in.dx = 0x0000U;
    dint(0x13, &in, &out);
    printf("cf is %u\n", out.cf);
    printf("ax is %x\n", out.ax);
    fromfar(&faddr, buf, 512);
    for (x=0; x <512; x++)
    {
        if (isprint(buf[x])) printf("%c", buf[x]);
    }
    return (0);
}

