APP = app/queues2
ARCH = mips/pic32mz_chipkit-wifire

SERIAL_BAUD=57600
SERIAL_DEVICE=/dev/ttyUSB0

CPU_ARCH = \"$(ARCH)\"
MAX_TASKS = 30
MUTEX_TYPE = 0
MEM_ALLOC = 3
HEAP_SIZE = 300000
FLOATING_POINT = 0
KERNEL_LOG = 0

SRC_DIR = $(CURDIR)/../..

include $(SRC_DIR)/arch/$(ARCH)/arch.mak
include $(SRC_DIR)/lib/lib.mak
include $(SRC_DIR)/sys/kernel.mak
include $(SRC_DIR)/$(APP)/app.mak

INC_DIRS += -I $(SRC_DIR)/lib/include -I $(SRC_DIR)/sys/include
CFLAGS += -DCPU_ARCH=$(CPU_ARCH) -DMAX_TASKS=$(MAX_TASKS) -DMEM_ALLOC=$(MEM_ALLOC) -DHEAP_SIZE=$(HEAP_SIZE) -DMUTEX_TYPE=$(MUTEX_TYPE) -DFLOATING_POINT=$(FLOATING_POINT) -DKERNEL_LOG=$(KERNEL_LOG) -DTERM_BAUD=$(SERIAL_BAUD)

serial:
	stty ${SERIAL_BAUD} raw cs8 -parenb -crtscts clocal cread ignpar ignbrk -ixon -ixoff -ixany -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke -F ${SERIAL_DEVICE}

load: serial
	pic32prog -d ${SERIAL_DEVICE} test.hex 

debug: serial
	cat ${SERIAL_DEVICE}

image: hal libc kernel app
	$(LD) $(LDFLAGS) -T$(LINKER_SCRIPT) -Map image.map -o image.elf *.o
	$(DUMP) --disassemble --reloc image.elf > image.lst
	$(DUMP) -h image.elf > image.sec
	$(DUMP) -s image.elf > image.cnt
	$(OBJ) -O binary image.elf image.bin
	$(OBJ) -O ihex --change-addresses=0x80000000 image.elf image.hex
	$(SIZE) image.elf
	hexdump -v -e '4/1 "%02x" "\n"' image.bin > image.txt

clean:
	rm -rf *.o *~ *.elf *.bin *.cnt *.lst *.sec *.txt *.hex *.map

