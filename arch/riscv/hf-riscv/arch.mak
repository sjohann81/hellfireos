# this is stuff specific to this architecture
ARCH_DIR = $(SRC_DIR)/arch/$(ARCH)
INC_DIRS  = -I $(ARCH_DIR)/include

F_CLK=25000000
TIME_SLICE=0

#remove unreferenced functions
CFLAGS_STRIP = -fdata-sections -ffunction-sections
LDFLAGS_STRIP = --gc-sections

# this is stuff used everywhere - compiler and flags should be declared (ASFLAGS, CFLAGS, LDFLAGS, LINKER_SCRIPT, CC, AS, LD, DUMP, READ, OBJ and SIZE).
# remember the kernel, as well as the application, will be compiled using the *same* compiler and flags!
ASFLAGS = -m32 -msoft-float #-fPIC
CFLAGS = -Wall -march=RV32I -O2 -c -msoft-float -fshort-double -ffreestanding -nostdlib -ffixed-s10 -ffixed-s11 $(INC_DIRS) -DCPU_SPEED=${F_CLK} -DTIME_SLICE=${TIME_SLICE} -DLITTLE_ENDIAN $(CFLAGS_STRIP) -DKERN_VER=\"$(KERNEL_VER)\" #-mrvc -fPIC -DDEBUG_PORT
#CFLAGS = -Wall -march=RV32I -O2 -c -msoft-float -ffreestanding -nostdlib -ffixed-s10 -ffixed-s11 $(INC_DIRS) -DCPU_SPEED=${F_CLK} -DTIME_SLICE=${TIME_SLICE} -DLITTLE_ENDIAN $(CFLAGS_STRIP) -DKERN_VER=\"$(KERNEL_VER)\" #-mrvc -fPIC -DDEBUG_PORT
LDFLAGS = -melf32lriscv $(LDFLAGS_STRIP)
LINKER_SCRIPT = $(ARCH_DIR)/hf-riscv.ld

CC = riscv64-unknown-elf-gcc
AS = riscv64-unknown-elf-as
LD = riscv64-unknown-elf-ld
DUMP = riscv64-unknown-elf-objdump -Mno-aliases
READ = riscv64-unknown-elf-readelf
OBJ = riscv64-unknown-elf-objcopy
SIZE = riscv64-unknown-elf-size

hal:
	$(AS) $(ASFLAGS) -o crt0.o $(ARCH_DIR)/boot/crt0.s
	$(CC) $(CFLAGS) \
		$(ARCH_DIR)/drivers/interrupt.c \
		$(ARCH_DIR)/drivers/hal.c \
		$(ARCH_DIR)/drivers/eth_enc28j60.c
		
