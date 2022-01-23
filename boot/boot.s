; ----------------------------------------------------------------
;
;   boot.s -- 内核从这里开始
;
; ----------------------------------------------------------------

; Multiboot 魔数，由规范决定的
MBOOT_HEADER_MAGIC  equ     0x1BADB002

; 0 号位表示所有的引导模块将按页(4KB)边界对齐
MBOOT_PAGE_ALIGN    equ     1 << 0

; 1 号位通过 Multiboot 信息结构的 mem_* 域包括可用内存的信息
; (告诉GRUB把内存空间的信息包含在Multiboot信息结构中)
MBOOT_MEM_INFO      equ     1 << 1    

; 定义我们使用的 Multiboot 的标记
MBOOT_HEADER_FLAGS  equ     MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO

; 域checksum是一个32位的无符号值，当与其他的magic域(也就是magic和flags)
; 相加时，要求其结果必须是32位的无符号值 0 (即magic+flags+checksum = 0)
MBOOT_CHECKSUM      equ     -(MBOOT_HEADER_MAGIC+MBOOT_HEADER_FLAGS)

; 符合Multiboot规范的 OS 映象需要这样一个 magic Multiboot 头
; Multiboot 头的分布必须如下表所示：
; ----------------------------------------------------------
; 偏移量  类型  域名        备注
;
;   0     u32   magic       必需
;   4     u32   flags       必需 
;   8     u32   checksum    必需 
;
; 我们只使用到这些就够了，更多的详细说明请参阅 GNU 相关文档
;-----------------------------------------------------------

;-----------------------------------------------------------------------------

pg_dir      equ     0x0         ; 页目录表
pg_tab_k   equ     0x1000      ; 虚拟地址 0xC0000000 处对应的页表

[BITS 32]       ; 所有代码以 32-bit 的方式编译

section .init   ; 临时代码段从这里开始
; 在代码段的起始位置设置符合 Multiboot 规范的标记
dd MBOOT_HEADER_MAGIC   ; GRUB 会通过这个魔数判断该映像是否支持
dd MBOOT_HEADER_FLAGS   ; GRUB 的一些加载时选项，其详细注释在定义处
dd MBOOT_CHECKSUM       ; 检测数值，其含义在定义处
mboot_ptr:
	dd 0                ; 暂时保存 GRUB 给出的 multiboot 指针，指向0x10000

[GLOBAL start]          ; 向外部声明内核代码入口，此处提供该声明给链接器
[EXTERN stack_bottom]   ; 声明全局内核栈底指针
[EXTERN glb_mboot_ptr]  ; 声明全局 multiboot 指针
[EXTERN kern_entry]     ; 声明内核 C 代码的入口函数

start:
	cli     ; 此时还没有设置好保护模式的中断处理，要关闭中断
	mov [mboot_ptr], ebx

page_init:
	mov eax, pg_tab_k
	or eax, 0x3
	mov [pg_dir], eax
	mov [pg_dir + 0xC00], eax

	mov ecx, 0
pg_tab_loop:
	mov eax, ecx
	mov edx, ecx
	shl edx, 2
	shl eax, 12
	or eax, 0x3
	mov [pg_tab_k + edx], eax
	inc ecx
	cmp ecx, 0x400
	jne pg_tab_loop

pg_enable:
	mov eax, pg_dir
	mov cr3, eax
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

c_entry:
	mov ebp, 0 
	mov esp, [stack_bottom]	; 设置内核栈
	mov eax, [mboot_ptr]
	mov [glb_mboot_ptr], eax
	call kern_entry         ; 调用内核入口函数

stop:
	hlt              ; 停机指令，可以降低 CPU 功耗
	jmp stop         ; 到这里结束，关机什么的后面再说

;-----------------------------------------------------------------------------