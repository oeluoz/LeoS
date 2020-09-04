BUILD_DIR = ./../build
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB = -I ./../include
INCLUDE = ./../include
ASFLAGS = -f elf32

CFLAGS = -Wall $(LIB) -fno-stack-protector -c -m32
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main

OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
      $(BUILD_DIR)/printk.o $(BUILD_DIR)/kernel.o 

# C代码编译
$(BUILD_DIR)/main.o : ./../kernel/main.c $(INCLUDE)/*.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/init.o : ./../kernel/init.c $(INCLUDE)/*.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/interrupt.o : ./../kernel/interrupt.c $(INCLUDE)/*.h
	$(CC) $(CFLAGS) $< -o $@

# 汇编代码编译
$(BUILD_DIR)/kernel.o : ./../kernel/kernel.s
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/printk.o : ./../kernel/printk.s
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/boot.bin : ./boot.s ./boot.inc
	$(AS) ./boot.s -I boot.inc -o $(BUILD_DIR)/boot.bin
$(BUILD_DIR)/setup.bin : ./setup.s ./boot.inc
	$(AS) ./setup.s -I boot.inc -o $(BUILD_DIR)/setup.bin
$(BUILD_DIR)/clear.bin : ./clear.s ./boot.inc
	$(AS) ./clear.s -I boot.inc -o $(BUILD_DIR)/clear.bin
# 链接所有目标文件
$(BUILD_DIR)/kernel.bin : $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@
# 向磁盘写入数据
hd :
	# 增加一个文件用来清空img
	dd if=$(BUILD_DIR)/clear.bin of=LeoS.img bs=512 count=45  seek=0 conv=notrunc
	dd if=$(BUILD_DIR)/boot.bin of=LeoS.img bs=512 count=1  seek=0 conv=notrunc
	dd if=$(BUILD_DIR)/setup.bin of=LeoS.img bs=512 count=4 seek=1 conv=notrunc
	dd if=$(BUILD_DIR)/kernel.bin of=LeoS.img bs=512 count=40 seek=5 conv=notrunc
build : $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/boot.bin $(BUILD_DIR)/setup.bin $(BUILD_DIR)/clear.bin 
# 删除编译完成的文件
clean :
	rm -rf $(BUILD_DIR)/*
all : build hd
	