#include "common.h"
#include "string.h"
#include "elf.h"

elf_t elf_from_multiboot(multiboot_t * mb)
{
    elf_t elf;
    elf_section_header_t * sh = (elf_section_header_t *)mb->addr;

    uint32_t shstrtab = sh[mb->shndx].addr;
    for (int i = 0; i < mb->num; i++) {
        const char * name = (const char *)(shstrtab + sh[i].name);
        if (strcmp(name, ".strtab") == 0) {
            elf.strtab = (const char *)sh[i].addr;
            elf.strtabsz = sh[i].size;
        }
        if (strcmp(name, ".symtab") == 0) {
            elf.symtab = (elf_symbol_t *)sh[i].addr;
            elf.symtabsz = sh[i].size;
        }
    }

    return elf;
}

const char * elf_lookup_symbol(uint32_t addr, elf_t * elf)
{
    for (int i = 0; i < (elf->symtabsz / sizeof(elf_symbol_t)); i++) {
        if (ELF32_ST_TYPE(elf->symtab[i].info) != 0x2) {
            continue;
        }
        if ( (addr >= elf->symtab[i].value) && (addr < (elf->symtab[i].value + elf->symtab[i].size)) ) {
            return (const char *)((uint32_t)elf->strtab + elf->symtab[i].name);
        }
    }

    return NULL;
}