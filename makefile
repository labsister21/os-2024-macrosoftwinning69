# Compiler & linker
ASM           = nasm
LIN           = ld
CC            = gcc
ISOGEN		  = genisoimage

# Directory
SOURCE_FOLDER = src
USER_PROGRAM_FOLDER = src/user-program
OUTPUT_FOLDER = bin
ISO_NAME      = OS2024
DISK_NAME     = sample-image-copy

# Flags
WARNING_CFLAG = -Wall -Wextra -Werror
DEBUG_CFLAG   = -fshort-wchar -g
STRIP_CFLAG   = -nostdlib -fno-stack-protector -nostartfiles -nodefaultlibs -ffreestanding
CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) -m32 -c -I$(SOURCE_FOLDER)
AFLAGS        = -f elf32 -g -F dwarf
LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386

disk:
	@qemu-img create -f raw $(OUTPUT_FOLDER)/$(DISK_NAME).bin 4M

run: all
	@qemu-system-i386 -s -S -drive file=$(OUTPUT_FOLDER)/$(DISK_NAME).bin,format=raw,if=ide,index=0,media=disk -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso

all: build
build: iso
clean:
	rm -rf *.o *.iso $(OUTPUT_FOLDER)/kernel

# Compile file
ASM_F = $(wildcard $(SOURCE_FOLDER)/*.s)
SRC = $(wildcard $(SOURCE_FOLDER)/*.c)
OBJS = $(patsubst $(SOURCE_FOLDER)/%.c, $(OUTPUT_FOLDER)/%.o, $(SRC)) $(patsubst $(SOURCE_FOLDER)/%.s, $(OUTPUT_FOLDER)/%.o, $(ASM_F))

# Compile .s rule
$(OUTPUT_FOLDER)/%.o: $(SOURCE_FOLDER)/%.s
	@echo "Compiling $<"
	@$(ASM) $(AFLAGS) $< -o $@

# Compile .c rule
$(OUTPUT_FOLDER)/%.o: $(SOURCE_FOLDER)/%.c
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) $< -o $@

kernel: $(OBJS)
	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/kernel
	@echo Linking object files and generate elf32...
	@rm -f *.o

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

inserter:
	@$(CC) -Wno-builtin-declaration-mismatch -g \
		$(SOURCE_FOLDER)/string.c $(SOURCE_FOLDER)/fat32.c \
		$(USER_PROGRAM_FOLDER)/external-inserter.c \
		-o $(OUTPUT_FOLDER)/inserter

SHELL_FOLDER = $(USER_PROGRAM_FOLDER)/shell

user-shell:
# ASM Flags 
	@$(ASM) $(AFLAGS) $(USER_PROGRAM_FOLDER)/crt0.s -o crt0.o
# CC Flags
	@$(CC)  $(CFLAGS) -fno-pie $(USER_PROGRAM_FOLDER)/string.c -o stdmem.o
	@$(CC)  $(CFLAGS) -fno-pie $(SHELL_FOLDER)/user-shell.c -o user-shell.o
# LIN Flags
	@$(LIN) -T $(SHELL_FOLDER)/user-linker.ld -melf_i386 --oformat=binary \
		crt0.o user-shell.o stdmem.o -o $(OUTPUT_FOLDER)/shell
	@echo Linking object shell object files and generate flat binary...
	@$(LIN) -T $(SHELL_FOLDER)/user-linker.ld -melf_i386 --oformat=elf32-i386\
		crt0.o user-shell.o stdmem.o -o $(OUTPUT_FOLDER)/shell_elf

# Misc
	@echo Linking object shell object files and generate ELF32 for debugging...
	@size --target=binary bin/shell
	@rm -f *.o

CLOCK_FOLDER = $(USER_PROGRAM_FOLDER)/clock

user-clock:
# ASM
	@$(ASM) $(AFLAGS) $(USER_PROGRAM_FOLDER)/crt0.s -o crt0.o
# CC
	@$(CC)  $(CFLAGS) -fno-pie $(USER_PROGRAM_FOLDER)/string.c -o stdmem.o
	@$(CC)  $(CFLAGS) -fno-pie $(CLOCK_FOLDER)/clock.c -o clock.o
# LIN
	@$(LIN) -T $(CLOCK_FOLDER)/clock-linker.ld -melf_i386 --oformat=binary \
		crt0.o clock.o stdmem.o -o $(OUTPUT_FOLDER)/clock
	@echo Linking object clock object files and generate flat binary...
	@$(LIN) -T $(CLOCK_FOLDER)/clock-linker.ld -melf_i386 --oformat=elf32-i386\
		crt0.o clock.o stdmem.o -o $(OUTPUT_FOLDER)/clock_elf
# Misc
	@echo Linking object clock object files and generate ELF32 for debugging...
	@size --target=binary bin/clock
	@rm -f *.o

insert-shell: inserter user-shell user-clock
	@echo Inserting shell into bin directory...
		@cd $(OUTPUT_FOLDER); ./inserter shell 5 $(DISK_NAME).bin
	@echo Inserting clock into bin directory...
		@cd $(OUTPUT_FOLDER); ./inserter clock 5 $(DISK_NAME).bin