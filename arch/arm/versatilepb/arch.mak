# this is stuff specific to this architecture
ARCH_DIR = $(SRC_DIR)/arch/$(ARCH)
INC_DIRS  = -I $(ARCH_DIR)/include

F_CLK=250000000
TIME_SLICE=0

#remove unreferenced functions
CFLAGS_STRIP = -ffunction-sections -fdata-sections -fno-strict-aliasing
LDFLAGS_STRIP = --gc-sections

# this is stuff used everywhere - compiler and flags should be declared (ASFLAGS, CFLAGS, LDFLAGS, LINKER_SCRIPT, CC, AS, LD, DUMP, READ, OBJ and SIZE).
# remember the kernel, as well as the application, will be compiled using the *same* compiler and flags!
CFLAGS = -Wall -O2 -c -march=armv6 -msoft-float -mabi=atpcs -fPIC -marm -ffreestanding -nostdlib $(INC_DIRS) -DCPU_SPEED=${F_CLK} -DTIME_SLICE=${TIME_SLICE} -DLITTLE_ENDIAN -DKERN_VER=\"$(KERNEL_VER)\" $(CFLAGS_STRIP) # -fshort-double -mapcs-frame 
LDFLAGS = -nodefaultlibs $(LDFLAGS_STRIP)
LINKER_SCRIPT = $(ARCH_DIR)/versatilepb.ld

CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
DUMP = arm-none-eabi-objdump
READ = arm-none-eabi-readelf
OBJ = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

hal:
	$(CC) $(CFLAGS) -o crt0.o $(ARCH_DIR)/boot/crt0.s
	$(CC) $(CFLAGS) \
		$(ARCH_DIR)/lib/__aeabi_idiv.c \
		$(ARCH_DIR)/lib/__aeabi_idivmod.S \
		$(ARCH_DIR)/lib/__aeabi_uidiv.c \
		$(ARCH_DIR)/lib/__aeabi_uidivmod.S \
		$(ARCH_DIR)/drivers/interrupt.c \
		$(ARCH_DIR)/drivers/hal.c
