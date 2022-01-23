%macro ISR_NOERRCODE 1  ; 宏定义，接收一个变量
	[GLOBAL isr%1]
	isr%1:
		cli
		push 0          ; push err_code
		push %1         ; push intr_no
		jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
	[GLOBAL isr%1]
	isr%1:
		cli
		push %1
		jmp isr_common_stub
%endmacro

; 实现中断处理函数
; 0 - 19
ISR_NOERRCODE  0    ; 0 #DE 除 0 异常
ISR_NOERRCODE  1    ; 1 #DB 调试异常
ISR_NOERRCODE  2    ; 2 NMI
ISR_NOERRCODE  3    ; 3 BP 断点异常 
ISR_NOERRCODE  4    ; 4 #OF 溢出 
ISR_NOERRCODE  5    ; 5 #BR 对数组的引用超出边界 
ISR_NOERRCODE  6    ; 6 #UD 无效或未定义的操作码 
ISR_NOERRCODE  7    ; 7 #NM 设备不可用(无数学协处理器) 
ISR_ERRCODE    8    ; 8 #DF 双重故障(有错误代码) 
ISR_NOERRCODE  9    ; 9 协处理器跨段操作
ISR_ERRCODE   10    ; 10 #TS 无效TSS(有错误代码) 
ISR_ERRCODE   11    ; 11 #NP 段不存在(有错误代码) 
ISR_ERRCODE   12    ; 12 #SS 栈错误(有错误代码) 
ISR_ERRCODE   13    ; 13 #GP 常规保护(有错误代码) 
ISR_ERRCODE   14    ; 14 #PF 页故障(有错误代码) 
ISR_NOERRCODE 15    ; 15 CPU 保留 
ISR_NOERRCODE 16    ; 16 #MF 浮点处理单元错误 
ISR_ERRCODE   17    ; 17 #AC 对齐检查 
ISR_NOERRCODE 18    ; 18 #MC 机器检查 
ISR_NOERRCODE 19    ; 19 #XM SIMD(单指令多数据)浮点异常

; 20 - 31
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

[EXTERN isr_handler]

[GLOBAL isr_common_stub]
isr_common_stub:
	pusha           ; push eax, ecx, edx, ebx, esp, ebp, esi, edi
	mov ax, ds
	push eax        ; push ds

	mov ax, 0x10    ; 加载内核数据段描述符
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	push esp        ; 压入 pt_regs_t 指针，作为服务函数的参数
	call isr_handler
	add esp, 4      ; 清空输入参数

	pop ebx         ; pop ds
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx
	mov ss, bx

	popa            ; pop edi, esi, ebp, esp, ebx, edx, ecx, eax
	add esp, 8      ; 清空 intr_no 和 err_code
	
	iret


%macro IRQ 2            ; 接受两个变量。分别为 IRQ 号 (0 - 15) 和中断向量号 (32 - 47)
	[GLOBAL irq%1]
	irq%1:
		cli
		push 0
		push %2
		jmp irq_common_stub
%endmacro

; 32 - 47
IRQ   0,    32      ; 32 系统计时器
IRQ   1,    33      ; 33 键盘
IRQ   2,    34      ; 34 与 IRQ9 相接，MPU-401 MD 使用
IRQ   3,    35      ; 35 串口设备
IRQ   4,    36      ; 36 串口设备
IRQ   5,    37      ; 37 建议声卡使用
IRQ   6,    38      ; 38 软驱传输控制使用
IRQ   7,    39      ; 39 打印机传输控制使用
IRQ   8,    40      ; 40 即时时钟
IRQ   9,    41      ; 41 与 IRQ2 相接，可设定给其他硬件
IRQ  10,    42      ; 42 建议网卡使用
IRQ  11,    43      ; 43 建议 AGP 显卡使用
IRQ  12,    44      ; 44 接 PS/2 鼠标，也可设定给其他硬件
IRQ  13,    45      ; 45 协处理器使用
IRQ  14,    46      ; 46 IDE0 传输控制使用
IRQ  15,    47      ; 47 IDE1 传输控制使用

[EXTERN irq_handler]

[GLOBAL irq_common_stub]
irq_common_stub:
	pusha
	mov ax, ds
	push eax

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	push esp
	call irq_handler
	add esp, 4

	pop ebx
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx
	mov ss, bx

	popa
	add esp, 8

	iret