[GLOBAL switch_to]
switch_to:
	mov eax, [esp+4]
	mov edx, [esp+8]

	pushf
	pusha
	mov [eax], esp

	mov esp, [edx]
	popa
	popf

	ret
