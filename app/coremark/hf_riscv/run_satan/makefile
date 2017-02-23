LIBC_PATH=../../../../hf_risc_test
BENCH_PATH=../..
CORE_PATH = ~/Dropbox/stuff/stuff/hf_risc

BAUDRATE=57600
F_CLK=25000000
SERIAL_DEV=/dev/ttyUSB0

# more aggressive optimizations, also turns -frename-registers on by default.
CFLAGS_XTREME = -funroll-all-loops -fgcse-sm -finline-limit=500 -fno-schedule-insns
CFLAGS_COREMARK = -Dee_printf=printf -DPERFORMANCE_RUN=1 -DITERATIONS=600

#remove unreferenced functions
CFLAGS_STRIP = -fdata-sections -ffunction-sections
LDFLAGS_STRIP = --gc-sections

GCC_RISCV_S = riscv64-unknown-elf-gcc -march=RV32I -O3 -c -msoft-float -fshort-double -ffreestanding -nostdlib -ffixed-s10 -ffixed-s11 -I$(BENCH_PATH)/ -I$(BENCH_PATH)/hf_risc -I $(LIBC_PATH)/include -DCPU_SPEED=${F_CLK} -DLITTLE_ENDIAN $(CFLAGS_STRIP) $(CFLAGS_COREMARK) $(CFLAGS_XTREME) -DDEBUG_PORT #-ffixed-s10 -ffixed-s11 -nostdinc -fno-builtin
GCC_RISCV = riscv64-unknown-elf-gcc -march=RV32I -O2 -c -msoft-float -fshort-double -ffreestanding -nostdlib -ffixed-s10 -ffixed-s11 -I$(BENCH_PATH)/ -I$(BENCH_PATH)/hf_risc -I $(LIBC_PATH)/include -DCPU_SPEED=${F_CLK} -DLITTLE_ENDIAN $(CFLAGS_STRIP) $(CFLAGS_COREMARK)  #-ffixed-s10 -ffixed-s11 -nostdinc -fno-builtin
AS_RISCV = riscv64-unknown-elf-as -m32 -msoft-float
LD_RISCV = riscv64-unknown-elf-ld -melf32lriscv $(LDFLAGS_STRIP)
DUMP_RISCV = riscv64-unknown-elf-objdump -Mno-aliases
READ_RISCV = riscv64-unknown-elf-readelf
OBJ_RISCV = riscv64-unknown-elf-objcopy
SIZE_RISCV = riscv64-unknown-elf-size

all:

serial:
	stty ${BAUDRATE} raw cs8 -parenb -crtscts clocal cread ignpar ignbrk -ixon -ixoff -ixany -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke -F ${SERIAL_DEV}

load: serial
	cp code.bin ${SERIAL_DEV}

debug: serial
	cat ${SERIAL_DEV}

coremark_sim:
	$(AS_RISCV) -o crt0v.o $(LIBC_PATH)/crt0v.s
	$(GCC_RISCV_S) -o core_list_join.o $(BENCH_PATH)/core_list_join.c
	$(GCC_RISCV_S) -o core_main.o $(BENCH_PATH)/core_main.c -DFLAGS_STR=\"'${GCC_RISCV_S}'\"
	$(GCC_RISCV_S) -o core_matrix.o $(BENCH_PATH)/core_matrix.c
	$(GCC_RISCV_S) -o core_state.o $(BENCH_PATH)/core_state.c
	$(GCC_RISCV_S) -o core_util.o $(BENCH_PATH)/core_util.c
	$(GCC_RISCV_S) -o libc.o $(LIBC_PATH)/libc.c
	$(GCC_RISCV_S) -o core_portme.o $(BENCH_PATH)/hf_risc/core_portme.c
	$(LD_RISCV) -T$(LIBC_PATH)/hf_riscv.ld -Map test.map -N -o test.axf \
		crt0v.o core_list_join.o core_main.o core_matrix.o core_state.o core_util.o libc.o core_portme.o
	$(DUMP_RISCV) --disassemble --reloc test.axf > test.lst
	$(DUMP_RISCV) -h test.axf > test.sec
	$(DUMP_RISCV) -s test.axf > test.cnt
	$(OBJ_RISCV) -O binary test.axf test.bin
	$(SIZE_RISCV) test.axf
	mv test.axf code.axf
	mv test.bin code.bin
	mv test.lst code.lst
	mv test.sec code.sec
	mv test.cnt code.cnt
	mv test.map code.map
	hexdump -v -e '4/1 "%02x" "\n"' code.bin > code.txt
	mkdir coremark
	mv code.lst coremark
	mv code.txt coremark
	
coremark:
	$(AS_RISCV) -o crt0v.o $(LIBC_PATH)/crt0v.s
	$(GCC_RISCV) -o core_list_join.o $(BENCH_PATH)/core_list_join.c
	$(GCC_RISCV) -o core_main.o $(BENCH_PATH)/core_main.c -DFLAGS_STR=\"'${GCC_RISCV}'\"
	$(GCC_RISCV) -o core_matrix.o $(BENCH_PATH)/core_matrix.c
	$(GCC_RISCV) -o core_state.o $(BENCH_PATH)/core_state.c
	$(GCC_RISCV) -o core_util.o $(BENCH_PATH)/core_util.c
	$(GCC_RISCV) -o libc.o $(LIBC_PATH)/libc.c
	$(GCC_RISCV) -o core_portme.o $(BENCH_PATH)/hf_risc/core_portme.c
	$(LD_RISCV) -T$(LIBC_PATH)/hf_riscv.ld -Map test.map -N -o test.axf \
		crt0v.o core_list_join.o core_main.o core_matrix.o core_state.o core_util.o libc.o core_portme.o
	$(DUMP_RISCV) --disassemble --reloc test.axf > test.lst
	$(DUMP_RISCV) -h test.axf > test.sec
	$(DUMP_RISCV) -s test.axf > test.cnt
	$(OBJ_RISCV) -O binary test.axf test.bin
	$(SIZE_RISCV) test.axf
	mv test.axf code.axf
	mv test.bin code.bin
	mv test.lst code.lst
	mv test.sec code.sec
	mv test.cnt code.cnt
	mv test.map code.map
	hexdump -v -e '4/1 "%02x" "\n"' code.bin > code.txt
	mkdir coremark
	mv code.lst coremark
	mv code.txt coremark

run:
	-cp ../../../../sim/boot.txt .
	-cp ./coremark/*.txt .
	-vsim -c -do "set CORE_PATH $(CORE_PATH); do $(CORE_PATH)/sim/satan/sim.do"
	-mv debug.txt coremark
	-rm code.txt

veryclean:
	-rm -rf *.o *.axf *.map *.lst *.sec *.cnt *.txt *.bin *~
	-rm -rf *~ transcript modelsim.ini work *.txt *.wlf
	-rm -rf coremark

