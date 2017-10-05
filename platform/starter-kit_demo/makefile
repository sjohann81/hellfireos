APP = app/malloc_test
ARCH = mips/pic32mz_starter-kit

SERIAL_BAUD=57600
SERIAL_DEVICE=/dev/ttyACM0

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
include $(SRC_DIR)/net/ustack.mak
include $(SRC_DIR)/sys/kernel.mak
include $(SRC_DIR)/$(APP)/app.mak

INC_DIRS += -I $(SRC_DIR)/lib/include -I $(SRC_DIR)/sys/include -I $(SRC_DIR)/net/include
USTACKFLAGS = -DUSTACK \
	-DMYIP_1=192 -DMYIP_2=168 -DMYIP_3=1 -DMYIP_4=10 \
	-DMYNM_1=255 -DMYNM_2=255 -DMYNM_3=255 -DMYNM_4=0 \
	-DMYGW_1=192 -DMYGW_2=168 -DMYGW_3=1 -DMYGW_4=2
CFLAGS += -DCPU_ARCH=$(CPU_ARCH) -DMAX_TASKS=$(MAX_TASKS) -DMEM_ALLOC=$(MEM_ALLOC) -DHEAP_SIZE=$(HEAP_SIZE) -DMUTEX_TYPE=$(MUTEX_TYPE) -DFLOATING_POINT=$(FLOATING_POINT) -DKERNEL_LOG=$(KERNEL_LOG) -DTERM_BAUD=$(SERIAL_BAUD) $(USTACKFLAGS)

serial:
	stty ${SERIAL_BAUD} raw cs8 -parenb -crtscts clocal cread ignpar ignbrk -ixon -ixoff -ixany -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke -F ${SERIAL_DEVICE}

load: serial
	pic32prog -d ${SERIAL_DEVICE} test.hex 

debug: serial
	cat ${SERIAL_DEVICE}

image: hal libc ustack kernel app
	rm eth_enc28j60.o
	$(LD) $(LDFLAGS) -T$(LINKER_SCRIPT) -Map image.map -o image.elf *.o
	$(DUMP) --disassemble --reloc image.elf > image.lst
	$(DUMP) -h image.elf > image.sec
	$(DUMP) -s image.elf > image.cnt
	$(OBJ) -O binary image.elf image.bin
	$(OBJ) -O ihex --change-addresses=0x80000000 image.elf image.hex
	$(SIZE) image.elf
	hexdump -v -e '4/1 "%02x" "\n"' image.bin > image.txt
dump:
	tcpdump -l -n -S -XX -s 0 -vv -e -i eth0

clean:
	rm -rf *.o *~ *.elf *.bin *.cnt *.lst *.sec *.txt *.hex *.map

