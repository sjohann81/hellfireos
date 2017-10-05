# this is stuff specific to this architecture
ARCH_DIR = $(SRC_DIR)/arch/$(ARCH)
INC_DIRS  = -I $(ARCH_DIR)/include

F_CLK=25000000
TIME_SLICE=0

CFLAGS_FEW_REGS = -ffixed-t0 -ffixed-t1 -ffixed-t2 -ffixed-t3 -ffixed-t4 -ffixed-t5 -ffixed-t6 -ffixed-t7 -ffixed-s0 -ffixed-s1 -ffixed-s2 -ffixed-s3 -ffixed-s4 -ffixed-s5 -ffixed-s6 -ffixed-s7
CFLAGS_NO_HW_MULDIV = -mnohwmult -mnohwdiv -ffixed-lo -ffixed-hi
CFLAGS_NO_HW_DIV = -mnohwdiv
#remove unreferenced functions
CFLAGS_STRIP = -fdata-sections -ffunction-sections
LDFLAGS_STRIP = --gc-sections

# this is stuff used everywhere - compiler and flags should be declared (ASFLAGS, CFLAGS, LDFLAGS, LINKER_SCRIPT, CC, AS, LD, DUMP, READ, OBJ and SIZE).
# remember the kernel, as well as the application, will be compiled using the *same* compiler and flags!
ASFLAGS = -mips1 -msoft-float
CFLAGS = -Wall -O2 -c -mips2 -mno-branch-likely -mpatfree -mfix-r4000 -mno-check-zero-division -msoft-float -ffreestanding -nostdlib -fomit-frame-pointer -G 0 $(INC_DIRS) -DCPU_SPEED=${F_CLK} -DTIME_SLICE=${TIME_SLICE} -DBIG_ENDIAN $(CFLAGS_NO_HW_MULDIV) -DKERN_VER=\"$(KERNEL_VER)\" $(CFLAGS_STRIP) #-DDEBUG_PORT # -mips2 -mno-branch-likely -fshort-double
LDFLAGS = -mips1 $(LDFLAGS_STRIP)
LINKER_SCRIPT = $(ARCH_DIR)/hf-risc.ld

CC = mips-elf-gcc
AS = mips-elf-as
LD = mips-elf-ld 
DUMP = mips-elf-objdump
READ = mips-elf-readelf
OBJ = mips-elf-objcopy
SIZE = mips-elf-size

hal:
	$(AS) $(ASFLAGS) -o crt0.o $(ARCH_DIR)/boot/crt0.s
	$(CC) $(CFLAGS) \
		$(ARCH_DIR)/drivers/interrupt.c \
		$(ARCH_DIR)/drivers/hal.c \
		$(ARCH_DIR)/drivers/eth_enc28j60.c
