/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*  Reworked and AmigaOS and MVS added by Paul Edwards               */
/*  All changes remain public domain                                 */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  exeload.c - functions for loading executables                    */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exeload.h"

/* Headers for executable support. */
#include "a_out.h"

#if 0
#include "elf.h"
#include "mz.h"
#include "pecoff.h"
#endif

/* For reading files Pos API must be used directly
 * as PDPCLIB allocates memory from the process address space. */

static int exeloadLoadAOUT(char **entry_point, FILE *fp, char **loadloc);

#if 0
static int exeloadLoadELF(unsigned long *entry_point, int fhandle);
static int exeloadLoadMZ(unsigned long *entry_point, int fhandle);
/* Subfunctions of exeloadLoadMZ() for loading extensions to MZ. */
static int exeloadLoadPE(unsigned long *entry_point,
                         int fhandle,
                         unsigned long e_lfanew);
static int exeloadLoadLX(unsigned long *entry_point,
                         int fhandle,
                         unsigned long e_lfanew);
static int exeloadLoadNE(unsigned long *entry_point,
                         int fhandle,
                         unsigned long e_lfanew);

/* Support function for loading DLLs. */
static int exeloadLoadPEDLL(unsigned char *exeStart,
                            IMAGE_IMPORT_DESCRIPTOR *import_desc);
#endif

static int exeloadLoadMVS(char **entry_point, FILE *fp, char **loadloc);
static int exeloadLoadAmiga(char **entry_point, FILE *fp, char **loadloc);
static int fixPE(char *buf, int *len, int *entry, int rlad);
static int processRLD(char *buf, int rlad, char *rld, int len);

int exeloadDoload(char **entry_point, char *progname, char **loadloc)
{
    FILE *fp;
    int ret = 1;

    fp = fopen(progname, "rb");
    if (fp == NULL)
    {
        printf("Failed to open %s for loading\n", progname);
        return (1);
    }
    /* Tries to load the executable as different formats.
     * Returned 0 means the executable was loaded successfully.
     * 1 means it is not the format the function loads.
     * 2 means correct format, but error occured. */
#if 1
    ret = exeloadLoadAOUT(entry_point, fp, loadloc);
    if (ret == 1) ret = exeloadLoadMVS(entry_point, fp, loadloc);
#endif
    if (ret == 1) ret = exeloadLoadAmiga(entry_point, fp, loadloc);
#if 0
    if (ret == 1) ret = exeloadLoadELF(entry_point, fhandle);
    if (ret == 1) ret = exeloadLoadMZ(entry_point, fhandle);
#endif
    fclose(fp);
    if (ret != 0)
    {
        return (1);
    }

    return (0);
}

static int exeloadLoadAOUT(char **entry_point, FILE *fp, char **loadloc)
{
    struct exec firstbit;
    unsigned long exeLen;
    unsigned char *exeStart;
    unsigned char *bss;
    long newpos;
    size_t readbytes;

    if ((fseek(fp, 0, SEEK_SET) != 0)
        || (fread(&firstbit, sizeof(firstbit), 1, fp) != 1))
    {
        return (1);
    }
    /* ZMAGIC and NMAGIC are currently not supported
     * as they should be loaded at 0x10000,
     * but the kernel reserves the first 4 MiB. */
    if ((firstbit.a_info & 0xffff) == ZMAGIC)
    {
        printf("a.out ZMAGIC is not supported\n");
        return (2);
    }
    else if ((firstbit.a_info & 0xffff) == NMAGIC)
    {
        printf("a.out NMAGIC is not supported\n");
        return (2);
    }
    else if ((firstbit.a_info & 0xffff) == QMAGIC)
    {
        printf("a.out QMAGIC is not supported\n");
        return (2);
    }
    else if ((firstbit.a_info & 0xffff) != OMAGIC)
    {
        /* The file is not A.OUT. */
        return (1);
    }
    exeLen = firstbit.a_text + firstbit.a_data + firstbit.a_bss;
    if (*loadloc != NULL)
    {
        exeStart = *loadloc;
    }
    else
    {
        exeStart = malloc(exeLen);
        if (exeStart == NULL)
        {
            printf("Insufficient memory to load A.OUT program\n");
            return (2);
        }
    }

    printf("loading %lu bytes of text\n", firstbit.a_text);
    if (fread(exeStart, firstbit.a_text, 1, fp) != 1)
    {
        printf("Error occured while reading A.OUT text\n");
        return (2);
    }
    if (firstbit.a_data != 0)
    {
        printf("loading %lu bytes of data\n", firstbit.a_data);
        if (fread(exeStart + firstbit.a_text,
                  firstbit.a_data,
                  1,
                  fp) != 1)
        {
            printf("Error occured while reading A.OUT data\n");
            return (2);
        }
    }

    /* initialise BSS */
    bss = exeStart + firstbit.a_text + firstbit.a_data;
    memset(bss, '\0', firstbit.a_bss);
    /* Relocations. */
    if (1)
    {
        unsigned int *corrections;
        unsigned int i;
        unsigned int offs;
        unsigned int type;
        unsigned int zapdata;
        unsigned char *zap;

        zap = exeStart;
        zapdata = (unsigned int)zap;
        if (firstbit.a_trsize != 0)
        {
            corrections = malloc(firstbit.a_trsize);
            if (corrections == NULL)
            {
                printf("insufficient memory %lu\n", firstbit.a_trsize);
                return (2);
            }
            if (fread(corrections, firstbit.a_trsize, 1, fp) != 1)
            {
                printf("Error occured while reading A.OUT text relocations\n");
                free(corrections);
                return (2);
            }
            for (i = 0; i < firstbit.a_trsize / 4; i += 2)
            {
                offs = corrections[i];
                type = corrections[i + 1];
                if (((type >> 24) & 0xff) != 0x04)
                {
                    continue;
                }
                *(unsigned int *)(zap + offs) += zapdata;
            }
            free(corrections);
        }
        if (firstbit.a_drsize != 0)
        {
            corrections = malloc(firstbit.a_drsize);
            if (corrections == NULL)
            {
                printf("insufficient memory %lu\n", firstbit.a_drsize);
                return (2);
            }
            if (fread(corrections, firstbit.a_drsize, 1, fp) != 1)
            {
                printf("Error occured while reading A.OUT data relocations\n");
                free(corrections);
                return (2);
            }
            zap = exeStart + firstbit.a_text;
            for (i = 0; i < firstbit.a_drsize / 4; i += 2)
            {
                offs = corrections[i];
                type = corrections[i + 1];
                if (((type >> 24) & 0xff) != 0x04)
                {
                    continue;
                }
                *(unsigned int *)(zap + offs) += zapdata;
            }
            free(corrections);
        }
    }

    *entry_point = exeStart + firstbit.a_entry;
    if (*loadloc == NULL)
    {
        *loadloc = exeStart;
    }

    return (0);
}

static int exeloadLoadMVS(char **entry_point, FILE *fp, char **loadloc)
{
    /* struct exec firstbit; */
    unsigned long exeLen;
    unsigned char *exeStart;
    long newpos;
    size_t readbytes;
    int entry;

    printf("in LoadMVS\n");
    rewind(fp);
#if 0
    if ((fseek(fp, 0, SEEK_SET) != 0)
        || (fread(&firstbit, sizeof(firstbit), 1, fp) != 1))
    {
        return (1);
    }
#endif
    readbytes = fread(*loadloc, 1, 1000000, fp);
    printf("read %d bytes\n", (int)readbytes);
    if (fixPE(*loadloc, &readbytes, &entry, (int)*loadloc) != 0)
    {
        return (1);
    }
    *entry_point = (char *)entry; /* fix this !!! */

    return (0);
}

#define getword(p) (p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3])
#define putword(p, val) (p[0] = (val >> 24) & 0xff, \
                         p[1] = (val >> 16) & 0xff, \
                         p[2] = (val >> 8) & 0xff, \
                         p[3] = val & 0xff)

#define HUNK_HEADER 0x3F3
#define HUNK_CODE 0x3E9
#define HUNK_DATA 0x3EA
#define HUNK_RELOC32 0x3EC
#define HUNK_SYMBOL 0x3F0
#define HUNK_END 0x3F2

static int exeloadLoadAmiga(char **entry_point, FILE *fp, char **loadloc)
{
    unsigned long exeLen;
    unsigned char *exeStart;
    long newpos;
    size_t readbytes;
    int entry;
    unsigned char *p;
    unsigned long hunkid;
    int havedata; /* are we expecting a data section? */
    unsigned long temp;
    int datahunk = 1;
    int codehunk = 0;
    int bsshunk;
    unsigned char *beg;
    unsigned char *bssstart;
    unsigned char *codestart;
    unsigned char *datastart;
    unsigned char *reloc;

    printf("in LoadAmiga\n");
    p = beg = *loadloc;
    rewind(fp);
    readbytes = fread(p, 1, 1000000, fp);
    printf("read %d bytes\n", (int)readbytes);
    if (readbytes < 4)
    {
        return (1);
    }
    bssstart = p + readbytes;
    hunkid = getword(p);
    if (hunkid != HUNK_HEADER)
    {
        return (1);
    }
    p += 4;
    temp = getword(p);
    if (temp != 0)
    {
        printf("can't handle 1\n");
        return (1);
    }
    p += 4;
    temp = getword(p);
    if (temp == 2)
    {
        printf("no data section\n");
        havedata = 0;
        bsshunk = 1;
        datahunk = -1;
        printf("bsshunk determined to be %d\n", bsshunk);
        p += 8;
    }
    else if (temp == 3)
    {
        /* looks like this:
        000000  000003F3 00000000 00000003 00000000  ................
        000010  00000002 0000002A 00000001 00000002  .......*........

        So that 3 is the number of hunks, or highest hunk number
        plus 1. Then comes 0, the first hunk number. Then comes
        2, the last hunk number, then comes x'2A', the length of
        hunk number 0 (CODE), then comes 1, the length of
        hunk number 1 (DATA), then comes 2, the length of hunk
        number 2 (BSS since we have a data section, otherwise
        it would have been 1). Lengths are the number of 4-byte
        words.
        */
        printf("we have a data section\n");
        havedata = 1;
        bsshunk = 2;
        printf("bsshunk determined to be %d\n", bsshunk);
        p += 12;
    }
    else
    {
        printf("can't handle 2\n");
        return (1);
    }
    p += 12;
    hunkid = getword(p);
    if (hunkid != HUNK_CODE)
    {
        printf("expecting code at hex %lx\n", (unsigned long)(p - beg));
        return (1);
    }
    p += 4;
    temp = getword(p);
    printf("length of code is hex %lx\n", temp);
    p += 4;
    codestart = p;
    printf("at codestart is %02X\n", *(unsigned char *)codestart);
    p += temp * 4;
    temp = getword(p);
    if (temp != HUNK_RELOC32)
    {
        printf("expecting relocations at hex %lx\n", (unsigned long)(p - beg));
        return (1);
    }
    p += 4;
    reloc = p;
    if (havedata)
    {
        /* skip relocations for now, come back later */
        temp = getword(p);
        while (temp != 0)
        {
            p += 4; /* skip count of relocations */
            p += 4; /* skip hunk number */
            p += temp * 4; /* skip relocations */
            temp = getword(p);
        }
        printf("skipped relocations\n");
        p += 4;
        /* now look for either symbols or data section */
        /* or hunk end */
        /* stop when we find data section */
        while (1)
        {
            temp = getword(p);
            if (temp == HUNK_DATA)
            {
                p += 8;
                datastart = p;
                printf("got data start for real\n");
                break;
            }
            if (temp == HUNK_SYMBOL)
            {
                printf("got symbol\n");
                p += 4; /* skip hunk id */
                temp = getword(p);
                while (temp != 0) /* until hunk number is 0 */
                {
                    long len;
                    long rem;

                    p += 4; /* skip hunk number */
                    printf("symbol %s found\n", p);
                    len = strlen(p);
                    len++;
                    rem = len % 4;
                    if (rem != 0)
                    {
                        len = len - rem + 4; /* move up to 4-byte boundary */
                    }
                    p += len;
                    p += 4; /* skip offset */
                    temp = getword(p);
                }
                p += 4; /* skip hunk number */
            }
            else if (temp == HUNK_END)
            {
                printf("got hunk end\n");
                p += 4;
            }
            else
            {
                printf("unexpected hunk id hex %lx\n", temp);
                return (1);
            }
        }
    } /* have data */
    
    /* apply relocations */
    printf("applying relocations\n");
    p = reloc;
    temp = getword(p);
    while (temp != 0)
    {
        unsigned long count;
        unsigned long hunk_nbr;
        unsigned char *correction;
        unsigned long x;

        count = temp;
        p += 4; /* skip count of relocations */
        hunk_nbr = getword(p);
        p += 4; /* skip hunk number */
        if (hunk_nbr == 0)
        {
            correction = codestart;
            printf("got %lu CODE relocations\n", count);
/*            correction = bssstart;
            printf("got %lu BSS relocations\n", count); */
        }
        else if (hunk_nbr == codehunk)
        {
            correction = codestart;
            printf("got %lu CODE relocations\n", count);
        }
        else if (hunk_nbr == datahunk)
        {
            correction = datastart;
            printf("got %lu DATA relocations\n", count);
        }
        else if (hunk_nbr == bsshunk)
        {
            correction = bssstart;
            printf("got %lu BSS relocations\n", count);
        }
        else
        {
            printf("got unexpected hunk number hex %lx\n", hunk_nbr);
            printf("code hunk is %d\n", codehunk);
            printf("data hunk is %d\n", datahunk);
            printf("bss hunk is %d\n", bsshunk);
            return (1);
        }
        /* Note that all relocations are applied to the code
           section, but the code needs different corrections
           depending on whether it is referencing another location
           in code, a data reference, e.g. a global variable with a
           value of 5, or a BSS reference, e.g. an uninitialized
           global variable. */
        /* At the various offsets in the code there will be values,
           such as 48, which are the correct offsets if the module
           was loaded at location 0 in memory, and also the data
           section started at location 0 in memory, and also the
           bss section (variables that need to be set to 0 before
           the executable runs) is set to location 0 in memory.
           Note that the BSS takes up no space in the executable
           because it would just be a whole lot of zeros serving
           no purpose. So the onus is on the program loading the
           executable to initialize that area. That area is
           normally placed at the end of the executable, but you
           can do a separate malloc and put it anywhere if you
           want. You can also put the data section anywhere you
           want to, with a separate malloc, so long as you actually
           copy all that data across to the new location. This is
           what needs to happen with reentrant modules. There needs
           to be a register (such as ds: in 8086) pointing to where
           the global variables are. And normally the BSS comes
           after that, so you can use a single register to point to
           both data and BSS. The code corrections only need to be
           done once for the code section itself, and the data
           offsets are all given in reference to some dedicated
           register (such as ds) so that no more corrections are
           required to the code to reference the correct offsets.
           In fact, you don't need to correct them at all, they are
           all set to the correct offsets for the dedicated
           register. But if you're not using a dedicated register,
           or you are using the huge, large or compact memory models
           of the 8086, you can't use a dedicated register, as there
           is too much data (more than 64k) for ds to reference, so
           ds keeps on being reloaded, and thus the code corrections
           need to be made to point to the full address
           (segment:offset) of where the variable can be found.
           For the 68000 you also need the full address (as the
           compiler is not using a dedicated register and dynamically
           providing the offsets), but it is a flat 32-bit pointer.
           If yu are using a compiler that DOES use a dedicated
           register, then there will simply be no "data corrections"
           found in the relocation table, ie the relocation table
           will only have references to locations in the code where
           somewhere else in the code needs to be referenced (e.g.
           to call a function) but because the code is not loaded
           at location 0 (or effectively done so via virtual memory,
           or some other fixed location like Windows used to use),
           so it needs to know where exactly in memory it has been
           loaded. */

        for (x = 0; x < count; x++)
        {
            unsigned char *zaploc;
            unsigned char *xxx;
            unsigned long value;
            unsigned long oldref;
            unsigned char *properref;
            unsigned long properref_val;

            xxx = &p[x * 4]; /* point to a value, such as 12,
                which is where in the code section will need
                to be adjusted */
            value = getword(xxx); /* now we have that value 12 */
            zaploc = codestart + value; /* now we point to the
                exact location in the code section that needs
                to have a correction applied. Note that the
                corrected value to be applied depends on whether
                it is a code, data or BSS reference, but we have
                already determined that, and are already pointing
                to the relevant memory address. */
            oldref = getword(zaploc); /* now we have the old
                value of the function or whatever, e.g. if the
                called function is at offset 40 in the code
                section, then it will have the value 40. */
            properref = correction + oldref; /* This is where we
                want the code to start looking for this code, data
                or BSS reference. Even if we are on a 64-bit system
                and we are loading this 32-bit module, someone
                else will have made sure that we are effectively
                on a 4 GiB boundary so that this pointer will be
                valid, even though we're just about to truncate
                it to 4 bytes. ie on a 64-bit system, properref
                could be 0x1 0000 0000 - but that's OK so long
                as the high 32 bits of all registers have been
                set to 0x1 0000 0000. None of this 68000 code
                knows about the high 32-bits of 64-bit registers
                so will not disturb any data stored there. */
            properref_val = (unsigned long)properref; /* this
                probably won't truncate the pointer, but we are
                putting it in integer form, ready for manipulation.
                Note that if this program is running on a 8086,
                and you are trying to load this 68000 executable
                (prior to switching to the 68000 coprocessor),
                using far data pointers, you will need to use a
                compiler that converts far pointers into flat
                references when converting from pointer to long,
                because
                the 68000 program will require a flat reference.
                ie the compiler needs to do the equivalent of
                ((properref >> 4) << 16) | (properref & 0xffff)
                in order to create properref_val. Or you need
                to provide your own routine to do that. In my
                opinion this is the job of the compiler, it
                should know that you are wanting a flat reference.
                So too when you go:
                char *p = (char *)0xb8000UL;
                it should convert that into b800:0000 itself.
                Speak to your compiler vendor!
                If we were doing things the other way around, ie
                you have a 68000 loading an 8086 large memory model
                program, then this is a "known quantity" (the same
                as loading a small memory model program) and it is
                this loader code (perhaps compiled with a 68000)
                that would need to convert a flat memory pointer (ie
                where it loaded the 8086 program into 68000 memory)
                that would need to convert a flat pointer into a
                segmented pointer suitable for the 8086 prior to
                switching to the 8086 coprocessor. So it is reasonable
                for you to do the bit shifting in your own code. After
                first of all converting into an unsigned long, because
                maybe the loader is not 68000 either, but another
                segmented architecture with different segmentation rules,
                and it too needs to convert to a flat address (as
                represented by unsigned long) prior to being manually
                converted into an address suitable for the 8086. */
                
            putword(zaploc, properref_val);
        }
        p += temp * 4; /* skip relocations */
        temp = getword(p);
    }
    p += 4; /* get past the relocations and on to the next hunk ID,
        even though currently we have no further need to look at hunks. */
    printf("finished relocations\n");

    *entry_point = codestart;
    printf("just set entry point, first byte %02X\n",
           *(unsigned char *)codestart);

    return (0);
}

#if 0
static int exeloadLoadELF(unsigned long *entry_point, int fhandle)
{
    int doing_elf_rel = 0;
    int doing_elf_exec = 0;
    Elf32_Ehdr *elfHdr;
    Elf32_Phdr *program_table = NULL;
    Elf32_Phdr *segment;
    Elf32_Shdr *section_table = NULL;
    Elf32_Shdr *section;
    unsigned char *elf_other_sections;
    Elf32_Addr lowest_p_vaddr = 0;
    Elf32_Word lowest_segment_align = 0;
    unsigned char *exeStart;
    unsigned char *bss;
    unsigned long exeLen;
    unsigned char firstbit[4];
    long newpos;
    size_t readbytes;

    if (PosMoveFilePointer(fhandle, 0, SEEK_SET, &newpos)
        || PosReadFile(fhandle, firstbit, sizeof(firstbit), &readbytes)
        || (readbytes != sizeof(firstbit))
        || (memcmp((&firstbit), "\x7f""ELF", 4) != 0))
    {
        return (1);
    }
    {
        int elf_invalid = 0;

        /* Loads entire ELF header into memory. */
        elfHdr = kmalloc(sizeof(Elf32_Ehdr));
        if (elfHdr == NULL)
        {
            printf("Insufficient memory for ELF header\n");
            return (2);
        }
        if (PosMoveFilePointer(fhandle, 0, SEEK_SET, &newpos)
            || PosReadFile(fhandle, elfHdr, sizeof(Elf32_Ehdr), &readbytes)
            || (readbytes != sizeof(Elf32_Ehdr)))
        {
            printf("Error occured while reading ELF header\n");
            kfree(elfHdr);
            return (2);
        }

        /* Checks e_ident if the program can be used on PDOS-32. */
        if (elfHdr->e_ident[EI_CLASS] != ELFCLASS32)
        {
            if (elfHdr->e_ident[EI_CLASS] == ELFCLASS64)
            {
                printf("64-bit ELF is not supported\n");
            }
            else if (elfHdr->e_ident[EI_CLASS] == ELFCLASSNONE)
            {
                printf("Invalid ELF class\n");
            }
            else
            {
                printf("Unknown ELF class: %u\n", elfHdr->e_ident[EI_CLASS]);
            }
            elf_invalid = 1;
        }
        if (elfHdr->e_ident[EI_DATA] != ELFDATA2LSB)
        {
            if (elfHdr->e_ident[EI_DATA] == ELFDATA2MSB)
            {
                printf("Big-endian ELF encoding is not supported\n");
            }
            else if (elfHdr->e_ident[EI_DATA] == ELFDATANONE)
            {
                printf("Invalid ELF data encoding\n");
            }
            else
            {
                printf("Unknown ELF data encoding: %u\n",
                       elfHdr->e_ident[EI_DATA]);
            }
            elf_invalid = 1;
        }
        if (elfHdr->e_ident[EI_OSABI] != ELFOSABI_NONE)
        {
            printf("No OS or ABI specific extensions for ELF supported\n");
            elf_invalid = 1;
        }
        /* Checks other parts of the header if the file can be loaded. */
        if (elfHdr->e_type == ET_REL)
        {
            doing_elf_rel = 1;
        }
        else if (elfHdr->e_type == ET_EXEC)
        {
            doing_elf_exec = 1;
        }
        else
        {
            printf("Only ELF relocatable and executable "
                   "files are supported\n");
            elf_invalid = 1;
        }
        if (elfHdr->e_machine != EM_386)
        {
            printf("Only Intel 386 architecture is supported\n");
            elf_invalid = 1;
        }
        if (doing_elf_exec)
        {
            if (elfHdr->e_phoff == 0 || elfHdr->e_phnum == 0)
            {
                printf("Executable file is missing Program Header Table\n");
                elf_invalid = 1;
            }
            if (elfHdr->e_phnum >= SHN_LORESERVE)
            {
                printf("Reserved indexes for e_phnum are not supported\n");
                printf("e_phnum is %04x\n", elfHdr->e_phnum);
                elf_invalid = 1;
            }
            if (elfHdr->e_phentsize != sizeof(Elf32_Phdr))
            {
                printf("Program Header Table entries have unsupported size\n");
                printf("e_phentsize: %u supported size: %lu\n",
                       elfHdr->e_phentsize, sizeof(Elf32_Phdr));
                elf_invalid = 1;
            }
        }
        else if (doing_elf_rel)
        {
            if (elfHdr->e_shoff == 0 || elfHdr->e_shnum == 0)
            {
                printf("Relocatable file is missing Section Header Table\n");
                elf_invalid = 1;
            }
            if (elfHdr->e_shnum >= SHN_LORESERVE)
            {
                printf("Reserved indexes for e_shnum are not supported\n");
                printf("e_shnum is %04x\n", elfHdr->e_shnum);
                elf_invalid = 1;
            }
            if (elfHdr->e_shentsize != sizeof(Elf32_Shdr))
            {
                printf("Section Header Table entries have unsupported size\n");
                printf("e_shentsize: %u supported size: %lu\n",
                       elfHdr->e_shentsize, sizeof(Elf32_Shdr));
                elf_invalid = 1;
            }
        }
        if (elf_invalid)
        {
            /* All problems with ELF header are reported
             * and loading is stopped. */
            printf("This ELF file cannot be loaded\n");
            kfree(elfHdr);
            return (2);
        }
        /* Loads Program Header Table if it is present. */
        if (!(elfHdr->e_phoff == 0 || elfHdr->e_phnum == 0))
        {
            program_table = kmalloc(elfHdr->e_phnum * elfHdr->e_phentsize);
            if (program_table == NULL)
            {
                printf("Insufficient memory for ELF Program Header Table\n");
                kfree(elfHdr);
                return (2);
            }
            if (PosMoveFilePointer(fhandle, elfHdr->e_phoff, SEEK_SET, &newpos)
                || PosReadFile(fhandle, program_table,
                               elfHdr->e_phnum * elfHdr->e_phentsize,
                               &readbytes)
                || (readbytes != (elfHdr->e_phnum * elfHdr->e_phentsize)))
            {
                printf("Error occured while reading "
                       "ELF Program Header Table\n");
                kfree(elfHdr);
                kfree(program_table);
                return (2);
            }
        }
        /* Loads Section Header Table if it is present. */
        if (!(elfHdr->e_shoff == 0 || elfHdr->e_shnum == 0))
        {
            section_table = kmalloc(elfHdr->e_shnum * elfHdr->e_shentsize);
            if (section_table == NULL)
            {
                printf("Insufficient memory for ELF Section Header Table\n");
                kfree(elfHdr);
                return (2);
            }
            if (PosMoveFilePointer(fhandle, elfHdr->e_shoff, SEEK_SET, &newpos)
                || PosReadFile(fhandle, section_table,
                               elfHdr->e_shnum * elfHdr->e_shentsize,
                               &readbytes)
                || (readbytes != (elfHdr->e_shnum * elfHdr->e_shentsize)))
            {
                printf("Error occured while reading "
                       "ELF Section Header Table\n");
                kfree(elfHdr);
                kfree(section_table);
                return (2);
            }
        }
    }

    if (doing_elf_rel || doing_elf_exec)
    {
        /* Calculates how much memory is needed
         * and allocates memory for sections used only for loading. */
        unsigned long otherLen = 0;

        exeLen = 0;
        if (doing_elf_exec)
        {
            Elf32_Addr highest_p_vaddr = 0;
            Elf32_Word highest_segment_memsz = 0;

            for (segment = program_table;
                 segment < program_table + elfHdr->e_phnum;
                 segment++)
            {
                if (segment->p_type == PT_LOAD)
                {
                    if (!lowest_p_vaddr || lowest_p_vaddr > segment->p_vaddr)
                    {
                        lowest_p_vaddr = segment->p_vaddr;
                        lowest_segment_align = segment->p_align;
                    }
                    if (highest_p_vaddr < segment->p_vaddr)
                    {
                        highest_p_vaddr = segment->p_vaddr;
                        highest_segment_memsz = segment->p_memsz;
                    }
                }
            }
            exeLen = highest_p_vaddr - lowest_p_vaddr + highest_segment_memsz;
            if (lowest_segment_align > 1)
            {
                /* Ensures alignment of the lowest segment.
                 * 0 and 1 mean no alignment restrictions. */
                exeLen += lowest_segment_align;
            }
        }
        if (section_table)
        {
            for (section = section_table;
                 section < section_table + elfHdr->e_shnum;
                 section++)
            {
                unsigned long section_size = section->sh_size;
                if (section->sh_addralign > 1)
                {
                    /* Some sections must be aligned
                     * on sh_addralign byte boundaries.
                     * 0 and 1 mean no alignment restrictions. */
                    section_size += section->sh_addralign;
                }
                if (section->sh_flags & SHF_ALLOC)
                {
                    /* Section is needed while the program is running,
                     * but if we are loading an executable file,
                     * the memory is already counted
                     * using Program Header Table. */
                    if (doing_elf_exec) continue;
                    exeLen += section_size;
                }
                else
                {
                    /* Section is used only for loading. */
                   otherLen += section_size;
                }
            }
            elf_other_sections = kmalloc(otherLen);
            if (elf_other_sections == NULL)
            {
                printf("Insufficient memory to load ELF sections\n");
                kfree(elfHdr);
                kfree(section_table);
                return (2);
            }
        }
    }
    /* Allocates memory for the process. */
    if (doing_elf_exec)
    {
        exeStart = PosVirtualAlloc((void *)lowest_p_vaddr, exeLen);
    }
    else if (doing_elf_rel)
    {
        exeStart = PosVirtualAlloc(0, exeLen);
    }
    if (exeStart == NULL)
    {
        printf("Insufficient memory to load ELF program\n");
        kfree(elfHdr);
        if (program_table) kfree(program_table);
        if (section_table)
        {
            kfree(section_table);
            kfree(elf_other_sections);
        }
        return (2);
    }

    if (doing_elf_rel || doing_elf_exec)
    {
        /* Loads all sections of ELF file with proper alignment,
         * clears all SHT_NOBITS sections and stores the addresses
         * in sh_addr of each section.
         * bss is set now too. */
        unsigned char *exe_addr = exeStart;
        unsigned char *other_addr = elf_other_sections;

        bss = 0;
        if (doing_elf_exec)
        {
            /* Aligns the exeStart on lowest segment alignment boundary. */
            /*exeStart = (unsigned char *)((((unsigned long)exeStart
                                           / lowest_segment_align) + 1)
                                         * lowest_segment_align);*/
            /* +++Enable aligning. */
            for (segment = program_table;
                 segment < program_table + elfHdr->e_phnum;
                 segment++)
            {
                if (segment->p_type == PT_LOAD)
                {
                    exe_addr = exeStart + (segment->p_vaddr - lowest_p_vaddr);

                    if (PosMoveFilePointer(fhandle, segment->p_offset,
                                           SEEK_SET, &newpos)
                        || PosReadFile(fhandle, exe_addr, segment->p_filesz,
                                       &readbytes)
                        || (readbytes != (segment->p_filesz)))
                    {
                        printf("Error occured while reading ELF segment\n");
                        kfree(program_table);
                        if (section_table)
                        {
                            kfree(section_table);
                            kfree(elf_other_sections);
                        }
                        return (2);
                    }

                    /* Bytes that are not present in file,
                     * but must be present in memory must be set to 0. */
                    if (segment->p_filesz < segment->p_memsz)
                    {
                        bss = exe_addr + (segment->p_filesz);
                        memset(bss, '\0',
                               segment->p_memsz - segment->p_filesz);
                    }
                }
            }
        }

        for (section = section_table;
             section < section_table + elfHdr->e_shnum;
             section++)
        {
            if (section->sh_flags & SHF_ALLOC)
            {
                /* If we are loading executable file,
                 * SHF_ALLOC sections are already loaded in segments. */
                if (doing_elf_exec) continue;
                if (section->sh_addralign > 1)
                {
                    exe_addr = (void *)((((unsigned long)exe_addr
                                          / (section->sh_addralign)) + 1)
                                        * (section->sh_addralign));
                }
                if (section->sh_type != SHT_NOBITS)
                {
                    if (PosMoveFilePointer(fhandle, section->sh_offset,
                                           SEEK_SET, &newpos)
                        || PosReadFile(fhandle, exe_addr, section->sh_size,
                                       &readbytes)
                        || (readbytes != (section->sh_size)))
                    {
                        printf("Error occured while reading ELF section\n");
                        kfree(elfHdr);
                        if (program_table) kfree(program_table);
                        kfree(section_table);
                        kfree(elf_other_sections);
                        return (2);
                    }
                }
                else
                {
                    /* The section is probably BSS. */
                    if (bss != 0)
                    {
                        printf("Multiple SHT_NOBITS with SHF_ALLOC "
                               "present in ELF file\n");
                    }
                    bss = exe_addr;
                    /* All SHT_NOBITS should be cleared to 0. */
                    memset(bss, '\0', section->sh_size);
                }
                /* sh_addr is 0 in relocatable files,
                 * so we can use it to store the real address. */
                section->sh_addr = (Elf32_Addr)exe_addr;
                exe_addr += section->sh_size;
            }
            else
            {
                if (section->sh_addralign > 1)
                {
                    other_addr = (void *)((((unsigned long)other_addr
                                            / (section->sh_addralign)) + 1)
                                          * (section->sh_addralign));
                }
                if (section->sh_type != SHT_NOBITS)
                {
                    if (PosMoveFilePointer(fhandle, section->sh_offset,
                                           SEEK_SET, &newpos)
                        || PosReadFile(fhandle, other_addr, section->sh_size,
                                       &readbytes)
                        || (readbytes != (section->sh_size)))
                    {
                        printf("Error occured while reading ELF section\n");
                        kfree(elfHdr);
                        if (program_table) kfree(program_table);
                        kfree(section_table);
                        kfree(elf_other_sections);
                        return (2);
                    }
                }
                else
                {
                    /* All SHT_NOBITS should be cleared to 0. */
                    memset(other_addr, '\0', section->sh_size);
                }
                /* sh_addr is 0 in relocatable files,
                 * so we can use it to store the real address. */
                section->sh_addr = (Elf32_Addr)other_addr;
                other_addr += section->sh_size;
            }
        }
    }
    /* Program was successfully loaded from the file,
     * no more errors can occur. */
    PosCloseFile(fhandle);

    /* Relocations. */
    if (doing_elf_rel)
    {
        for (section = section_table;
             section < section_table + elfHdr->e_shnum;
             section++)
        {
            if (section->sh_type == SHT_RELA)
            {
                printf("ELF Relocations with explicit addend "
                       "are not supported\n");
            }
            else if (section->sh_type == SHT_REL)
            {
                /* sh_link specifies the symbol table
                 * and sh_info section being modified. */
                Elf32_Sym *sym_table = (Elf32_Sym *)(section_table
                                        + (section->sh_link))->sh_addr;
                unsigned char *target_base = (unsigned char *)(section_table
                                             + (section->sh_info))->sh_addr;
                Elf32_Rel *startrel = (Elf32_Rel *)section->sh_addr;
                Elf32_Rel *currel;

                if (section->sh_entsize != sizeof(Elf32_Rel))
                {
                    printf("Invalid size of relocation entries in ELF file\n");
                    continue;
                }

                for (currel = startrel;
                     currel < (startrel
                               + ((section->sh_size) / (section->sh_entsize)));
                     currel++)
                {
                    long *target = (long *)(target_base + currel->r_offset);
                    Elf32_Sym *symbol = (sym_table
                                         + ELF32_R_SYM(currel->r_info));
                    Elf32_Addr sym_value = 0;

                    if (ELF32_R_SYM(currel->r_info) != STN_UNDEF)
                    {
                        if (symbol->st_shndx == SHN_ABS)
                        {
                            /* Absolute symbol, stores absolute value. */
                            sym_value = symbol->st_value;
                        }
                        else if (symbol->st_shndx == SHN_UNDEF)
                        {
                            /* Dynamic linker should fill this symbol. */
                            printf("Undefined symbol in ELF file\n");
                            continue;
                        }
                        else if (symbol->st_shndx == SHN_XINDEX)
                        {
                            printf("Unsupported value in ELF symbol\n");
                            printf("symbol->st_shndx: %x\n", symbol->st_shndx);
                            continue;
                        }
                        else
                        {
                            /* Internal symbol. Must be converted
                             * to absolute symbol.*/
                            sym_value = symbol->st_value;
                            /* Adds the address of the related section
                             * so the symbol stores absolute address. */
                            sym_value += ((section_table
                                           + symbol->st_shndx)->sh_addr);
                        }
                    }
                    switch (ELF32_R_TYPE(currel->r_info))
                    {
                        case R_386_NONE:
                            break;
                        case R_386_32:
                            /* Symbol value + offset. */
                            *target = sym_value + *target;
                            break;
                        case R_386_PC32:
                            /* Symbol value + offset - absolute address
                             * of the modified field. */
                            *target = (sym_value + (*target)
                                       - (unsigned long)target);
                            break;
                        default:
                            printf("Unknown relocation type in ELF file\n");
                    }
                }
            }
        }
    }
    else if (doing_elf_exec)
    {
        /* +++Implement relocations for ELF Executables. */
        ;
    }

    if (doing_elf_rel)
    {
        *entry_point = ((unsigned long)exeStart) + (elfHdr->e_entry);
    }
    else if (doing_elf_exec)
    {
        *entry_point = elfHdr->e_entry;
    }

    /* Frees memory not needed by the process. */
    kfree(elfHdr);
    if (program_table) kfree(program_table);
    if (section_table)
    {
        kfree(section_table);
        kfree(elf_other_sections);
    }

    return (0);
}

static int exeloadLoadMZ(unsigned long *entry_point, int fhandle)
{
    Mz_hdr firstbit;
    long newpos;
    size_t readbytes;

    /* The header size is in paragraphs,
     * so the smallest possible header is 16 bytes (paragraph) long.
     * Next is the magic number checked. */
    if (PosMoveFilePointer(fhandle, 0, SEEK_SET, &newpos)
        || PosReadFile(fhandle, &firstbit, 16, &readbytes)
        || (readbytes != 16)
        || (memcmp(firstbit.magic, "MZ", 2) != 0
            && memcmp(&firstbit.magic, "ZM", 2) != 0))
    {
        return (1);
    }
    if (firstbit.header_size == 0)
    {
        printf("MZ Header has 0 size\n");
        return (2);
    }
    if (firstbit.header_size * 16 > sizeof(firstbit))
    {
        printf("MZ Header is too large, size: %u\n",
               firstbit.header_size * 16);
        return (2);
    }
    /* Loads the rest of the header. */
    if (PosReadFile(fhandle, ((char *)&firstbit) + 16,
                    (firstbit.header_size - 1) * 16, &readbytes)
        || (readbytes != (firstbit.header_size - 1) * 16))
    {
        printf("Error occured while reading MZ header\n");
        return (2);
    }
    /* Determines whether the executable has extensions or is a pure MZ.
     * Extensions are at offset in e_lfanew,
     * so the header must have at least 4 paragraphs. */
    if ((firstbit.header_size >= 4)
        && (firstbit.e_lfanew != 0))
    {
        int ret;

        /* Same logic as in exeloadDoload(). */
        ret = exeloadLoadPE(entry_point, fhandle, firstbit.e_lfanew);
        if (ret == 1)
            ret = exeloadLoadLX(entry_point, fhandle, firstbit.e_lfanew);
        if (ret == 1)
            ret = exeloadLoadNE(entry_point, fhandle, firstbit.e_lfanew);
        if (ret == 1)
            printf("Unknown MZ extension\n");
        return (ret);
    }
    /* Pure MZ executables are for 16-bit DOS, so we cannot run them. */
    printf("Pure MZ executables are not supported\n");

    return (2);
}

static int exeloadLoadPE(unsigned long *entry_point,
                         int fhandle,
                         unsigned long e_lfanew)
{
    Coff_hdr coff_hdr;
    Pe32_optional_hdr *optional_hdr;
    Coff_section *section_table, *section;
    unsigned char *exeStart;
    long newpos;
    size_t readbytes;

    {
        unsigned char firstbit[4];

        if (PosMoveFilePointer(fhandle, e_lfanew, SEEK_SET, &newpos)
            || PosReadFile(fhandle, firstbit, 4, &readbytes)
            || (readbytes != 4)
            || (memcmp(firstbit, "PE\0\0", 4) != 0))
        {
            return (1);
        }
    }
    if (PosReadFile(fhandle, &coff_hdr, sizeof(coff_hdr), &readbytes)
        || (readbytes != sizeof(coff_hdr)))
    {
        printf("Error occured while reading COFF header\n");
        return (2);
    }
    if ((coff_hdr.Machine != IMAGE_FILE_MACHINE_UNKNOWN)
        && (coff_hdr.Machine != IMAGE_FILE_MACHINE_I386))
    {
        if (coff_hdr.Machine == IMAGE_FILE_MACHINE_AMD64)
        {
            printf("64-bit PE programs are not supported\n");
        }
        else
        {
            printf("Unknown PE machine type: %04x\n", coff_hdr.Machine);
        }
        return (2);
    }

    optional_hdr = kmalloc(coff_hdr.SizeOfOptionalHeader);
    if (optional_hdr == NULL)
    {
        printf("Insufficient memory to load PE optional header\n");
        return (2);
    }
    if (PosReadFile(fhandle, optional_hdr, coff_hdr.SizeOfOptionalHeader,
                    &readbytes)
        || (readbytes != (coff_hdr.SizeOfOptionalHeader)))
    {
        printf("Error occured while reading PE optional header\n");
        kfree(optional_hdr);
        return (2);
    }
    if (optional_hdr->Magic != MAGIC_PE32)
    {
        printf("Unknown PE optional header magic: %04x\n",
               optional_hdr->Magic);
        kfree(optional_hdr);
        return (2);
    }

    section_table = kmalloc(coff_hdr.NumberOfSections * sizeof(Coff_section));
    if (section_table == NULL)
    {
        printf("Insufficient memory to load PE section headers\n");
        return (2);
    }
    if (PosReadFile(fhandle, section_table,
                    coff_hdr.NumberOfSections * sizeof(Coff_section),
                    &readbytes)
        || (readbytes != (coff_hdr.NumberOfSections * sizeof(Coff_section))))
    {
        printf("Error occured while reading PE optional header\n");
        kfree(section_table);
        kfree(optional_hdr);
        return (2);
    }

    /* Allocates memory for the process.
     * Size of image is obtained from the optional header. */
    /* PE files are loaded at their preferred address. */
    exeStart = PosVirtualAlloc((void *)(optional_hdr->ImageBase),
                               optional_hdr->SizeOfImage);
    if ((exeStart == NULL)
        && !(coff_hdr.Characteristics & IMAGE_FILE_RELOCS_STRIPPED))
    {
        /* If the PE file could not be loaded at its preferred address,
         * but it is relocatable, it is loaded elsewhere. */
        exeStart = PosVirtualAlloc(0, optional_hdr->SizeOfImage);
    }
    if (exeStart == NULL)
    {
        printf("Insufficient memory to load PE program\n");
        kfree(section_table);
        kfree(optional_hdr);
        return (2);
    }

    /* Loads all sections at their addresses. */
    for (section = section_table;
         section < section_table + (coff_hdr.NumberOfSections);
         section++)
    {
        unsigned long size_in_file;

        /* Virtual size of a section is larger than in file,
         * so the difference is filled with 0. */
        if ((section->VirtualSize) > (section->SizeOfRawData))
        {
            memset((exeStart
                    + (section->VirtualAddress)
                    + (section->SizeOfRawData)),
                   0,
                   (section->VirtualSize) - (section->SizeOfRawData));
            size_in_file = section->SizeOfRawData;
        }
        /* SizeOfRawData is rounded up,
         * so it can be larger than VirtualSize. */
        else
        {
            size_in_file = section->VirtualSize;
        }
        if (PosMoveFilePointer(fhandle, section->PointerToRawData, SEEK_SET,
                               &newpos)
            || PosReadFile(fhandle, exeStart + (section->VirtualAddress),
                           size_in_file, &readbytes)
            || (readbytes != size_in_file))
        {
            printf("Error occured while reading PE section\n");
            kfree(section_table);
            kfree(optional_hdr);
            return (2);
        }
    }
    PosCloseFile(fhandle);

    if (((unsigned long)exeStart) != (optional_hdr->ImageBase))
    {
        /* Relocations are not stripped, so the executable can be relocated. */
        if (optional_hdr->NumberOfRvaAndSizes > DATA_DIRECTORY_REL)
        {
            IMAGE_DATA_DIRECTORY *data_dir = (((IMAGE_DATA_DIRECTORY *)
                                               (optional_hdr + 1))
                                              + DATA_DIRECTORY_REL);
            /* Difference between preferred address and real address. */
            unsigned long image_diff = (optional_hdr->ImageBase
                                        - (unsigned long)exeStart);
            int lower_exeStart = 0;
            Base_relocation_block *rel_block = ((Base_relocation_block *)
                                                (exeStart
                                                 + (data_dir
                                                    ->VirtualAddress)));
            Base_relocation_block *end_rel_block;

            if (optional_hdr->ImageBase > (unsigned long)exeStart)
            {
                /* Image is loaded  at lower address than preferred. */
                lower_exeStart = 1;
            }
            end_rel_block = rel_block + ((data_dir->Size)
                                         / sizeof(Base_relocation_block));

            for (; rel_block < end_rel_block;)
            {
                unsigned short *cur_rel = (unsigned short *)rel_block;
                unsigned short *end_rel;

                end_rel = (cur_rel
                           + ((rel_block->BlockSize)
                              / sizeof(unsigned short)));
                cur_rel = (unsigned short *)(rel_block + 1);

                for (; cur_rel < end_rel; cur_rel++)
                {
                    /* Top 4 bits indicate the type of relocation. */
                    unsigned char rel_type = (*cur_rel) >> 12;
                    void *rel_target = (exeStart + (rel_block->PageRva)
                                        + ((*cur_rel) & 0x0fff));

                    if (rel_type == IMAGE_REL_BASED_ABSOLUTE) continue;
                    if (rel_type == IMAGE_REL_BASED_HIGHLOW)
                    {
                        if (lower_exeStart)
                            (*((unsigned long *)rel_target)) -= image_diff;
                        else (*((unsigned long *)rel_target)) += image_diff;
                    }
                    else
                    {
                        printf("Unknown PE relocation type: %u\n",
                               rel_type);
                    }
                }

                rel_block = (Base_relocation_block *)cur_rel;
            }
        }
    }

    if (optional_hdr->NumberOfRvaAndSizes > DATA_DIRECTORY_IMPORT_TABLE)
    {
        IMAGE_DATA_DIRECTORY *data_dir = (((IMAGE_DATA_DIRECTORY *)
                                           (optional_hdr + 1))
                                          + DATA_DIRECTORY_IMPORT_TABLE);
        if (data_dir->Size != 0)
        {
            IMAGE_IMPORT_DESCRIPTOR *import_desc = ((void *)
                                                    (exeStart
                                                     + (data_dir
                                                        ->VirtualAddress)));

            /* Each descriptor is for one DLL
             * and the array has a null terminator. */
            for (; import_desc->OriginalFirstThunk; import_desc++)
            {
                if (exeloadLoadPEDLL(exeStart, import_desc))
                {
                    printf("Failed to load DLL %s\n",
                           exeStart + (import_desc->Name));
                    return (2);
                }
            }
        }
    }

    *entry_point = (((unsigned long)exeStart)
                    + (optional_hdr->AddressOfEntryPoint));
    /* Frees memory not needed by the process. */
    kfree(section_table);
    kfree(optional_hdr);

    return (0);
}

static int exeloadLoadPEDLL(unsigned char *exeStart,
                            IMAGE_IMPORT_DESCRIPTOR *import_desc)
{
    Mz_hdr mzFirstbit;
    int fhandle;
    Coff_hdr coff_hdr;
    Pe32_optional_hdr *optional_hdr;
    Coff_section *section_table, *section;
    unsigned char *dllStart;
    long newpos;
    size_t readbytes;
    char *name1;
    char *name2;

    /* PE DLL is being loaded,
     * so it is loaded in the same way as PE executable,
     * but the MZ stage is integrated. */
    name1 = exeStart + import_desc->Name;
    if ((strcmp(name1, "kernel32.dll") == 0)
        || (strcmp(name1, "KERNEL32.dll") == 0))
    {
        name2 = "\\KERNEL32.DLL";
    }
    else if (strcmp(name1, "msvcrt.dll") == 0)
    {
        name2 = "\\MSVCRT.DLL";
    }
    else
    {
        name2 = name1;
    }
    if (PosOpenFile(name2, 0, &fhandle))
    {
        printf("Failed to open %s for loading\n",
               exeStart + (import_desc->Name));
        return (1);
    }

    /* The header size is in paragraphs,
     * so the smallest possible header is 16 bytes (paragraph) long.
     * Next is the magic number checked. */
    if (PosMoveFilePointer(fhandle, 0, SEEK_SET, &newpos)
        || PosReadFile(fhandle, &mzFirstbit, 16, &readbytes)
        || (readbytes != 16)
        || (memcmp(mzFirstbit.magic, "MZ", 2) != 0
            && memcmp(&mzFirstbit.magic, "ZM", 2) != 0))
    {
        PosCloseFile(fhandle);
        return (1);
    }
    if (mzFirstbit.header_size == 0)
    {
        printf("MZ Header has 0 size\n");
        PosCloseFile(fhandle);
        return (1);
    }
    if (mzFirstbit.header_size * 16 > sizeof(mzFirstbit))
    {
        printf("MZ Header is too large, size: %u\n",
               mzFirstbit.header_size * 16);
        PosCloseFile(fhandle);
        return (1);
    }
    /* Loads the rest of the header. */
    if (PosReadFile(fhandle, ((char *)&mzFirstbit) + 16,
                    (mzFirstbit.header_size - 1) * 16, &readbytes)
        || (readbytes != (mzFirstbit.header_size - 1) * 16))
    {
        printf("Error occured while reading MZ header\n");
        PosCloseFile(fhandle);
        return (1);
    }
    /* Determines whether the executable has extensions or is a pure MZ.
     * Extensions are at offset in e_lfanew,
     * so the header must have at least 4 paragraphs. */
    if ((mzFirstbit.header_size < 4)
        || (mzFirstbit.e_lfanew == 0))
    {
        PosCloseFile(fhandle);
        return (1);
    }

    /* PE stage begins. */
    {
        unsigned char firstbit[4];

        if (PosMoveFilePointer(fhandle, mzFirstbit.e_lfanew, SEEK_SET, &newpos)
            || PosReadFile(fhandle, firstbit, 4, &readbytes)
            || (readbytes != 4)
            || (memcmp(firstbit, "PE\0\0", 4) != 0))
        {
            PosCloseFile(fhandle);
            return (1);
        }
    }
    if (PosReadFile(fhandle, &coff_hdr, sizeof(coff_hdr), &readbytes)
        || (readbytes != sizeof(coff_hdr)))
    {
        printf("Error occured while reading COFF header\n");
        PosCloseFile(fhandle);
        return (1);
    }
    if ((coff_hdr.Machine != IMAGE_FILE_MACHINE_UNKNOWN)
        && (coff_hdr.Machine != IMAGE_FILE_MACHINE_I386))
    {
        if (coff_hdr.Machine == IMAGE_FILE_MACHINE_AMD64)
        {
            printf("64-bit PE programs are not supported\n");
        }
        else
        {
            printf("Unknown PE machine type: %04x\n", coff_hdr.Machine);
        }
        PosCloseFile(fhandle);
        return (1);
    }

    optional_hdr = kmalloc(coff_hdr.SizeOfOptionalHeader);
    if (optional_hdr == NULL)
    {
        printf("Insufficient memory to load PE optional header\n");
        PosCloseFile(fhandle);
        return (1);
    }
    if (PosReadFile(fhandle, optional_hdr, coff_hdr.SizeOfOptionalHeader,
                    &readbytes)
        || (readbytes != (coff_hdr.SizeOfOptionalHeader)))
    {
        printf("Error occured while reading PE optional header\n");
        kfree(optional_hdr);
        PosCloseFile(fhandle);
        return (1);
    }
    if (optional_hdr->Magic != MAGIC_PE32)
    {
        printf("Unknown PE optional header magic: %04x\n",
               optional_hdr->Magic);
        kfree(optional_hdr);
        PosCloseFile(fhandle);
        return (1);
    }

    section_table = kmalloc(coff_hdr.NumberOfSections * sizeof(Coff_section));
    if (section_table == NULL)
    {
        printf("Insufficient memory to load PE section headers\n");
        return (2);
    }
    if (PosReadFile(fhandle, section_table,
                    coff_hdr.NumberOfSections * sizeof(Coff_section),
                    &readbytes)
        || (readbytes != (coff_hdr.NumberOfSections * sizeof(Coff_section))))
    {
        printf("Error occured while reading PE optional header\n");
        kfree(section_table);
        kfree(optional_hdr);
        PosCloseFile(fhandle);
        return (1);
    }

    /* Allocates memory for the process.
     * Size of image is obtained from the optional header. */
    /* PE files are loaded at their preferred address. */
    dllStart = PosVirtualAlloc((void *)(optional_hdr->ImageBase),
                               optional_hdr->SizeOfImage);
    if ((dllStart == NULL)
        && !(coff_hdr.Characteristics & IMAGE_FILE_RELOCS_STRIPPED))
    {
        /* If the PE file could not be loaded at its preferred address,
         * but it is relocatable, it is loaded elsewhere. */
        dllStart = PosVirtualAlloc(0, optional_hdr->SizeOfImage);
    }
    if (dllStart == NULL)
    {
        printf("Insufficient memory to load PE program\n");
        kfree(section_table);
        kfree(optional_hdr);
        PosCloseFile(fhandle);
        return (1);
    }

    /* Loads all sections at their addresses. */
    for (section = section_table;
         section < section_table + (coff_hdr.NumberOfSections);
         section++)
    {
        unsigned long size_in_file;

        /* Virtual size of a section is larger than in file,
         * so the difference is filled with 0. */
        if ((section->VirtualSize) > (section->SizeOfRawData))
        {
            memset((dllStart
                    + (section->VirtualAddress)
                    + (section->SizeOfRawData)),
                   0,
                   (section->VirtualSize) - (section->SizeOfRawData));
            size_in_file = section->SizeOfRawData;
        }
        /* SizeOfRawData is rounded up,
         * so it can be larger than VirtualSize. */
        else
        {
            size_in_file = section->VirtualSize;
        }
        if (PosMoveFilePointer(fhandle, section->PointerToRawData, SEEK_SET,
                               &newpos)
            || PosReadFile(fhandle, dllStart + (section->VirtualAddress),
                           size_in_file, &readbytes)
            || (readbytes != size_in_file))
        {
            printf("Error occured while reading PE section\n");
            kfree(section_table);
            kfree(optional_hdr);
            PosCloseFile(fhandle);
            return (1);
        }
    }
    PosCloseFile(fhandle);

    if (((unsigned long)dllStart) != (optional_hdr->ImageBase))
    {
        /* Relocations are not stripped, so the executable can be relocated. */
        if (optional_hdr->NumberOfRvaAndSizes > DATA_DIRECTORY_REL)
        {
            IMAGE_DATA_DIRECTORY *data_dir = (((IMAGE_DATA_DIRECTORY *)
                                               (optional_hdr + 1))
                                              + DATA_DIRECTORY_REL);
            /* Difference between preferred address and real address. */
            unsigned long image_diff = (optional_hdr->ImageBase
                                        - (unsigned long)dllStart);
            int lower_dllStart = 0;
            Base_relocation_block *rel_block = ((Base_relocation_block *)
                                                (dllStart
                                                 + (data_dir
                                                    ->VirtualAddress)));
            Base_relocation_block *end_rel_block;

            if (optional_hdr->ImageBase > (unsigned long)dllStart)
            {
                /* Image is loaded  at lower address than preferred. */
                lower_dllStart = 1;
            }
            end_rel_block = rel_block + ((data_dir->Size)
                                         / sizeof(Base_relocation_block));

            for (; rel_block < end_rel_block;)
            {
                unsigned short *cur_rel = (unsigned short *)rel_block;
                unsigned short *end_rel;

                end_rel = (cur_rel
                           + ((rel_block->BlockSize)
                              / sizeof(unsigned short)));
                cur_rel = (unsigned short *)(rel_block + 1);

                for (; cur_rel < end_rel; cur_rel++)
                {
                    /* Top 4 bits indicate the type of relocation. */
                    unsigned char rel_type = (*cur_rel) >> 12;
                    void *rel_target = (dllStart + (rel_block->PageRva)
                                        + ((*cur_rel) & 0x0fff));

                    if (rel_type == IMAGE_REL_BASED_ABSOLUTE) continue;
                    if (rel_type == IMAGE_REL_BASED_HIGHLOW)
                    {
                        if (lower_dllStart)
                            (*((unsigned long *)rel_target)) -= image_diff;
                        else (*((unsigned long *)rel_target)) += image_diff;
                    }
                    else
                    {
                        printf("Unknown PE relocation type: %u\n",
                               rel_type);
                    }
                }

                rel_block = (Base_relocation_block *)cur_rel;
            }
        }
    }

    if (optional_hdr->NumberOfRvaAndSizes > DATA_DIRECTORY_EXPORT_TABLE)
    {
        IMAGE_DATA_DIRECTORY *data_dir = (((IMAGE_DATA_DIRECTORY *)
                                           (optional_hdr + 1))
                                          + DATA_DIRECTORY_EXPORT_TABLE);
        IMAGE_EXPORT_DIRECTORY *export_dir;
        unsigned long *functionTable;
        unsigned long *nameTable;
        unsigned short *ordinalTable;
        unsigned long *thunk;

        if (data_dir->Size == 0)
        {
            printf("DLL has no export tables\n");
            kfree(section_table);
            kfree(optional_hdr);
            return (1);
        }

        export_dir = ((void *)(dllStart + (data_dir->VirtualAddress)));
        functionTable = (void *)(dllStart + (export_dir->AddressOfFunctions));
        nameTable = (void *)(dllStart + (export_dir->AddressOfNames));
        ordinalTable = (void *)(dllStart
                                + (export_dir->AddressOfNameOrdinals));

        /* The importing itself.
         * Import Address Table of the executable is parsed
         * and function addresses are written
         * when they are found. */
        for (thunk = (void *)(exeStart + (import_desc->FirstThunk));
             *thunk != 0;
             thunk++)
        {
            if ((*thunk) & 0x80000000)
            {
                /* Bit 31 set, import by ordinal. */
                *thunk = ((unsigned long)
                          (dllStart
                           + (functionTable[((*thunk) & 0x7fffffff)
                                            - (export_dir->Base)])));
            }
            else
            {
                /* Import by name. */
                unsigned char *hintname = exeStart + ((*thunk) & 0x7fffffff);
                int i;

                /* The first 2 bytes are hint index,
                 * so they are skipped to get the name. */
                hintname += 2;
                for (i = 0; i < (export_dir->NumberOfNames); i++)
                {
                    if (strcmp(hintname, dllStart + (nameTable[i]))) continue;
                    break;
                }
                if (i == (export_dir->NumberOfNames))
                {
                    printf("Function %s not found in DLL\n", hintname);
                    kfree(section_table);
                    kfree(optional_hdr);
                    return (1);
                }
                /* PE specification says that the ordinals in ordinal table
                 * are biased by Ordinal Base,
                 * but it does not seem to be correct. */
                *thunk = (unsigned long)(dllStart
                                         + (functionTable[ordinalTable[i]]));
            }
        }
        /* After the DLL is bound, time/date stamp is copied. */
        import_desc->TimeDateStamp = export_dir->TimeDateStamp;
    }
    if (optional_hdr->NumberOfRvaAndSizes > DATA_DIRECTORY_IMPORT_TABLE)
    {
        IMAGE_DATA_DIRECTORY *data_dir = (((IMAGE_DATA_DIRECTORY *)
                                           (optional_hdr + 1))
                                          + DATA_DIRECTORY_IMPORT_TABLE);
        if (data_dir->Size != 0)
        {
            IMAGE_IMPORT_DESCRIPTOR *import_desc = ((void *)
                                                    (dllStart
                                                     + (data_dir
                                                        ->VirtualAddress)));

            /* Each descriptor is for one DLL
             * and the array has a null terminator. */
            for (; import_desc->OriginalFirstThunk; import_desc++)
            {
                if (exeloadLoadPEDLL(dllStart, import_desc))
                {
                    printf("Failed to load DLL %s\n",
                           dllStart + (import_desc->Name));
                    return (1);
                }
            }
        }
    }

    /* Entry point is optional for DLLs. */
    if (optional_hdr->AddressOfEntryPoint)
    {
        int ret = callDllEntry(dllStart + (optional_hdr->AddressOfEntryPoint),
                               dllStart,
                               1, /* fdwReason = DLL_PROCESS_ATTACH */
                               (void *)1); /* lpvReserved = non-NULL */

        if (ret == 0)
        {
            printf("Initialization of DLL %s failed\n",
                   exeStart + (import_desc->Name));
            kfree(section_table);
            kfree(optional_hdr);
            return (1);
        }
    }

    /* Frees memory not needed by the DLL. */
    kfree(section_table);
    kfree(optional_hdr);

    return (0);
}

static int exeloadLoadLX(unsigned long *entry_point,
                         int fhandle,
                         unsigned long e_lfanew)
{
    unsigned char firstbit[2];
    long newpos;
    size_t readbytes;

    if (PosMoveFilePointer(fhandle, e_lfanew, SEEK_SET, &newpos)
        || PosReadFile(fhandle, firstbit, sizeof(firstbit), &readbytes)
        || (readbytes != sizeof(firstbit))
        || (memcmp(firstbit, "LX", 2) != 0))
    {
        return (1);
    }
    /* LX seems to be called LE sometimes. */
    printf("LX (other name LE) is not supported\n");

    return (2);
}

static int exeloadLoadNE(unsigned long *entry_point,
                         int fhandle,
                         unsigned long e_lfanew)
{
    unsigned char firstbit[2];
    long newpos;
    size_t readbytes;

    if (PosMoveFilePointer(fhandle, e_lfanew, SEEK_SET, &newpos)
        || PosReadFile(fhandle, firstbit, sizeof(firstbit), &readbytes)
        || (readbytes != sizeof(firstbit))
        || (memcmp(firstbit, "NE", 2) != 0))
    {
        return (1);
    }
    printf("NE is not supported\n");

    return (2);
}
#endif

/* Structures here are documented in Appendix B and E of
   MVS Program Management: Advanced Facilities SA22-7644-14:
   http://publibfp.dhe.ibm.com/cgi-bin/bookmgr/BOOKS/iea2b2b1/CCONTENTS
   and the IEBCOPY component is documented here:
   Appendix B of OS/390 DFSMSdfp Utilities SC26-7343-00
   or Appendix B of the z/OS equivalent SC26-7414-05 available here at
   http://publibz.boulder.ibm.com/epubs/pdf/dgt2u140.pdf
*/

#define PE_DEBUG 1

static int fixPE(char *buf, int *len, int *entry, int rlad)
{
    char *p;
    char *q;
    int z;
    typedef struct {
        char pds2name[8];
        char unused1[19];
        char pds2epa[3];
        char pds2ftb1;
        char pds2ftb2;
        char pds2ftb3;
    } IHAPDS;
    IHAPDS *ihapds;
    int rmode;
    int amode;
    int ent;
    int rec = 0;
    int corrupt = 1;
    int rem = *len;
    int l;
    int l2;
    int lastt = -1;
    char *lasttxt = NULL;
    char *upto = buf;
    int initlen = *len;
    
    if ((*len <= 8) || (*((int *)buf + 1) != 0xca6d0f))
    {
        printf("Not an MVS PE executable\n");
        return (-1);
    }
#if PE_DEBUG
    printf("MVS PE total length is %d\n", *len);
#endif
    p = buf;
    while (1)
    {
        rec++;
        l = *(short *)p;
        /* keep track of remaining bytes, and ensure they really exist */
        if (l > rem)
        {
            break;
        }
        rem -= l;
#if PE_DEBUG
        printf("rec %d, offset %d is len %d\n", rec, p - buf, l);
#endif
#if 0
        if (1)
        {
            for (z = 0; z < l; z++)
            {
                printf("z %d %x %c\n", z, p[z], isprint(p[z]) ? p[z] : ' ');
            }
        }
#endif
        if (rec == 3) /* directory record */
        {
            /* there should only be one directory entry, 
               which is 4 + 276 + 12 */
            if (l < 292)
            {
                break;
            }
            q = p + 24;
            l2 = *(short *)q;
            if (l2 < 32) break;
            ihapds = (IHAPDS *)(q + 2);
            rmode = ihapds->pds2ftb2 & 0x10;
/* do we care about alias entry point amode? */
#if 0
            amode = (ihapds->pds2ftb2 & 0x0c) >> 2;
#else
            amode = ihapds->pds2ftb2 & 0x03;
#endif
            ent = 0;
            memcpy((char *)&ent + sizeof(ent) - 3, ihapds->pds2epa, 3);
            *entry = (int)(buf + ent);
#if PE_DEBUG
            printf("module name is %.8s\n", ihapds->pds2name);
            printf("rmode is %s\n", rmode ? "ANY" : "24");
            printf("amode is ");
            if (amode == 0)
            {
                printf("24");
            }
            else if (amode == 2)
            {
                printf("31");
            }
            else if (amode == 1)
            {
                printf("64");
            }
            else if (amode == 3)
            {
                printf("ANY");
            }
            printf("\n");
            printf("entry point is %x\n", ent);
#endif            
        }
        else if (rec > 3)
        {
            int t;
            int r2;
            int l2;
            int term = 0;
            
            if (l < (4 + 12))
            {
                break;
            }
            q = p + 4 + 10;
            r2 = l - 4 - 10;
            while (1)
            {
                l2 = *(short *)q;
                r2 -= sizeof(short);
                if (l2 > r2)
                {
                    term = 1;
                    break;
                }
                r2 -= l2;

                if (l2 == 0) break;
                q += sizeof(short);
#if PE_DEBUG
                printf("load module record is of type %2x (len %5d)"
                       " offset %d\n", 
                       *q, l2, q - p);
#endif

                t = *q;
                if ((lastt == 1) || (lastt == 3) || (lastt == 0x0d))
                {
#if PE_DEBUG
                    printf("rectype: program text\n");
#endif
                    lasttxt = q;
                    memmove(upto, q, l2);
                    upto += l2;
                    t = -1;
                    if (lastt == 0x0d)
                    {
                        term = 1;
                        corrupt = 0;
                        break;
                    }
                }
                else if (t == 0x20)
                {
                    /* printf("rectype: CESD\n"); */
                }
                else if (t == 1)
                {
                    /* printf("rectype: Control\n"); */
                }
                else if (t == 0x0d)
                {
                    /* printf("rectype: Control, about to end\n"); */
                }
                else if (t == 2)
                {
                    /* printf("rectype: RLD\n"); */
                    if (processRLD(buf, rlad, q, l2) != 0)
                    {
                        term = 1;
                        break;
                    }
                }
                else if (t == 3)
                {
                    int l3;
                    
                    /* printf("rectype: Dicionary = Control + RLD\n"); */
                    l3 = *(short *)(q + 6) + 16;
#if 0
                    printf("l3 is %d\n", l3);
#endif
                    if (processRLD(buf, rlad, q, l3) != 0)
                    {
                        term = 1;
                        break;
                    }
                }
                else if (t == 0x0e)
                {
                    /* printf("rectype: Last record of module\n"); */
                    if (processRLD(buf, rlad, q, l2) != 0)
                    {
                        term = 1;
                        break;
                    }
                    term = 1;
                    corrupt = 0;
                    break;
                }
                else if (t == 0x80)
                {
                    /* printf("rectype: CSECT\n"); */
                }
                else
                {
                    /* printf("rectype: unknown %x\n", t); */
                }
#if 0
                if ((t == 0x20) || (t == 2))
                {
                    for (z = 0; z < l; z++)
                    {
                        printf("z %d %x %c\n", z, q[z], 
                               isprint(q[z]) ? q[z] : ' ');
                    }
                }
#endif
                lastt = t;

                q += l2;
                if (r2 == 0)
                {
#if PE_DEBUG
                    printf("another clean exit\n");
#endif
                    break;
                }
                else if (r2 < (10 + sizeof(short)))
                {
                    /* printf("another unclean exit\n"); */
                    term = 1;
                    break;
                }
                r2 -= 10;
                q += 10;
            }
            if (term) break;            
        }
        p = p + l;
        if (rem == 0)
        {
#if PE_DEBUG
            printf("breaking cleanly\n");
#endif
        }
        else if (rem < 2)
        {
            break;
        }
    }
    if (corrupt)
    {
        printf("corrupt module\n");
        return (-1);
    }
#if 0
    printf("dumping new module\n");
#endif
    *len = upto - buf; /* return new module length */
    return (0);
}


static int processRLD(char *buf, int rlad, char *rld, int len)
{
    int l;
    char *r;
    int cont = 0;
    char *fin;
    int negative;
    int ll;
    int a;
    int newval;
    int *zaploc;
    
    r = rld + 16;
    fin = rld + len;
    while (r != fin)
    {
        if (!cont)
        {
            r += 4; /* skip R & P */
            if (r >= fin)
            {
                printf("corrupt1 at position %x\n", r - rld - 4);
                return (-1);
            }
        }
        negative = *r & 0x02;
        if (negative)
        {
            printf("got a negative adjustment - unsupported\n");
            return (-1);
        }
        ll = (*r & 0x0c) >> 2;
        ll++;
        if ((ll != 4) && (ll != 3))
        {
            printf("untested and unsupported relocation %d\n", ll);
            return (-1);
        }
        if (ll == 3)
        {
            if (rlad > 0xffffff)
            {
                printf("AL3 prevents relocating this module to %x\n", rlad);
                return (-1);
            }
        }
        cont = *r & 0x01; /* do we have A & F continous? */
        r++;
        if ((r + 3) > fin)
        {
            printf("corrupt2 at position %x\n", r - rld);
            return (-1);
        }
        a = 0;
        memcpy((char *)&a + sizeof(a) - 3, r, 3);
        /* +++ need bounds checking on this OS code */
        /* printf("need to zap %d bytes at offset %6x\n", ll, a); */
        zaploc = (int *)(buf + a - ((ll == 3) ? 1 : 0));
        newval = *zaploc;
        /* printf("which means that %8x ", newval); */
        newval += rlad;
        /* printf("becomes %8x\n", newval); */
        *zaploc = newval;
        r += 3;
    }
    return (0);
}
