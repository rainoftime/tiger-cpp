.text
.globl tigermain
.type tigermain, @function
.set tigermain_framesize, 0
tigermain:
L4:
leaq tigermain_framesize(%rsp), %rcx
movq $0, %rdx
movq $0, %rcx
movq $100, %rsi
L0:
cmpq %rsi, %rcx
jle L1
L2:
jmp L3
L1:
addq $1, %rdx
addq $1, %rcx
jmp L0
L3:


retq
.size tigermain, .-tigermain
.section .rodata
