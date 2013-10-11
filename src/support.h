/* public domain by Paul Edwards */

#ifndef SUPPORT_INCLUDED
#define SUPPORT_INCLUDED

#ifdef __32BIT__
typedef struct {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
    unsigned int esi;
    unsigned int edi;
    unsigned int cflag;
    unsigned int flags;
} DWORDREGS;

typedef struct {
    unsigned short ax;
    unsigned short fill1;
    unsigned short bx;
    unsigned short fill2;
    unsigned short cx;
    unsigned short fill3;
    unsigned short dx;
    unsigned short fill4;
    unsigned short si;
    unsigned short fill5;
    unsigned short di;
    unsigned short fill6;
    unsigned short cflag;
    unsigned short fill7;    
    unsigned short flags;
    unsigned short fill8;
} WORDREGS;

typedef struct {
    unsigned char al;
    unsigned char ah;
    unsigned short fill1;
    unsigned char bl;
    unsigned char bh;
    unsigned short fill2;
    unsigned char cl;
    unsigned char ch;
    unsigned short fill3;
    unsigned char dl;
    unsigned char dh;
    unsigned short fill4;
} BYTEREGS;

#else

typedef struct {
    unsigned short ax;
    unsigned short bx;
    unsigned short cx;
    unsigned short dx;
    unsigned short si;
    unsigned short di;
    unsigned short cflag;
    unsigned short flags;
} WORDREGS;

typedef struct {
    unsigned char al;
    unsigned char ah;
    unsigned char bl;
    unsigned char bh;
    unsigned char cl;
    unsigned char ch;
    unsigned char dl;
    unsigned char dh;
} BYTEREGS;

#endif

union REGS
{
    BYTEREGS h;
    WORDREGS x;
#ifdef __32BIT__    
    DWORDREGS d;
#endif    
};

struct SREGS
{
    unsigned int es;
    unsigned int cs;
    unsigned int ss;
    unsigned int ds;
};

#ifdef __32BIT__
#define far
extern int __abscor;
extern char *__vidptr;
#define ABSADDR(x) ((void *)((char *)(x) - __abscor))
#define ADDR2ABS(x) ((void *)((char *)(x) + __abscor))
void enable(void);
void disable(void);
#else
/* ADDR2ABS - convert a 16:16 address into a flat absolute address */
#define ADDR2ABS(x) ((((unsigned long)(x) >> 16) << 4) \
                   + ((unsigned long)(x) & 0xffffU))
/* ABS2ADDR - convert a flat address into a 16:16 pointer */
#define ABS2ADDR(x) ((void far *)((((unsigned long)(x) & 0xf0000UL) << 12) \
                   | ((unsigned long)(x) & 0xffff)))
#endif

#define MK_FP(x, y) (void far *)(((unsigned long)(x) << 16) | (y))
#define FP_OFF(x) (unsigned int)((unsigned long)(x) & 0xffffU)
#define FP_SEG(x) (unsigned int)((unsigned long)(x) >> 16)
#define FP_NORM(x) (void *)(((((unsigned long)(x) >> 16) \
                     + (((unsigned long)(x) & 0xffffU) / 16)) << 16) \
                     | (((unsigned long)(x) & 0xffffU) % 16))

int int86(int intno, union REGS *regsin, union REGS *regsout);
int int86x(int intno, union REGS *regsin, 
           union REGS *regsout, struct SREGS *sregs);

#endif
