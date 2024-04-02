# Compiler & linker
ASM           = nasm
LIN           = ld
CC            = gcc
ISOGEN		  = genisoimage

# Directory
SOURCE_FOLDER = src
OUTPUT_FOLDER = bin
ISO_NAME      = OS2024

# Flags
WARNING_CFLAG = -Wall -Wextra -Werror
DEBUG_CFLAG   = -fshort-wchar -g
STRIP_CFLAG   = -nostdlib -fno-stack-protector -nostartfiles -nodefaultlibs -ffreestanding
CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) -m32 -c -I$(SOURCE_FOLDER)
AFLAGS        = -f elf32 -g -F dwarf
LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386


run: all
	@qemu-system-i386 -s -S -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso
all: build
build: iso
clean:
	rm -rf *.o *.iso $(OUTPUT_FOLDER)/kernel

# Compile files
ASM = $(wildcard $(SOURCE_FOLDER)/*.s)
SRC = $(wildcard $(SOURCE_FOLDER)/*.c)
OBJS = $(patsubst $(SOURCE_FOLDER)/%.c, $(OUTPUT_FOLDER)/%.o, $(SRC)) $(patsubst $(SOURCE_FOLDER)/%.s, $(OUTPUT_FOLDER)/%.o, $(ASM))

# Compile .s rule
$(OUTPUT_FOLDER)/%.o: $(SOURCE_FOLDER)/%.s
	@echo "Compiling $<"
	@$(ASM) $(AFLAGS) $< -o $@`

# Compile .c rule
$(OUTPUT_FOLDER)/%.o: $(SOURCE_FOLDER)/%.c
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) $< -o $@ 

kernel: $(OBJS)
	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/kernel
	@echo Linking object files and generate elf32...
	@rm -f *.o

# kernel:
# 	# ASM
# 	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/kernel-entrypoint.s -o $(OUTPUT_FOLDER)/kernel-entrypoint.o
	
# 	# C
# 	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/kernel.c -o $(OUTPUT_FOLDER)/kernel.o
# 	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/gdt.c -o $(OUTPUT_FOLDER)/gdt.o
# 	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/portio.c -o $(OUTPUT_FOLDER)/portio.o
# 	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/framebuffer.c -o $(OUTPUT_FOLDER)/framebuffer.o
# 	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/interrupt.c -o $(OUTPUT_FOLDER)/interrupt.o
# 	# @$(CC) $(CFLAGS) $(SOURCE_FOLDER)/keyboard.c -o $(OUTPUT_FOLDER)/keyboard.o
	
# 	# Linker
# 	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/kernel
# 	@echo Linking object files and generate elf32...
# 	@rm -f *.o

iso: kernel
	@mkdir -p $(OUTPUT_FOLDER)/iso/boot/grub
	@cp $(OUTPUT_FOLDER)/kernel     $(OUTPUT_FOLDER)/iso/boot/
	@cp other/grub1                 $(OUTPUT_FOLDER)/iso/boot/grub/
	@cp $(SOURCE_FOLDER)/menu.lst   $(OUTPUT_FOLDER)/iso/boot/grub/
	@$(ISOGEN) \
		-R \
		-b boot/grub/grub1 \
		-no-emul-boot \
		-boot-load-size 4 \
		-A os \
		-input-charset utf8 \
		-quiet \
		-boot-info-table \
		-o $(OUTPUT_FOLDER)/OS2024.iso \
		$(OUTPUT_FOLDER)/iso
	#@rm -r $(OUTPUT_FOLDER)/iso/
