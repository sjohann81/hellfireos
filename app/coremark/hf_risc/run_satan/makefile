LIBC_PATH=../../../../hf_risc_test
BENCH_PATH=../..
CORE_PATH = ~/Dropbox/stuff/stuff/hf_risc

BAUDRATE=57600
SERIAL_DEV=/dev/ttyUSB0

CFLAGS_FEW_REGS = -ffixed-t0 -ffixed-t1 -ffixed-t2 -ffixed-t3 -ffixed-t4 -ffixed-t5 -ffixed-t6 -ffixed-t7 -ffixed-s0 -ffixed-s1 -ffixed-s2 -ffixed-s3 -ffixed-s4 -ffixed-s5 -ffixed-s6 -ffixed-s7
CFLAGS_FEW_FEW_REGS = -ffixed-t8 -ffixed-t9 -ffixed-a0 -ffixed-a1 -ffixed-v0 -ffixed-v1 -ffixed-fp -ffixed-gp
CFLAGS_NO_HW_MULDIV = -mnohwmult -mnohwdiv -ffixed-lo -ffixed-hi
CFLAGS_NO_HW_DIV = -mnohwdiv
# recycle $gp as a temporary register, do not put instructions on the BDS
CFLAGS_OTHER = -fcall-used-gp -fno-delayed-branch
# more aggressive optimizations, also turns -frename-registers on by default.
CFLAGS_XTREME = -funroll-all-loops -fgcse-sm -finline-limit=500 -fno-schedule-insns
CFLAGS_COREMARK = -Dee_printf=printf -DPERFORMANCE_RUN=1 -DITERATIONS=1600

#remove unreferenced functions
CFLAGS_STRIP = -fdata-sections -ffunction-sections
LDFLAGS_STRIP = --gc-sections

GCC_MIPS_S = mips-elf-gcc -O3 -c -mips2 -mno-branch-likely -mpatfree -mfix-r4000 -mno-check-zero-division -msoft-float -fshort-double -ffreestanding -nostdlib -fomit-frame-pointer -G 8 -I$(BENCH_PATH)/ -I$(BENCH_PATH)/hf_risc -I $(LIBC_PATH)/include -DCPU_SPEED=25000000 -DBIG_ENDIAN $(CFLAGS_STRIP) $(CFLAGS_COREMARK) $(CFLAGS_XTREME) $(CFLAGS_NO_HW_MULDIV) #-fPIC -mabicalls #$(CFLAGS_FEW_REGS) -DDEBUG_PORT

GCC_MIPS = mips-elf-gcc -O2 -c -mips2 -mno-branch-likely -mpatfree -mfix-r4000 -mno-check-zero-division -msoft-float -fshort-double -ffreestanding -nostdlib -fomit-frame-pointer -G 8 -I$(BENCH_PATH)/ -I$(BENCH_PATH)/hf_risc -I $(LIBC_PATH)/include -DCPU_SPEED=25000000 -DBIG_ENDIAN $(CFLAGS_STRIP) $(CFLAGS_COREMARK) $(CFLAGS_NO_HW_DIV) #-fPIC -mabicalls #$(CFLAGS_FEW_REGS) -DDEBUG_PORT

AS_MIPS = mips-elf-as -mips1
LD_MIPS = mips-elf-ld -mips1 $(LDFLAGS_STRIP)
DUMP_MIPS = mips-elf-objdump
READ_MIPS = mips-elf-readelf
OBJ_MIPS = mips-elf-objcopy
SIZE_MIPS = mips-elf-size

all:

serial:
	stty ${BAUDRATE} raw cs8 -parenb -crtscts clocal cread ignpar ignbrk -ixon -ixoff -ixany -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke -F ${SERIAL_DEV}

load: serial
	cp code.bin ${SERIAL_DEV}

debug: serial
	cat ${SERIAL_DEV}

coremark_sim:
	$(AS_MIPS) -o crt0.o $(LIBC_PATH)/crt0.s
	$(GCC_MIPS_S) -o core_list_join.o $(BENCH_PATH)/core_list_join.c
	$(GCC_MIPS_S) -o core_main.o $(BENCH_PATH)/core_main.c -DFLAGS_STR=\"'${GCC_MIPS_S}'\"
	$(GCC_MIPS_S) -o core_matrix.o $(BENCH_PATH)/core_matrix.c
	$(GCC_MIPS_S) -o core_state.o $(BENCH_PATH)/core_state.c
	$(GCC_MIPS_S) -o core_util.o $(BENCH_PATH)/core_util.c
	$(GCC_MIPS_S) -o libc.o $(LIBC_PATH)/libc.c
	$(GCC_MIPS_S) -o core_portme.o $(BENCH_PATH)/hf_risc/core_portme.c
	$(LD_MIPS) -T$(LIBC_PATH)/hf_risc.ld -Map test.map -N -o test.axf \
		crt0.o core_list_join.o core_main.o core_matrix.o core_state.o core_util.o libc.o core_portme.o
	$(DUMP_MIPS) --disassemble --reloc test.axf > test.lst
	$(DUMP_MIPS) -h test.axf > test.sec
	$(DUMP_MIPS) -s test.axf > test.cnt
	$(OBJ_MIPS) -O binary test.axf test.bin
	$(SIZE_MIPS) test.axf
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
	$(AS_MIPS) -o crt0.o $(LIBC_PATH)/crt0.s
	$(GCC_MIPS) -o core_list_join.o $(BENCH_PATH)/core_list_join.c
	$(GCC_MIPS) -o core_main.o $(BENCH_PATH)/core_main.c -DFLAGS_STR=\"'${GCC_MIPS}'\"
	$(GCC_MIPS) -o core_matrix.o $(BENCH_PATH)/core_matrix.c
	$(GCC_MIPS) -o core_state.o $(BENCH_PATH)/core_state.c
	$(GCC_MIPS) -o core_util.o $(BENCH_PATH)/core_util.c
	$(GCC_MIPS) -o libc.o $(LIBC_PATH)/libc.c
	$(GCC_MIPS) -o core_portme.o $(BENCH_PATH)/hf_risc/core_portme.c
	$(LD_MIPS) -T$(LIBC_PATH)/hf_risc.ld -Map test.map -N -o test.axf \
		crt0.o core_list_join.o core_main.o core_matrix.o core_state.o core_util.o libc.o core_portme.o
	$(DUMP_MIPS) --disassemble --reloc test.axf > test.lst
	$(DUMP_MIPS) -h test.axf > test.sec
	$(DUMP_MIPS) -s test.axf > test.cnt
	$(OBJ_MIPS) -O binary test.axf test.bin
	$(SIZE_MIPS) test.axf
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

