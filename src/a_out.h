/* written by Paul Edwards */
/* released to the public domain */
/* poor man's version of a.out.h - contains everything required by
   pdos at time of writing */

#define SEGMENT_SIZE 0x10000UL

struct exec {
    unsigned long a_info;
    unsigned long a_text;
    unsigned long a_data;
    unsigned long a_bss;
    unsigned long a_syms;
    unsigned long a_entry;
    unsigned long a_trsize;
    unsigned long a_drsize;
};

#define N_TXTOFF(e) (0x400)
#define N_TXTADDR(e) (SEGMENT_SIZE)
#define N_DATADDR(e) \
    (N_TXTADDR(e) + SEGMENT_SIZE + ((e).a_text & 0xffff0000UL))
#define N_BSSADDR(e) (N_DATADDR(e) + (e).a_data)
