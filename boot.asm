[BITS 32]
[GLOBAL loader]
[EXTERN kernel_main]

; Multiboot 头部必须在最前面
MODULEALIGN equ  1<<0
MEMINFO     equ  1<<1
FLAGS       equ  MODULEALIGN | MEMINFO
MAGIC       equ  0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

align 4
dd MAGIC
dd FLAGS
dd CHECKSUM

section .text
loader:
    mov esp, 0x90000
    call kernel_main
    cli
    hlt
