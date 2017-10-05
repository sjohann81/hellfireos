APP = app/noc_test4
ARCH = mips/plasma

CPU_ARCH = \"$(ARCH)\"
MAX_TASKS = 30
MUTEX_TYPE = 0
MEM_ALLOC = 3
HEAP_SIZE = 500000
FLOATING_POINT = 0
KERNEL_LOG = 0

SRC_DIR = $(CURDIR)/../..

include $(SRC_DIR)/arch/$(ARCH)/arch.mak
include $(SRC_DIR)/lib/lib.mak
include $(SRC_DIR)/drivers/noc.mak
include $(SRC_DIR)/sys/kernel.mak
include $(SRC_DIR)/$(APP)/app.mak

INC_DIRS += -I $(SRC_DIR)/lib/include -I $(SRC_DIR)/sys/include -I $(SRC_DIR)/drivers/noc/include
CFLAGS += -DCPU_ID=$(CORE) -DCPU_ARCH=$(CPU_ARCH) -DMAX_TASKS=$(MAX_TASKS) -DMEM_ALLOC=$(MEM_ALLOC) -DHEAP_SIZE=$(HEAP_SIZE) -DMUTEX_TYPE=$(MUTEX_TYPE) -DFLOATING_POINT=$(FLOATING_POINT) -DKERNEL_LOG=$(KERNEL_LOG) $(NOC_FLAGS) -DDEBUG_PORT

CORE := 0
CORE_LIST = 0 1 2 3 4 5 6 7 8
NOC_FLAGS = -DNOC_INTERCONNECT -DNOC_WIDTH=3 -DNOC_HEIGHT=3 -DNOC_PACKET_SIZE=64 -DNOC_PACKET_SLOTS=64

images: 
	make hal
	make libc
	for i in $(CORE_LIST) ; do \
		echo "Building image for core $$i.."; \
		make noc CORE="$$i" && \
		make kernel CORE="$$i" && \
		make app CORE="$$i" && \
		$(LD) $(LDFLAGS) -T$(LINKER_SCRIPT) -o code$$i.elf *.o && \
		$(DUMP) --disassemble --reloc code$$i.elf > code$$i.lst && \
		$(DUMP) -h code$$i.elf > code$$i.sec && \
		$(DUMP) -s code$$i.elf > code$$i.cnt && \
		$(OBJ) -O binary code$$i.elf code$$i.bin && \
		$(SIZE) code$$i.elf && \
		hexdump -v -e '4/1 "%02x" "\n"' code$$i.bin > code$$i.txt \
	;done

clean:
	rm -rf *.o *~ *.elf *.bin *.cnt *.lst *.sec *.txt

