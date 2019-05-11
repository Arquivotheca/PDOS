/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  exeload.h - functions for loading executables                    */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exeload.h"
#include "pos.h"
#include "liballoc.h" /* For kmalloc() and kfree(). */
#include "support.h"

/* Headers for executable support. */
#include "a_out.h"
#include "elf.h"
#include "mz.h"
#include "pecoff.h"

static int exeloadLoadAOUT(EXELOAD *exeload, FILE *fp);
static int exeloadLoadELF(EXELOAD *exeload, FILE *fp);
static int exeloadLoadMZ(EXELOAD *exeload, FILE *fp);
/* Subfunctions of exeloadLoadMZ() for loading extensions to MZ. */
static int exeloadLoadPE(EXELOAD *exeload, FILE *fp, unsigned long e_lfanew);
static int exeloadLoadLX(EXELOAD *exeload, FILE *fp, unsigned long e_lfanew);
static int exeloadLoadNE(EXELOAD *exeload, FILE *fp, unsigned long e_lfanew);

int exeloadDoload(EXELOAD *exeload, char *prog)
{
    FILE *fp;
    int ret;

    fp = fopen(prog, "rb");
    /* Tries to load the executable as different formats.
     * Returned 0 means the executable was loaded successfully.
     * 1 means it is not the format the function loads.
     * 2 means correct format, but error occured. */
    ret = exeloadLoadAOUT(exeload, fp);
    if (ret == 1) ret = exeloadLoadELF(exeload, fp);
    if (ret == 1) ret = exeloadLoadMZ(exeload, fp);
    if (ret != 0)
    {
        fclose(fp);
        return (1);
    }

    return (0);
}

static int exeloadLoadAOUT(EXELOAD *exeload, FILE *fp)
{
    int doing_zmagic = 0;
    int doing_nmagic = 0;
    struct exec firstbit;
    unsigned int headerLen;
    unsigned char *header = NULL;
    unsigned long exeLen;
    unsigned char *exeStart;
    unsigned int sp;
    unsigned char *bss;

    rewind(fp);
    if (fread(&firstbit, 1, sizeof(firstbit), fp) != sizeof(firstbit))
    {
        return (1);
    }
    if ((firstbit.a_info & 0xffff) == ZMAGIC)
    {
        doing_zmagic = 1;
    }
    else if ((firstbit.a_info & 0xffff) == NMAGIC)
    {
        doing_nmagic = 1;
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
    if (doing_zmagic)
    {
        headerLen = N_TXTOFF(firstbit);
        header = kmalloc(headerLen);
        memcpy(header, &firstbit, sizeof firstbit);
        fread(header + sizeof firstbit, 1, headerLen - sizeof firstbit, fp);
    }

    if (doing_zmagic || doing_nmagic)
    {
        exeLen = N_BSSADDR(firstbit) - N_TXTADDR(firstbit) + firstbit.a_bss;
    }
    else
    {
        exeLen = firstbit.a_text + firstbit.a_data + firstbit.a_bss;
    }
    /* Allocates memory for the process and stack. */
    exeStart = PosVirtualAlloc(0, (exeLen + (exeload->extra_memory_after)));

    fread(exeStart, 1, firstbit.a_text, fp);
    if (doing_zmagic || doing_nmagic)
    {
        fread(exeStart + N_DATADDR(firstbit) - N_TXTADDR(firstbit), 1,
              firstbit.a_data, fp);
    }
    else
    {
        fread(exeStart + firstbit.a_text, 1, firstbit.a_data, fp);
    }
    /* Closes the file for ZMAGIC and NMAGIC as there is no need for it now. */
    if (doing_zmagic || doing_nmagic) fclose(fp);
    if (doing_zmagic) kfree(header);

    /* initialise BSS */
    if (doing_zmagic || doing_nmagic)
    {
        bss = exeStart + N_BSSADDR(firstbit);
    }
    else
    {
        bss = exeStart + firstbit.a_text + firstbit.a_data;
    }
    memset(bss, '\0', firstbit.a_bss);
    if (doing_zmagic || doing_nmagic)
    {
        sp = N_BSSADDR(firstbit) + firstbit.a_bss + 0x8000;
    }
    else
    {
        sp = (unsigned int)bss + firstbit.a_bss + exeload->stack_size;
    }
    /* Relocations. */
    if (!doing_zmagic && !doing_nmagic)
    {
        unsigned int *corrections;
        unsigned int i;
        unsigned int offs;
        unsigned int type;
        unsigned int zapdata;
        unsigned char *zap;

        zap = exeStart;
        zapdata = (unsigned int)ADDR2ABS(zap);
        if (firstbit.a_trsize != 0)
        {
            corrections = kmalloc(firstbit.a_trsize);
            if (corrections == NULL)
            {
                printf("insufficient memory %lu\n", firstbit.a_trsize);
                return (2);
            }
            fread(corrections, 1, firstbit.a_trsize, fp);
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
            kfree(corrections);
        }
        if (firstbit.a_drsize != 0)
        {
            corrections = kmalloc(firstbit.a_drsize);
            if (corrections == NULL)
            {
                printf("insufficient memory %lu\n", firstbit.a_drsize);
                return (2);
            }
            fread(corrections, 1, firstbit.a_drsize, fp);
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
            kfree(corrections);
        }
        firstbit.a_entry += (unsigned long)ADDR2ABS(exeStart);
        fclose(fp);
    }

    if (doing_zmagic || doing_nmagic)
    {
        /* a.out ZMAGIC and NMAGIC must be loaded at 0x10000. */
        exeStart = exeStart - 0x10000;
        exeStart = ADDR2ABS(exeStart);
        exeload->entry_point = firstbit.a_entry;
        exeload->sp = sp;
        exeload->cs_address = (unsigned long)exeStart;
        exeload->ds_address = (unsigned long)exeStart;
    }
    else
    {
        /* a.out OMAGIC can be loaded anywhere. */
        sp = (unsigned int)ADDR2ABS(sp);
        exeload->entry_point = firstbit.a_entry;
        exeload->sp = sp;
        exeload->cs_address = 0;
        exeload->ds_address = 0;
    }

    return (0);
}

static int exeloadLoadELF(EXELOAD *exeload, FILE *fp)
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
    unsigned int sp;
    unsigned long exeLen;
    unsigned char firstbit[4];

    rewind(fp);
    if ((fread(firstbit, 1, sizeof(firstbit), fp) != sizeof(firstbit))
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
        rewind(fp);
        if (fread(elfHdr, 1,
                  sizeof(Elf32_Ehdr), fp)
            != sizeof(Elf32_Ehdr))
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
            fseek(fp, elfHdr->e_phoff, SEEK_SET);
            if (fread(program_table, 1,
                      elfHdr->e_phnum * elfHdr->e_phentsize, fp)
                != (elfHdr->e_phnum * elfHdr->e_phentsize))
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
            fseek(fp, elfHdr->e_shoff, SEEK_SET);
            if (fread(section_table, 1,
                      elfHdr->e_shnum * elfHdr->e_shentsize, fp)
                != (elfHdr->e_shnum * elfHdr->e_shentsize))
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
    /* Allocates memory for the process and stack. */
    exeStart = PosVirtualAlloc(0, (exeLen + (exeload->extra_memory_after)));

    if (doing_elf_rel || doing_elf_exec)
    {
        /* Loads all sections of ELF file with proper alignment,
         * clears all SHT_NOBITS sections and stores the addresses
         * in sh_addr of each section.
         * bss and sp are set now too. */
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

                    fseek(fp, segment->p_offset, SEEK_SET);
                    if (fread(exe_addr, 1, segment->p_filesz, fp)
                        != (segment->p_filesz))
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
                    fseek(fp, section->sh_offset, SEEK_SET);
                    if (fread(exe_addr, 1, section->sh_size, fp)
                        != (section->sh_size))
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
                    fseek(fp, section->sh_offset, SEEK_SET);
                    if (fread(other_addr, 1, section->sh_size, fp)
                        != (section->sh_size))
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
        /* Sets the stack pointer. */
        if (doing_elf_exec)
        {
            sp = (unsigned long)exeStart + exeLen + exeload->stack_size;
        }
        else
        {
            sp = (unsigned long)exe_addr + exeload->stack_size;
        }
    }
    /* Program was successfully loaded from the file,
     * no more errors can occur. */
    fclose(fp);

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
        unsigned long entry_point = elfHdr->e_entry;

        /* ELF Relocatable files can be loaded anywhere. */
        sp = (unsigned int)ADDR2ABS(sp);
        /* Frees memory not needed by the process. */
        kfree(elfHdr);
        if (program_table) kfree(program_table);
        kfree(section_table);
        kfree(elf_other_sections);
        exeload->entry_point = (unsigned long)exeStart + entry_point;
        exeload->sp = sp;
        exeload->cs_address = 0;
        exeload->ds_address = 0;
    }
    else if (doing_elf_exec)
    {
        unsigned long entry_point = elfHdr->e_entry;

        /* ELF Executable files are loaded at the lowest p_vaddr. */
        exeStart -= lowest_p_vaddr;
        if (entry_point == 0) entry_point = lowest_p_vaddr;
        sp += lowest_p_vaddr;
        exeStart = ADDR2ABS(exeStart);
        /* Frees memory not needed by the process. */
        kfree(elfHdr);
        kfree(program_table);
        if (section_table)
        {
            kfree(section_table);
            kfree(elf_other_sections);
        }
        exeload->entry_point = entry_point;
        exeload->sp = sp;
        exeload->cs_address = (unsigned long)exeStart;
        exeload->ds_address = (unsigned long)exeStart;
    }

    return (0);
}

static int exeloadLoadMZ(EXELOAD *exeload, FILE *fp)
{
    Mz_hdr firstbit;

    rewind(fp);

    /* The header size is in paragraphs,
     * so the smallest possible header is 16 bytes (paragraph) long.
     * Next is the magic number checked. */
    if ((fread(&firstbit, 1, 16, fp) != 16)
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
    if (fread(((char *)&firstbit) + 16, 1,
              (firstbit.header_size - 1) * 16, fp)
        != (firstbit.header_size - 1) * 16)
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
        ret = exeloadLoadPE(exeload, fp, firstbit.e_lfanew);
        if (ret == 1)
            ret = exeloadLoadLX(exeload, fp, firstbit.e_lfanew);
        if (ret == 1)
            ret = exeloadLoadNE(exeload, fp, firstbit.e_lfanew);
        if (ret == 1)
            printf("Unknown MZ extension\n");
        return (ret);
    }
    /* Pure MZ executables are for 16-bit DOS, so we cannot run them. */
    printf("Pure MZ executables are not supported\n");

    return (2);
}

static int exeloadLoadPE(EXELOAD *exeload, FILE *fp, unsigned long e_lfanew)
{
    Coff_hdr coff_hdr;
    Pe32_optional_hdr *optional_hdr;
    Coff_section *section_table, *section;
    unsigned char *exeStart;
    unsigned int sp;

    {
        unsigned char firstbit[4];

        if (fseek(fp, e_lfanew, SEEK_SET)
            || (fread(firstbit, 1, 4, fp) != 4)
            || (memcmp(firstbit, "PE\0\0", 4) != 0))
        {
            return (1);
        }
    }
    if (fread(&coff_hdr, 1, sizeof(coff_hdr), fp) != sizeof(coff_hdr))
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
    if (fread(optional_hdr, 1, coff_hdr.SizeOfOptionalHeader, fp)
        != coff_hdr.SizeOfOptionalHeader)
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
    if (fread(section_table, 1,
              coff_hdr.NumberOfSections * sizeof(Coff_section), fp)
        != (coff_hdr.NumberOfSections * sizeof(Coff_section)))
    {
        printf("Error occured while reading PE optional header\n");
        kfree(section_table);
        kfree(optional_hdr);
        return (2);
    }

    /* Allocates memory for the process and stack.
     * Size of image is obtained from the optional header. */
    exeStart = PosVirtualAlloc(0, (optional_hdr->SizeOfImage
                                   + (exeload->extra_memory_after)));

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
        if (fseek(fp, section->PointerToRawData, SEEK_SET)
            || (fread(exeStart + section->VirtualAddress, 1,
                      size_in_file, fp) != size_in_file))
        {
            printf("Error occured while reading PE section\n");
            kfree(section_table);
            kfree(optional_hdr);
            return (2);
        }
    }
    fclose(fp);

    if (!(coff_hdr.Characteristics & IMAGE_FILE_RELOCS_STRIPPED))
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
        /* Relocatable files can be loaded anywhere. */
        exeload->entry_point = ((unsigned long)exeStart
                                + (optional_hdr->AddressOfEntryPoint));
        sp = ((unsigned long)exeStart
              + (optional_hdr->SizeOfImage)
              + (exeload->stack_size));
        exeload->sp = (unsigned int)ADDR2ABS(sp);
        /* Frees memory not needed by the process. */
        kfree(section_table);
        kfree(optional_hdr);
        exeload->cs_address = 0;
        exeload->ds_address = 0;

        return (0);
    }

    /* PE executable files are loaded at their preferred address. */
    exeload->entry_point = ((optional_hdr->ImageBase)
                             + (optional_hdr->AddressOfEntryPoint));
    sp = ((unsigned long)exeStart
          + (optional_hdr->SizeOfImage)
          + (exeload->stack_size));
    sp += optional_hdr->ImageBase;
    exeload->sp = sp;
    exeStart -= optional_hdr->ImageBase;
    exeStart = ADDR2ABS(exeStart);
    /* Frees memory not needed by the process. */
    kfree(section_table);
    kfree(optional_hdr);
    exeload->cs_address = (unsigned long)exeStart;
    exeload->ds_address = (unsigned long)exeStart;

    return (0);
}

static int exeloadLoadLX(EXELOAD *exeload, FILE *fp, unsigned long e_lfanew)
{
    unsigned char firstbit[2];

    if (fseek(fp, e_lfanew, SEEK_SET)
        || (fread(firstbit, 1, sizeof(firstbit), fp) != sizeof(firstbit))
        || (memcmp(firstbit, "LX", 2) != 0))
    {
        return (1);
    }
    /* LX seems to be called LE sometimes. */
    printf("LX (other name LE) is not supported\n");

    return (2);
}

static int exeloadLoadNE(EXELOAD *exeload, FILE *fp, unsigned long e_lfanew)
{
    unsigned char firstbit[2];

    if (fseek(fp, e_lfanew, SEEK_SET)
        || (fread(firstbit, 1, sizeof(firstbit), fp) != sizeof(firstbit))
        || (memcmp(firstbit, "NE", 2) != 0))
    {
        return (1);
    }
    printf("NE is not supported\n");

    return (2);
}
