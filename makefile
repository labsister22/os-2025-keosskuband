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

# DOOM Configuration
DOOM_DIR      = src/misc/doom
DOOM_OBJDIR   = $(OUTPUT_FOLDER)/doom
DOOM_OUTPUT   = doomgeneric
# DOOM_CFLAGS   = -ggdb3 -Os -Wall -DNORMALUNIX -DLINUX -DSNDSERV -D_DEFAULT_SOURCE -m32 -c -I$(SOURCE_FOLDER) -fno-stack-protector
DOOM_CFLAG    = -nostdlib -fno-stack-protector -nostartfiles -nodefaultlibs -ffreestanding -I$(SOURCE_FOLDER)
DOOM_LFLAGS   = -m32 -nostartfiles -Wl,--gc-sections -static -no-pie
DOOM_LIBS     = -lm -lgcc -lgcc_eh -lc

# DOOM source files (based on the standalone makefile)
DOOM_SRC_FILES = dummy.c am_map.c doomdef.c doomstat.c dstrings.c d_event.c d_items.c d_iwad.c d_loop.c d_main.c d_mode.c d_net.c f_finale.c f_wipe.c g_game.c hu_lib.c hu_stuff.c info.c i_cdmus.c i_endoom.c i_joystick.c i_scale.c i_sound.c i_system.c i_timer.c memio.c m_argv.c m_bbox.c m_cheat.c m_config.c m_controls.c m_fixed.c m_menu.c m_misc.c m_random.c p_ceilng.c p_doors.c p_enemy.c p_floor.c p_inter.c p_lights.c p_map.c p_maputl.c p_mobj.c p_plats.c p_pspr.c p_saveg.c p_setup.c p_sight.c p_spec.c p_switch.c p_telept.c p_tick.c p_user.c r_bsp.c r_data.c r_draw.c r_main.c r_plane.c r_segs.c r_sky.c r_things.c sha1.c sounds.c statdump.c st_lib.c st_stuff.c s_sound.c tables.c v_video.c wi_stuff.c w_checksum.c w_file.c w_main.c w_wad.c z_zone.c w_file_stdc.c i_input.c i_video.c doomgeneric.c doomgeneric_keosskubandOS.c

# Generate DOOM object file paths
DOOM_OBJ := $(addprefix $(DOOM_OBJDIR)/, $(DOOM_SRC_FILES:.c=.o))

run: all
	@qemu-system-i386 -s -m 1024M -drive file=bin/storage.bin,format=raw,if=ide,index=0,media=disk -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso

all: build

build: iso

clean:
	rm -rf $(OUTPUT_FOLDER)/*.o
	rm -rf $(DOOM_OBJDIR)
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
	rm -rf $(OUTPUT_FOLDER)/$(DOOM_OUTPUT)
	rm -rf $(OUTPUT_FOLDER)/$(DOOM_OUTPUT).map

# Create doom directory if it doesn't exist
$(DOOM_OBJDIR):
	@mkdir -p $(DOOM_OBJDIR)

# This pattern rule is now integrated above in the DOOM build system section

kernel:
	@echo "Assembling kernel entrypoint and interrupt setup..."
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/kernel-entrypoint.s -o $(OUTPUT_FOLDER)/kernel-entrypoint.o
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/header/interrupt/intsetup.s -o $(OUTPUT_FOLDER)/intsetup.o
	
	@echo "Compiling kernel C source files..."
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
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/process/process-commands/sleep.c -o $(OUTPUT_FOLDER)/sleep.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/comps/scheduler/scheduler.c -o $(OUTPUT_FOLDER)/scheduler.o

	@echo "Linking kernel objects into ELF32 kernel binary..."
	@$(LIN) $(LFLAGS) $(OUTPUT_FOLDER)/*.o -o $(OUTPUT_FOLDER)/kernel
	@echo "Kernel linked successfully."

ifeq ($(V),1)
	VB=''
else
	VB=@
endif

doom: $(DOOM_OBJ)
	@echo [Assembling DOOM crt0]
	$(VB)$(ASM) $(AFLAGS) $(DOOM_DIR)/crt0.s -o $(DOOM_OBJDIR)/crt0.o
	@echo [Linking $(DOOM_OUTPUT) ELF]
	$(VB)$(CC) $(DOOM_LFLAGS) -T $(DOOM_DIR)/doom-linker.ld \
		$(DOOM_OBJDIR)/crt0.o $(DOOM_OBJ) \
		-o $(OUTPUT_FOLDER)/$(DOOM_OUTPUT).elf $(DOOM_LIBS) -Wl,-Map,$(OUTPUT_FOLDER)/$(DOOM_OUTPUT).map \
		-Wl,--allow-multiple-definition
	@echo [Converting ELF to raw binary]
	objcopy -O binary $(OUTPUT_FOLDER)/$(DOOM_OUTPUT).elf $(OUTPUT_FOLDER)/$(DOOM_OUTPUT)
	@echo [Size of raw binary]
	-size $(OUTPUT_FOLDER)/$(DOOM_OUTPUT)


# Ensure DOOM objects depend on directory creation
$(DOOM_OBJ): | $(DOOM_OBJDIR)

# Pattern rule for compiling DOOM source files
$(DOOM_OBJDIR)/%.o: $(DOOM_DIR)/%.c
	@echo [Compiling $<]
	$(VB)$(CC) $(DOOM_CFLAGS) -c $< -o $@

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
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/stdlib/sleep.c -o sleep.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/apple.c -o apple.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/ps.c -o ps.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/kill.c -o kill.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/exec.c -o exec.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/ikuyokita.c -o ikuyokita.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/rm.c -o rm.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/cp.c -o cp.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/comps/usermode/commands/mv.c -o mv.o
	@$(LIN) -T $(SOURCE_FOLDER)/comps/usermode/user-linker.ld -melf_i386 --oformat=binary \
		crt0.o user-shell.o string.o help.o clear.o echo.o apple.o sleep.o cd.o ls.o mkdir.o find.o \
		cat.o strops.o ps.o kill.o exec.o touch.o ikuyokita.o rm.o cp.o mv.o -o $(OUTPUT_FOLDER)/shell
	@echo Linking object shell object files and generate flat binary...
	@$(LIN) -T $(SOURCE_FOLDER)/comps/usermode/user-linker.ld -melf_i386 --oformat=elf32-i386 \
		crt0.o user-shell.o string.o help.o clear.o echo.o apple.o sleep.o cd.o ls.o mkdir.o find.o \
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
	@echo Inserting experiment into root directory...
	@cd $(OUTPUT_FOLDER); ./inserter experiment 1 $(DISK_NAME).bin

insert-apple: inserter
	@echo Inserting apple into root directory...
	@cd $(OUTPUT_FOLDER); ./inserter apple 1 $(DISK_NAME).bin

insert-ikuyokita: inserter 
	@echo Inserting ikuyokita into root directory...
	@cd $(OUTPUT_FOLDER); ./inserter ikuyokita 1 $(DISK_NAME).bin

insert-doom: inserter doom
	@echo Inserting DOOM executable into root directory...
	@cd $(OUTPUT_FOLDER); ./inserter $(DOOM_OUTPUT) 1 $(DISK_NAME).bin

insert-doomwad: inserter
	@echo Inserting DOOM WAD file into root directory...
	@cd $(OUTPUT_FOLDER); ./inserter doom1.wad 1 $(DISK_NAME).bin

init: clean disk kernel  insert-shell insert-clock insert-experiment insert-apple insert-ikuyokita 
	# @echo "Initialization complete. Disk and executables are ready."

# Debug targets
doom-debug: $(DOOM_OBJ)
	@echo "DOOM Object files:"
	@echo $(DOOM_OBJ)
	@echo "Kernel Object files:"
	@echo $(KERNEL_OBJS)

print-doom:
	@echo "DOOM_SRC_FILES: $(DOOM_SRC_FILES)"
	@echo "DOOM_OBJ: $(DOOM_OBJ)"
	@echo "DOOM_CFLAGS: $(DOOM_CFLAGS)"
	@echo "DOOM_LFLAGS: $(DOOM_LFLAGS)"

