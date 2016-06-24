APP = app/noc_test
ARCH = mips/plasma

CPU_ARCH = \"$(ARCH)\"
MAX_TASKS = 30
MUTEX_TYPE = 0
MEM_ALLOC = 1
HEAP_SIZE = 500000
FLOATING_POINT = 0

SRC_DIR = $(CURDIR)/../..

include $(SRC_DIR)/arch/$(ARCH)/arch.mak
include $(SRC_DIR)/lib/lib.mak
include $(SRC_DIR)/drivers/noc.mak
include $(SRC_DIR)/sys/kernel.mak
include $(SRC_DIR)/$(APP)/app.mak

INC_DIRS += -I $(SRC_DIR)/lib/include -I $(SRC_DIR)/sys/include -I $(SRC_DIR)/drivers/noc/include
CFLAGS += -DCPU_ID=$(CORE) -DCPU_ARCH=$(CPU_ARCH) -DMAX_TASKS=$(MAX_TASKS) -DMEM_ALLOC=$(MEM_ALLOC) -DHEAP_SIZE=$(HEAP_SIZE) -DMUTEX_TYPE=$(MUTEX_TYPE) -DFLOATING_POINT=$(FLOATING_POINT) $(NOC_FLAGS)

CORE := 0
CORE_LIST = 0 1 2 3 4 5
NOC_FLAGS = -DNOC_INTERCONNECT -DCPU_CORES=6 -DNOC_WIDTH=3 -DNOC_HEIGHT=2 -DNOC_PACKET_SIZE=64 -DNOC_PACKET_SLOTS=64 -DNOC_TIMEOUT=1000

images: 
	make hal
	make libc
	for i in $(CORE_LIST) ; do \
		echo "Building image for core $$i.."; \
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

