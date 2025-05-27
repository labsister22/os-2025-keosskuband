# Compiler & linker
ASM           = nasm
LIN           = ld
CC            = gcc

# Disk (non-volatile storage)
DISK_NAME = storage

# Directory
SOURCE_FOLDER = src
OUTPUT_FOLDER = bin
ISO_NAME      = KeosskuBand

# Flags
# WARNING_CFLAG = -Wall -Wextra -Werror
WARNING_CFLAG = -Wall -Wextra
DEBUG_CFLAG   = -fshort-wchar -g
STRIP_CFLAG   = -nostdlib -fno-stack-protector -nostartfiles -nodefaultlibs -ffreestanding
CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) -m32 -c -I$(SOURCE_FOLDER)
AFLAGS        = -f elf32 -g -F dwarf
LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386


run: all
	@qemu-system-i386 -s -m 1024M -drive file=bin/storage.bin,format=raw,if=ide,index=0,media=disk -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso
all: build
build: iso
clean:
	rm -rf $(OUTPUT_FOLDER)/*.o
	rm -rf *.o *.iso $(OUTPUT_FOLDER)/kernel
	rm -rf $(OUTPUT_FOLDER)/*.iso
	rm -rf $(OUTPUT_FOLDER)/inserter
	rm -rf $(OUTPUT_FOLDER)/shell
	rm -rf $(OUTPUT_FOLDER)/clock
	rm -rf $(OUTPUT_FOLDER)/*.bin
	rm -rf $(OUTPUT_FOLDER)/*.txt
	rm -rf $(OUTPUT_FOLDER)/experiment
	rm -rf $(OUTPUT_FOLDER)/shell_elf
	rm -rf $(OUTPUT_FOLDER)/clock_elf
	rm -rf $(OUTPUT_FOLDER)/experiment_elf

kernel:
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/kernel-entrypoint.s -o $(OUTPUT_FOLDER)/kernel-entrypoint.o
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/header/interrupt/intsetup.s -o $(OUTPUT_FOLDER)/intsetup.o
# TODO: Compile C file with CFLAGSc
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/stdlib/string.c -o $(OUTPUT_FOLDER)/string.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/stdlib/strops.c -o $(OUTPUT_FOLDER)/strops.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/kernel.c -o $(OUTPUT_FOLDER)/kernel.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/cpu/gdt.c -o $(OUTPUT_FOLDER)/gdt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/cpu/portio.c -o $(OUTPUT_FOLDER)/portio.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/driver/cmos.c -o $(OUTPUT_FOLDER)/cmos.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/text/framebuffer.c -o $(OUTPUT_FOLDER)/framebuffer.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/interrupt/interrupt.c -o $(OUTPUT_FOLDER)/interrupt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/interrupt/idt.c -o $(OUTPUT_FOLDER)/idt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/driver/keyboard.c -o $(OUTPUT_FOLDER)/keyboard.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/driver/disk.c -o $(OUTPUT_FOLDER)/disk.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/driver/graphics.c -o $(OUTPUT_FOLDER)/graphics.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/filesys/ext2.c -o $(OUTPUT_FOLDER)/ext2.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/memory/paging.c -o $(OUTPUT_FOLDER)/paging.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/memory/memory-manager.c -o $(OUTPUT_FOLDER)/memory-manager.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/process/process.c -o $(OUTPUT_FOLDER)/process.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/process/process-commands/ps.c -o $(OUTPUT_FOLDER)/ps.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/process/process-commands/exec.c -o $(OUTPUT_FOLDER)/exec.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/scheduler/scheduler.c -o $(OUTPUT_FOLDER)/scheduler.o

	
	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/kernel
	@echo Linking object files and generate elf32...
	@rm -f *.o

iso: kernel
	@mkdir -p $(OUTPUT_FOLDER)/iso/boot/grub
	@cp $(OUTPUT_FOLDER)/kernel     $(OUTPUT_FOLDER)/iso/boot/
	@cp other/grub1                 $(OUTPUT_FOLDER)/iso/boot/grub/
	@cp $(SOURCE_FOLDER)/menu.lst   $(OUTPUT_FOLDER)/iso/boot/grub/
# TODO: Create ISO image
	@cd $(OUTPUT_FOLDER) && genisoimage -R -b boot/grub/grub1 -no-emul-boot -boot-load-size 4 -A os -input-charset utf8 -quiet -boot-info-table -o KeosskuBand.iso iso
	@rm -r $(OUTPUT_FOLDER)/iso/

disk:
	@qemu-img create -f raw $(OUTPUT_FOLDER)/$(DISK_NAME).bin 32M

inserter:
	@$(CC) -Wno-builtin-declaration-mismatch -g -I$(SOURCE_FOLDER) \
		$(SOURCE_FOLDER)/comps/stdlib/string.c \
		$(SOURCE_FOLDER)/comps/filesys/ext2.c \
		$(SOURCE_FOLDER)/comps/inserter/external-inserter.c \
		-o $(OUTPUT_FOLDER)/inserter

user-shell:
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/comps/usermode/crt0.s -o crt0.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/user-shell.c -o user-shell.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/help.c -o help.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/clear.c -o clear.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/echo.c -o echo.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/cd.c -o cd.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/ls.c -o ls.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/mkdir.c -o mkdir.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/find.c -o find.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/cat.c -o cat.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/touch.c -o touch.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/stdlib/string.c -o string.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/stdlib/strops.c -o strops.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/apple.c -o apple.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/ps.c -o ps.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/kill.c -o kill.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/exec.c -o exec.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/ikuyokita.c -o ikuyokita.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/rm.c -o rm.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/cp.c -o cp.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/mv.c -o mv.o
	@$(LIN) -T $(SOURCE_FOLDER)/comps/usermode/user-linker.ld -melf_i386 --oformat=binary \
		crt0.o user-shell.o string.o help.o clear.o echo.o apple.o cd.o ls.o mkdir.o find.o \
		cat.o strops.o ps.o kill.o exec.o touch.o ikuyokita.o rm.o cp.o mv.o -o $(OUTPUT_FOLDER)/shell
	@echo Linking object shell object files and generate flat binary...
	@$(LIN) -T $(SOURCE_FOLDER)/comps/usermode/user-linker.ld -melf_i386 --oformat=elf32-i386 \
		crt0.o user-shell.o string.o help.o clear.o echo.o apple.o cd.o ls.o mkdir.o find.o \
		cat.o strops.o ps.o kill.o exec.o touch.o ikuyokita.o rm.o cp.o mv.o -o $(OUTPUT_FOLDER)/shell_elf
	@echo Linking object shell object files and generate ELF32 for debugging...
	@size --target=binary $(OUTPUT_FOLDER)/shell
	@rm -f *.o

user-clock:
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/comps/clock/crt0-c.s -o crt0-c.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/clock/clock.c -o clock.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/stdlib/strops.c -o strops-c.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/stdlib/string.c -o string-c.o
	@$(LIN) -T $(SOURCE_FOLDER)/comps/clock/clock-linker.ld -melf_i386 --oformat=binary \
		crt0-c.o clock.o strops-c.o string-c.o -o $(OUTPUT_FOLDER)/clock
	@echo Linking clock object files and generate flat binary...
	@$(LIN) -T $(SOURCE_FOLDER)/comps/clock/clock-linker.ld -melf_i386 --oformat=elf32-i386 \
		crt0-c.o clock.o strops-c.o string-c.o -o $(OUTPUT_FOLDER)/clock_elf
	@echo Linking clock object files and generate ELF32 for debugging...
	@size --target=binary $(OUTPUT_FOLDER)/clock
	@rm -f *.o

user-experiment:
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/comps/experiment/crt0-c.s -o crt0-c.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/experiment/experiment.c -o experiment.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/stdlib/strops.c -o strops-c.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/stdlib/string.c -o string-c.o
	@$(LIN) -T $(SOURCE_FOLDER)/comps/experiment/experiment-linker.ld -melf_i386 --oformat=binary \
		crt0-c.o experiment.o strops-c.o string-c.o -o $(OUTPUT_FOLDER)/experiment
	@echo Linking experiment object files and generate flat binary...
	@$(LIN) -T $(SOURCE_FOLDER)/comps/experiment/experiment-linker.ld -melf_i386 --oformat=elf32-i386 \
		crt0-c.o experiment.o strops-c.o string-c.o -o $(OUTPUT_FOLDER)/experiment_elf
	@echo Linking experiment object files and generate ELF32 for debugging...
	@size --target=binary $(OUTPUT_FOLDER)/experiment
	@rm -f *.o

insert-shell: inserter user-shell
	@echo Inserting shell into root directory...
	@cd $(OUTPUT_FOLDER); ./inserter shell 1 $(DISK_NAME).bin

insert-clock: inserter user-clock
	@echo Inserting clock into root directory...
	@cd $(OUTPUT_FOLDER); ./inserter clock 1 $(DISK_NAME).bin

insert-experiment: inserter user-experiment
	@echo Inserting clock into root directory...
	@cd $(OUTPUT_FOLDER); ./inserter experiment 1 $(DISK_NAME).bin

insert-apple: inserter
	@echo Inserting apple into root directory...
	@cd $(OUTPUT_FOLDER); ./inserter apple 1 $(DISK_NAME).bin

insert-ikuyokita: inserter 
	@echo Inserting ikuyokita into root directory...
	@cd $(OUTPUT_FOLDER); ./inserter ikuyokita 1 $(DISK_NAME).bin

init: clean disk insert-shell insert-clock insert-experiment insert-apple insert-ikuyokita 
# test: clean disk insert-shell insert-clock