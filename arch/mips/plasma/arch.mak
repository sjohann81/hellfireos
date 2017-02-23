# this is stuff specific to this architecture
ARCH_DIR = $(SRC_DIR)/arch/$(ARCH)
INC_DIRS  = -I $(ARCH_DIR)/include

F_CLK=25000000
TIME_SLICE=10480

#remove unreferenced functions
CFLAGS_STRIP = -fdata-sections -ffunction-sections
LDFLAGS_STRIP = --gc-sections

# this is stuff used everywhere - compiler and flags should be declared (ASFLAGS, CFLAGS, LDFLAGS, LINKER_SCRIPT, CC, AS, LD, DUMP, READ, OBJ and SIZE).
# remember the kernel, as well as the application, will be compiled using the *same* compiler and flags!
ASFLAGS = -mips1 -msoft-float
CFLAGS = -Wall -O2 -c -mips1 -mpatfree -mno-check-zero-division -msoft-float -fshort-double -ffreestanding -nostdlib -fomit-frame-pointer -G 0 $(INC_DIRS) -DCPU_SPEED=${F_CLK} -DTIME_SLICE=${TIME_SLICE} -DBIG_ENDIAN $(CFLAGS_STRIP) -DKERN_VER=\"$(KERNEL_VER)\" -DTICK_TIME=18 #-DDEBUG_PORT
LDFLAGS = -mips1 $(LDFLAGS_STRIP)
LINKER_SCRIPT = $(ARCH_DIR)/plasma.ld

CC = mips-elf-gcc
AS = mips-elf-as
LD = mips-elf-ld 
DUMP = mips-elf-objdump
READ = mips-elf-readelf
OBJ = mips-elf-objcopy
SIZE = mips-elf-size

hal: noc
	$(AS) $(ASFLAGS) -o crt0.o $(ARCH_DIR)/boot/crt0.s
	$(CC) $(CFLAGS) \
		$(ARCH_DIR)/drivers/interrupt.c \
		$(ARCH_DIR)/drivers/hal.c \
		$(ARCH_DIR)/drivers/ni.c
