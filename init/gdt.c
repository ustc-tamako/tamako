#include "gdt.h"

#define GDT_LENGTH 5

#define GDT_NULL    0
#define GDT_KTEXT   1
#define GDT_KDATA   2
#define GDT_UTEXT   3
#define GDT_UDATA   4

glb_desc_t gdt[GDT_LENGTH];
gdtr_t gdtr;

// 将 GDT 首址加载到 GDTR
extern void gdt_flush(uint32_t);

static void gdt_set_gate(int32_t idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt[idx].base_low     = base & 0xFFFF;
    gdt[idx].base_middle  = (base >> 16) & 0xFF;
    gdt[idx].base_high    = (base >> 24) & 0xFF;

    gdt[idx].limit_low    = limit & 0xFFFF;
    gdt[idx].granularity  = (limit >> 16) & 0x0F;

    gdt[idx].access       = access;
    gdt[idx].granularity |= gran & 0xF0;
}

void setup_gdt()
{
    gdtr.limit = sizeof(glb_desc_t) * GDT_LENGTH - 1;
    gdtr.base = (uint32_t) &gdt;

    gdt_set_gate(GDT_NULL, 0, 0, 0, 0);
    gdt_set_gate(GDT_KTEXT, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    gdt_set_gate(GDT_KDATA, 0, 0xFFFFFFFF, 0x92, 0xCF);
    gdt_set_gate(GDT_UTEXT, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    gdt_set_gate(GDT_UDATA, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    gdt_flush((uint32_t) &gdtr);
}