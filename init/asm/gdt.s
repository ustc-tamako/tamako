[GLOBAL gdt_flush]
gdt_flush:
    mov eax, [esp+4]
    lgdt [eax]

    ; segment selector
    ;
    ; *------------------------*-------------*-------------*
    ; |        15  -  3        |      2      |   1  -  0   |
    ; |          index         |     in      |  privilege  |
    ; |       in gdt / idt     |  gdt / idt  |             |
    ; *------------------------*-------------*-------------*
    ;
    ; 0x10  -->  00000000 00010, 0, 00  -->  gdt[2]  -->  GDT_KDATA
    ; 0x08  -->  00000000 00001, 0, 00  -->  gdt[1]  -->  GDT_KTEXT

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.flush

.flush:
    ret