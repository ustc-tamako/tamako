#include "debug.h"
#include "elf.h"
#include "printk.h"

static elf_t kernel_elf;

void debug_init()
{
    // 从 GRUB 提供的信息中获取到内核符号表和代码地址信息
    kernel_elf = elf_from_multiboot(glb_mboot_ptr);
}

void print_sreg()
{
    static int round = 0;
    uint16_t cs, ds, es, ss;

    __asm__ __volatile__ (
        "mov %%cs, %0\n"
        "mov %%ds, %1\n"
        "mov %%es, %2\n"
        "mov %%ss, %3\n"
        : "=m"(cs), "=m"(ds), "=m"(es), "=m"(ss)
    );

    printk("%d:  cs = 0x%04x\n", round, cs);
    printk("%d:  ds = 0x%04x\n", round, ds);
    printk("%d:  es = 0x%04x\n", round, es);
    printk("%d:  ss = 0x%04x\n", round, ss);

    ++round;
}

void panic(const char * msg)
{
    uint32_t * ebp;
    uint32_t * eip;

    __asm__ __volatile__ (
        "mov %%ebp, %0"
        : "=r"(ebp)
    );

    printk("=============================================\n");
    printk("Kernel Panic: %s\n", msg);

    while (ebp != 0) {
        eip = ebp + 1;
        printk("[0x%08x] %s\n", *eip, elf_lookup_symbol(*eip, &kernel_elf));
        ebp = (uint32_t *) *ebp;
    }

    printk("=============================================\n");

    while(1);
}