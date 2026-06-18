#!/bin/bash

# ==============================================
# MyOS 自动编译脚本 (最终版)
# ==============================================

# 定义颜色
GREEN='\033[1;32m'
BLUE='\033[1;34m'
RED='\033[1;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}==============================================${NC}"
echo -e "${GREEN}           MyOS 自动编译脚本                  ${NC}"
echo -e "${GREEN}==============================================${NC}"

echo -e "\n${BLUE}[1/7] 清理旧文件...${NC}"
rm -rf isodir
rm -f boot.o main.o kernel.bin myos.iso
rm -f link.ld

echo -e "${BLUE}[2/7] 生成链接脚本 link.ld...${NC}"
cat > link.ld << 'EOF'
ENTRY(loader)

SECTIONS {
    . = 0x100000; /* 内核加载到物理地址 1MB */

    /* 强制把 .multiboot 段放在最前面 */
    .multiboot ALIGN(4) : {
        *(.multiboot)
    }

    .text ALIGN(4K) : {
        *(.text)
    }

    .rodata ALIGN(4K) : {
        *(.rodata)
    }

    .data ALIGN(4K) : {
        *(.data)
    }

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
    }
}
EOF


echo -e "${BLUE}[3/7] 编译 boot.asm...${NC}"
nasm -f elf32 boot.asm -o boot.o
if [ $? -ne 0 ]; then
    echo -e "${RED}汇编编译失败！${NC}"
    exit 1
fi

echo -e "${BLUE}[4/7] 编译 main.cpp (C++)...${NC}"
g++ -m32 -ffreestanding -fno-exceptions -fno-rtti -nostdlib -c main.cpp -o main.o
if [ $? -ne 0 ]; then
    echo -e "${RED}C++ 编译失败！${NC}"
    exit 1
fi

echo -e "${BLUE}[5/7] 链接内核...${NC}"
# 添加 -Map=kernel.map 以便检查链接结果
ld -m elf_i386 -T link.ld -o kernel.bin boot.o main.o --gc-sections -Map=kernel.map
if [ $? -ne 0 ]; then
    echo -e "${RED}链接失败！${NC}"
    exit 1
fi

echo -e "${BLUE}[6/7] 生成 ISO 镜像...${NC}"
mkdir -p isodir/boot/grub
cp kernel.bin isodir/boot/kernel.bin

cat > isodir/boot/grub/grub.cfg << 'EOF'
set timeout=1
set default=0

# 强制设置分辨率
set gfxpayload=640x480x32

menuentry "MyOS" {
    # Multiboot 1 标准
    multiboot /boot/kernel.bin
    boot
}
EOF

grub-mkrescue -o myos.iso isodir > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo -e "${RED}ISO 创建失败！请检查 grub-mkrescue 是否安装。${NC}"
    exit 1
fi

echo -e "${BLUE}[7/7] 启动 QEMU...${NC}"
echo -e "${GREEN}编译成功！正在启动...${NC}"
echo -e "${GREEN}==============================================${NC}\n"

qemu-system-i386 -cdrom myos.iso