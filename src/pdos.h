/* public domain by Paul Edwards */

/* contains definitions required by both pload and pdos */

typedef struct {
    unsigned long transferbuf;
    unsigned long doreboot;
    unsigned long bpb;
} pdos_parms;
