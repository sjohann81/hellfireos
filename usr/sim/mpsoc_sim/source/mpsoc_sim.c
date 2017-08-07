#include <stdio.h>
#include <string.h>
#include <time.h>
#include "noc.h"

/*
	SIMULATOR
*/

#define MAX_N_CORES			257		// max number of cores + 1
#define MEM_SIZE			(1024*1024)
#define CPU_NETWORK_CLK_RATIO		10		// freq ratio between cpus and interconnect. 

#define RAM_INTERNAL_BASE		0x00000000
#define RAM_EXTERNAL_BASE		0x10000000
#define MISC_BASE			0x20000000
#define UART_WRITE			0x20000000
#define UART_READ			0x20000000
#define IRQ_MASK			0x20000010
#define IRQ_STATUS			0x20000020
#define GPIO0_OUT			0x20000030
#define GPIO1_OUT			0x20000040
#define GPIOA_IN			0x20000050
#define COUNTER_REG			0x20000060
#define NOC_READ			0x20000070	/*READ*/
#define NOC_WRITE			0x20000080	/*WRITE*/
#define NOC_STATUS			0x20000090	/*STATUS*/
#define FREQUENCY_REG			0x200000A0
#define TICK_TIME_REG			0x200000B0
#define ENERGY_MONITOR			0x200000C0	/* not implemented */
#define OUT_FACILITY			0x200000D0	/* not implemented yet */
#define LOG_FACILITY			0x200000E0
#define EXIT_TRAP			0x200000F0

#define IRQ_UART_READ_AVAILABLE		0x01
#define IRQ_UART_WRITE_AVAILABLE	0x02
#define IRQ_COUNTER18_NOT		0x04
#define IRQ_COUNTER18			0x08
#define IRQ_GPIO30_NOT			0x10
#define IRQ_GPIO31_NOT			0x20
#define IRQ_GPIO30			0x40
#define IRQ_GPIO31			0x80
#define IRQ_NOC_READ			0x100

#define ENERGY_PER_CYCLE_ARITHMETIC	0.00000000160864
#define ENERGY_PER_CYCLE_BRANCH_JUMP	0.00000000239897
#define ENERGY_PER_CYCLE_LOAD_STORE	0.00000000169180
#define ENERGY_PER_CYCLE_LOGIC		0.00000000251948
#define ENERGY_PER_CYCLE_MOVE		0.00000000192844
#define ENERGY_PER_CYCLE_SHIFT		0.00000000292796
#define BUS_ENERGY_PER_CYCLE_IDLE	0.00000000004689
#define BUS_ENERGY_PER_PACKET		0.00000000812808

#define UART_DELAY ((reference_clock / 57600) * 10)	// uart delay @ 57600bps (cycles)

#define ntohs(A) ( ((A)>>8) | (((A)&0xff)<<8) )
#define htons(A) ntohs(A)
#define ntohl(A) ( ((A)>>24) | (((A)&0xff0000)>>8) | (((A)&0xff00)<<8) | ((A)<<24) )
#define htonl(A) ntohl(A)
#define ROUND_INT(x) ((x<0))?((int)(x-0.5)):((int)(x+0.5))

typedef struct {
	int r[32];
	int pc, pc_next, epc;
	unsigned int hi;
	unsigned int lo;
	int status;
	int userMode;
	int processId;
	int exceptionId;
	int faultAddr;
	int irqStatus;
	int skip;
	unsigned char *mem;
	int wakeup;
	int big_endian;
	char jump_or_branch;
	char no_execute_branch_delay_slot;
} State;

// implemented MIPS opcodes
static char *opcode_string[]={
	"SPECIAL","REGIMM","J","JAL","BEQ","BNE","BLEZ","BGTZ",
	"ADDI","ADDIU","SLTI","SLTIU","ANDI","ORI","XORI","LUI",
	"COP0","?","?","?","BEQL","BNEL","BLEZL","BGTZL",
	"?","?","?","?","?","?","?","?",
	"LB","LH","?","LW","LBU","LHU","?","?",
	"SB","SH","?","SW","?","?","?","?",
	"LL","?","?","?","?","?","?","?",
	"SC","?","?","?","?","?","?","?"
};

static char *special_string[]={
	"SLL","?","SRL","SRA","SLLV","?","SRLV","SRAV",
	"JR","JALR","MOVZ","MOVN","?","?","?","?",
	"MFHI","MTHI","MFLO","MTLO","?","?","?","?",
	"MULT","MULTU","DIV","DIVU","?","?","?","?",
	"ADD","ADDU","SUB","SUBU","AND","OR","XOR","NOR",
	"?","?","SLT","SLTU","?","DADDU","?","?",
	"?","?","?","?","?","?","?","?",
	"?","?","?","?","?","?","?","?"
};

static char *regimm_string[]={
	"BLTZ","BGEZ","BLTZL","BGEZL","?","?","?","?",
	"?","?","?","?","?","?","?","?",
	"BLTZAL","BGEZAL","BLTZALL","BGEZALL","?","?","?","?",
	"?","?","?","?","?","?","?","?"
};

static char *class_arith[]={
	"ADD","ADDI","ADDIU","ADDU","DIV","DIVU","MULT","MULTU",
	"SLT","SLTI","SLTIU","SLTU","SUB","SUBU","DADDU"
};

static char *class_branch[]={
	"BLTZL","BLTZALL","BEQ","BGEZ","BGEZAL","BGTZ","BLEZ","BLTZ",
	"BLTZAL","BNE","J","JAL","JALR","JR","BEQL","BNEL",
	"BLEZL","BGTZL","BGEZL","BGEZALL"
};

static char *class_loadstore[]={
	"LB","LBU","LH","LHU","LW","SB","SH","SW",
	"LL","SC"
};

static char *class_logic[]={
	"AND","ANDI","LUI","NOR","OR","ORI","XOR","XORI"
};

static char *class_move[]={
	"MFHI","MFLO","MTHI","MTLO","COP0","MOVZ","MOVN"
};

static char *class_shift[]={
	"SLL","SLLV","SRA","SRAV","SRL","SRLV"
};

/*
	SOME NOC GLOBAL VARIABLES
*/
unsigned char is_sending[MAX_N_CORES]; // necessary to synchronize with noc simulator 
unsigned char is_reading[MAX_N_CORES];
int flits_remaining[MAX_N_CORES]; 
extern Router *routers;
extern NetworkInterface *network_interfaces;
extern Core *cores;

unsigned int reference_clock=25000000;
static int n_cores=0;
static int big_endian=1;

unsigned int HWMemory[5][MAX_N_CORES];
unsigned char SRAM[MEM_SIZE*MAX_N_CORES];
unsigned int GPIOAIN[MAX_N_CORES];
unsigned int GPIO0OUT[MAX_N_CORES];

unsigned long long cpu_cycles[MAX_N_CORES];
unsigned int ins_counter_op[0x40][MAX_N_CORES], ins_counter_func[0x40][MAX_N_CORES], ins_counter_rt[0x20][MAX_N_CORES];
unsigned long long max_cycles=-1;
char sim_metric = '\0';
int uart_delay[MAX_N_CORES];
double est_energy[MAX_N_CORES];
unsigned int ins_counter[MAX_N_CORES];
unsigned int ins_class_counter[6][MAX_N_CORES];
unsigned int io_counter[MAX_N_CORES];
unsigned char brkpt[MAX_N_CORES];
double bus_est_energy;
unsigned int flits_sent[MAX_N_CORES];
unsigned int flits_received[MAX_N_CORES];
unsigned int broadcasts[MAX_N_CORES];
char logout_string[] = "./reports/logout\0\0\0\0\0\0\0\0\0\0\0";
char outout_string[] = "./reports/out\0\0\0\0\0\0\0\0\0\0\0";
FILE *log_out[MAX_N_CORES], *out_out[MAX_N_CORES];

char *itoa(unsigned int num){
	static char buf[12];
	static char buf2[12];
	int i,j;

	if (num==0){
		buf[0] = '0';
		buf[1] = '\0';
		return &buf[0];
	}

	for(i=0;i<11 && num!=0;i++){
		buf[i]=(char)((num%10)+'0');
		num/=10;
	}
	buf2[i] = '\0';
	j = 0;
	i--;
	for(;i>=0;i--){
		buf2[i]=buf[j];
		j++;
	}
	return &buf2[0];
}


static int show_cpu_stats(unsigned char *output, int cpu_n){
	FILE *rpt_ptr;
	int i,j,k;

	rpt_ptr = fopen(output, "w");
	if (rpt_ptr == NULL){
		printf("\nCould not open %s for writing.\n", *output);
		fflush(stdout);

		return (-1);
	}

	fprintf(rpt_ptr, "\nCode Execution Report - Core %d", cpu_n);
	fprintf(rpt_ptr, "\n\nCPU cycles: %ld",cpu_cycles[cpu_n]);
	fprintf(rpt_ptr, "\nWCET: %.04fms", (((double)cpu_cycles[cpu_n] / (double)reference_clock))*1000);
	fprintf(rpt_ptr, "\nEstimated energy consumption: %lfJ (20587 gates Plasma CPU core, CMOS TSMC 0.35um)", est_energy[cpu_n]);
	fprintf(rpt_ptr, "\n\nInstructions (MIPS I instruction set):\n");
	j=0;
	for(i=2;i<64;i++){
		if(opcode_string[i] != "?"){
			ins_counter[cpu_n]+=ins_counter_op[i][cpu_n];
			j++;
			fprintf(rpt_ptr, "%8s: %10d; ", opcode_string[i],ins_counter_op[i][cpu_n]);
			if (j==4){
				j=0;
				fprintf(rpt_ptr, "\n");
			}
			for(k=0;k<15;k++){
				if(strcmp(class_arith[k],opcode_string[i])==0){
					ins_class_counter[0][cpu_n]+=ins_counter_op[i][cpu_n];
					break;
				}
			}
			for(k=0;k<20;k++){
				if(strcmp(class_branch[k],opcode_string[i])==0){
					ins_class_counter[1][cpu_n]+=ins_counter_op[i][cpu_n];
					break;
				}
			}
			for(k=0;k<10;k++){
				if(strcmp(class_loadstore[k],opcode_string[i])==0){
					ins_class_counter[2][cpu_n]+=ins_counter_op[i][cpu_n];
					break;
				}
			}
			for(k=0;k<8;k++){
				if(strcmp(class_logic[k],opcode_string[i])==0){
					ins_class_counter[3][cpu_n]+=ins_counter_op[i][cpu_n];
					break;
				}
			}
			for(k=0;k<7;k++){
				if(strcmp(class_move[k],opcode_string[i])==0){
					ins_class_counter[4][cpu_n]+=ins_counter_op[i][cpu_n];
					break;
				}
			}
			for(k=0;k<6;k++){
				if(strcmp(class_shift[k],opcode_string[i])==0){
					ins_class_counter[5][cpu_n]+=ins_counter_op[i][cpu_n];
					break;
				}
			}
		}
	}
	for(i=0;i<64;i++){
		if(special_string[i] != "?"){
			ins_counter[cpu_n]+=ins_counter_func[i][cpu_n];
			j++;
			fprintf(rpt_ptr, "%8s: %10d; ", special_string[i], ins_counter_func[i][cpu_n]);
			if (j==4){
				j=0;
				fprintf(rpt_ptr, "\n");
			}
			for(k=0;k<15;k++){
				if(strcmp(class_arith[k],special_string[i])==0){
					ins_class_counter[0][cpu_n]+=ins_counter_func[i][cpu_n];
					break;
				}
			}
			for(k=0;k<20;k++){
				if(strcmp(class_branch[k],special_string[i])==0){
					ins_class_counter[1][cpu_n]+=ins_counter_func[i][cpu_n];
					break;
				}
			}
			for(k=0;k<10;k++){
				if(strcmp(class_loadstore[k],special_string[i])==0){
					ins_class_counter[2][cpu_n]+=ins_counter_func[i][cpu_n];
					break;
				}
			}
			for(k=0;k<8;k++){
				if(strcmp(class_logic[k],special_string[i])==0){
					ins_class_counter[3][cpu_n]+=ins_counter_func[i][cpu_n];
					break;
				}
			}
			for(k=0;k<7;k++){
				if(strcmp(class_move[k],special_string[i])==0){
					ins_class_counter[4][cpu_n]+=ins_counter_func[i][cpu_n];
					break;
				}
			}
			for(k=0;k<6;k++){
				if(strcmp(class_shift[k],special_string[i])==0){
					ins_class_counter[5][cpu_n]+=ins_counter_func[i][cpu_n];
					break;
				}
			}
		}
	}
	for(i=0;i<32;i++){
		if(regimm_string[i] != "?"){
			ins_counter[cpu_n]+=ins_counter_rt[i][cpu_n];
			j++;
			fprintf(rpt_ptr, "%8s: %10d; ", regimm_string[i], ins_counter_rt[i][cpu_n]);
			if (j==4){
				j=0;
				fprintf(rpt_ptr, "\n");
			}
			for(k=0;k<15;k++){
				if(strcmp(class_arith[k],regimm_string[i])==0){
					ins_class_counter[0][cpu_n]+=ins_counter_rt[i][cpu_n];
					break;
				}
			}
			for(k=0;k<20;k++){
				if(strcmp(class_branch[k],regimm_string[i])==0){
					ins_class_counter[1][cpu_n]+=ins_counter_rt[i][cpu_n];
					break;
				}
			}
			for(k=0;k<10;k++){
				if(strcmp(class_loadstore[k],regimm_string[i])==0){
					ins_class_counter[2][cpu_n]+=ins_counter_rt[i][cpu_n];
					break;
				}
			}
			for(k=0;k<8;k++){
				if(strcmp(class_logic[k],regimm_string[i])==0){
					ins_class_counter[3][cpu_n]+=ins_counter_rt[i][cpu_n];
					break;
				}
			}
			for(k=0;k<7;k++){
				if(strcmp(class_move[k],regimm_string[i])==0){
					ins_class_counter[4][cpu_n]+=ins_counter_rt[i][cpu_n];
					break;
				}
			}
			for(k=0;k<6;k++){
				if(strcmp(class_shift[k],regimm_string[i])==0){
					ins_class_counter[5][cpu_n]+=ins_counter_rt[i][cpu_n];
					break;
				}
			}
		}
	}
	fprintf(rpt_ptr, "\n\nInstructions executed: %d", ins_counter[cpu_n]);
	fprintf(rpt_ptr, "\nEffective instructions per cycle (IPC): %f", ((double)ins_counter[cpu_n]/(double)cpu_cycles[cpu_n]));
	fprintf(rpt_ptr, "\nI/O wait cycles: %d",io_counter[cpu_n]);
	fprintf(rpt_ptr, "\n\nInstructions executed from each class:");
	fprintf(rpt_ptr, "\nArithmetic:  %12d (%f%%)",ins_class_counter[0][cpu_n], (((float)ins_class_counter[0][cpu_n]/(float)ins_counter[cpu_n])*100));
	fprintf(rpt_ptr, "\nBranch/Jump: %12d (%f%%)",ins_class_counter[1][cpu_n], (((float)ins_class_counter[1][cpu_n]/(float)ins_counter[cpu_n])*100));
	fprintf(rpt_ptr, "\nLoad/Store:  %12d (%f%%)",ins_class_counter[2][cpu_n], (((float)ins_class_counter[2][cpu_n]/(float)ins_counter[cpu_n])*100));
	fprintf(rpt_ptr, "\nLogical:     %12d (%f%%)",ins_class_counter[3][cpu_n], (((float)ins_class_counter[3][cpu_n]/(float)ins_counter[cpu_n])*100));
	fprintf(rpt_ptr, "\nMove:        %12d (%f%%)",ins_class_counter[4][cpu_n], (((float)ins_class_counter[4][cpu_n]/(float)ins_counter[cpu_n])*100));
	fprintf(rpt_ptr, "\nShift:       %12d (%f%%)",ins_class_counter[5][cpu_n], (((float)ins_class_counter[5][cpu_n]/(float)ins_counter[cpu_n])*100));

	fprintf(rpt_ptr, "\n\nFlits sent: %ld",flits_sent[cpu_n]);
	fprintf(rpt_ptr, "\nFlits received: %ld",flits_received[cpu_n]);
	fprintf(rpt_ptr, "\nBroadcasts: %ld", broadcasts[cpu_n]);

	fprintf(rpt_ptr, "\n");

	fclose(rpt_ptr);
}

static int show_mpsoc_stats(unsigned char *output){
	FILE *rpt_ptr;
	int i,j,k;
	unsigned long long cycles=0;

	rpt_ptr = fopen(output, "w");
	if (rpt_ptr == NULL){
		printf("\nCould not open %s for writing.\n", *output);
		fflush(stdout);

		return (-1);
	}

	for(i=0;i<4;i++){
		if (cpu_cycles[i] > cycles)
			cycles = cpu_cycles[i];
	}

	fprintf(rpt_ptr, "\nMPSoC Report");
	fprintf(rpt_ptr, "\n\nMPSoC cycles: %ld",cycles);
	fprintf(rpt_ptr, "\nWCET: %.04fms", (((float)cycles / (float)reference_clock))*1000);
	fprintf(rpt_ptr, "\n\nEstimated energy consumption: %lfJ", est_energy[0]+est_energy[1]+est_energy[2]+est_energy[3]+bus_est_energy);
	fprintf(rpt_ptr, "\n(%d * 20587 gates Plasma CPU core + %d gates interconnection structure, CMOS TSMC 0.35um)",n_cores , 2816*n_cores);
	for(j=0;j<n_cores;j++)
		fprintf(rpt_ptr, "\n    core %d: %lfJ",j, est_energy[j]);
	fprintf(rpt_ptr, "\n       bus: %lfJ", bus_est_energy);
	k=0;
	for(j=0;j<n_cores;j++)
		k += flits_sent[j];
	fprintf(rpt_ptr, "\n\nFlits sent: %ld",k);
	for(j=0;j<n_cores;j++)
		fprintf(rpt_ptr, "\n    core %d: %ld",j, flits_sent[j]);
	k=0;
	for(j=0;j<n_cores;j++)
		k += flits_received[j];
	fprintf(rpt_ptr, "\n\nFlits received: %ld",k);
	for(j=0;j<n_cores;j++)
		fprintf(rpt_ptr, "\n    core %d: %ld",j, flits_received[j]);
	k=0;
	for(j=0;j<n_cores;j++)
		k += broadcasts[j];
	fprintf(rpt_ptr, "\n\nBroadcasts: %ld",k);
	for(j=0;j<n_cores;j++)
		fprintf(rpt_ptr, "\n    core %d: %ld",j, flits_received[j]);
	fprintf(rpt_ptr, "\n");

	fclose(rpt_ptr);	
}
	

static int mem_read(State *s, int size, unsigned int address, int cpu_n){
	unsigned int value=0;
	unsigned int *ptr;
	
	Core *core;
	NetworkInterface *ni;
	Buffer *buffer;
	Port *port;

	switch(address){
		case UART_READ:
//			if(kbhit())
//			HWMemory[0] = getchar();
			HWMemory[2][cpu_n] &= ~IRQ_UART_READ_AVAILABLE; //clear bit
			return HWMemory[0][cpu_n];
		case GPIOA_IN:
			return GPIOAIN[cpu_n];
		case IRQ_MASK:
			return HWMemory[1][cpu_n];
		case IRQ_MASK + 4:
//			Sleep(10);
			return 0;
		case IRQ_STATUS:
//			if(kbhit())
//				HWMemory[2][cpu_n] |= IRQ_UART_READ_AVAILABLE;
			return HWMemory[2][cpu_n];
		case COUNTER_REG:
			return (unsigned int)cpu_cycles[cpu_n];
		case NOC_READ:

			if(HWMemory[2][cpu_n] & IRQ_NOC_READ)
			{
				HWMemory[2][cpu_n] &= ~IRQ_NOC_READ;
			}

			core = getCore(cpu_n);
			port = &(core->port);
			if(flits_remaining[cpu_n] == OS_PACKET_SIZE+1)
			{
				flits_remaining[cpu_n]--;
				return 0;
			}
			
			if(port->in_request == ON)
			{
				port->in_ack = ON;
				flits_remaining[cpu_n]--;
				return port->in;
			}
			
			return 0xe0000000;
			
		case NOC_STATUS:

			ni = getNetworkInterface(cpu_n);
			buffer = getBuffer(ni, PLASMA);
			return isEmpty(buffer);

		case FREQUENCY_REG:
			return HWMemory[3][cpu_n];
		case TICK_TIME_REG:
			return HWMemory[4][cpu_n];
		case LOG_FACILITY:
			return 0xa5a5a5a5;
	}

	ptr = (unsigned int *)(s->mem + (address % MEM_SIZE));

	switch(size){
		case 4:
			if(address & 3){
				printf("\nUnaligned access PC=0x%x data=0x%x :(", s->pc, address);
				fflush(stdout);
			}
			value = *(int*)ptr;
			if(big_endian)
				value = ntohl(value);
			break;
		case 2:
			value = *(unsigned short*)ptr;
			if(big_endian)
				value = ntohs((unsigned short)value);
			break;
		case 1:
			value = *(unsigned char*)ptr;
			break;
		default:
			printf("ERROR");
			fflush(stdout);
	}

	return(value);
}

static void mem_write(State *s, int size, int unsigned address, unsigned int value, FILE *std_out, int cpu_n){
	static char_count=0;
	unsigned int *ptr;
	
	Core *core;
	Port *port;	

	switch(address){
		case UART_WRITE:
			HWMemory[2][cpu_n] &= ~IRQ_UART_WRITE_AVAILABLE;
			putc(value, std_out);
			return;
		case GPIO0_OUT:
			GPIO0OUT[cpu_n] = value;
			return;
		case IRQ_MASK:
			HWMemory[1][cpu_n] = value;
			return;
		case IRQ_STATUS:
//			HWMemory[2][cpu_n] = value;
			return;
		case NOC_WRITE:
			
			core = getCore(cpu_n);
			port = &(core->port);
			is_sending[cpu_n] = ON;
			port->out = value;
			port->out_request = ON;
			port->out_ack = OFF;			
			return;
			
		case FREQUENCY_REG:
			if ((value == 25000000) || (value == 33333333) || (value == 50000000) || (value == 66666666) || (value == 100000000)){
				HWMemory[3][cpu_n] = value;
				switch (sim_metric){
					case 'c':
							break;
					case 's':	max_cycles /= reference_clock;
							max_cycles *= value;
							break;
					case 'm':
							max_cycles /= ((double)reference_clock / 1000.0);
							max_cycles *= ((double)value / 1000.0);;
							break;
					case 'u':
							max_cycles /= ((double)reference_clock / 1000000.0);
							max_cycles *= ((double)value / 1000000.0);
							break;
					case 'n':
							max_cycles /= ((double)reference_clock / 1000000000.0);
							max_cycles *= ((double)value / 1000000000.0);
							break;
					default:
							break;
				}
				reference_clock = value;
				printf("\nClock frequency reconfigured for %d MHz on core %d", (value/1000000), cpu_n);
				fflush(stdout);
			}else{
				printf("\nClock frequency NOT reconfigured on core %d", cpu_n);
				fflush(stdout);
			}
			return;
		case TICK_TIME_REG:
//			HWMemory[2][cpu_n] |= IRQ_COUNTER18_NOT;
			HWMemory[4][cpu_n] = value;
			return;
		case OUT_FACILITY:
 			fprintf(out_out[cpu_n], "%c", value);
 			return;
		case LOG_FACILITY:
			if (value == 0xFFFFFFFF){
				fprintf(log_out[cpu_n], "\n");
			}else{
				if (value == 0xFFFFFFFE){
					fprintf(log_out[cpu_n], "#");
				}else{
					if (value == 0xFFFFFFFD){
						fprintf(log_out[cpu_n], "!");
					}else{
						fprintf(log_out[cpu_n], "%d\t", value);
					}
				}
			}
			return;
		case EXIT_TRAP:
			printf("[BP, CPU %d]", cpu_n);
			fflush(stdout);
			brkpt[cpu_n] = 1;
			return;
	}

	ptr = (unsigned int *)(s->mem + (address % MEM_SIZE));

	switch(size){
		case 4:
			if(big_endian)
				value = htonl(value);
			*(int*)ptr = value;
			break;
		case 2:
			if(big_endian)
				value = htons((unsigned short)value);
			*(short*)ptr = (unsigned short)value;
			break;
		case 1:
			*(char*)ptr = (unsigned char)value;
			break;
		default:
			printf("ERROR");
			fflush(stdout);
	}
}

void mult_big_unsigned(unsigned int a, unsigned int b, unsigned int *hi, unsigned int *lo){
	unsigned int ahi, alo, bhi, blo;
	unsigned int c0, c1, c2;
	unsigned int c1_a, c1_b;

	ahi = a >> 16;
	alo = a & 0xffff;
	bhi = b >> 16;
	blo = b & 0xffff;

	c0 = alo * blo;
	c1_a = ahi * blo;
	c1_b = alo * bhi;
	c2 = ahi * bhi;

	c2 += (c1_a >> 16) + (c1_b >> 16);
	c1 = (c1_a & 0xffff) + (c1_b & 0xffff) + (c0 >> 16);
	c2 += (c1 >> 16);
	c0 = (c1 << 16) + (c0 & 0xffff);
	*hi = c2;
	*lo = c0;
}

void mult_big_signed(int a, int b, unsigned int *hi, unsigned int *lo){
	unsigned int ahi, alo, bhi, blo;
	unsigned int c0, c1, c2;
	unsigned int c1_a, c1_b;

	ahi = a >> 16;
	alo = a & 0xffff;
	bhi = b >> 16;
	blo = b & 0xffff;

	c0 = alo * blo;
	c1_a = ahi * blo;
	c1_b = alo * bhi;
	c2 = ahi * bhi;

	c2 += (c1_a >> 16) + (c1_b >> 16);
	c1 = (c1_a & 0xffff) + (c1_b & 0xffff) + (c0 >> 16);
	c2 += (c1 >> 16);
	c0 = (c1 << 16) + (c0 & 0xffff);
	*hi = c2;
	*lo = c0;
}

//execute one cycle of a Plasma CPU
void cycle(State *s, int show_mode, int cpu_n, FILE *std_out, int *pause_cycles, int *irq){
	unsigned int opcode;
	unsigned int op, rs, rt, rd, re, func, imm, target;
	int imm_shift, branch=0, lbranch=2;
	int *r=s->r;
	unsigned int *u=(unsigned int*)s->r;
	unsigned int ptr, epc, rSave;

	*pause_cycles = 0;

	if (HWMemory[2][cpu_n] & IRQ_UART_WRITE_AVAILABLE){
		if ((*irq) && (!s->jump_or_branch)){
			s->epc = s->pc+4;
//			s->pc_next = 0x10000060;	// address of simulator ISR
			s->pc_next = 0x10000050;	// address of simulator ISR
			s->no_execute_branch_delay_slot = 1;
			*irq = 0;
			s->status = 0;
		}
	}

	opcode = mem_read(s, 4, s->pc, cpu_n);
	op = (opcode >> 26) & 0x3f;
	rs = (opcode >> 21) & 0x1f;
	rt = (opcode >> 16) & 0x1f;
	rd = (opcode >> 11) & 0x1f;
	re = (opcode >> 6) & 0x1f;
	func = opcode & 0x3f;
	imm = opcode & 0xffff;
	imm_shift = (((int)(short)imm) << 2) - 4;
	target = (opcode << 6) >> 4;
	ptr = (short)imm + r[rs];
	r[0] = 0;
	if(show_mode){
		printf("%8.8lx %8.8lx ", s->pc,opcode);
		if(op == 0)
			printf("%8s ", special_string[func]);
		else if(op == 1)
			printf("%8s ", regimm_string[rt]);
		else
			printf("%8s ", opcode_string[op]);
		printf("$%2.2ld $%2.2ld $%2.2ld $%2.2ld ", rs, rt, rd, re);
		printf("%4.4lx\n", imm);
		fflush(stdout);
	}

	if(s->pc_next != s->pc + 4)
		epc |= 2;  //branch delay slot
	s->pc = s->pc_next;
	s->pc_next = s->pc_next + 4;
	if(s->skip){
		s->skip = 0;
		return;
	}
	s->jump_or_branch = 0;
	if (opcode == 0 || s->no_execute_branch_delay_slot){
		s->no_execute_branch_delay_slot = 0;
		return;
	}
	rSave = r[rt];
	switch(op){
		case 0x00:/*SPECIAL*/
			switch(func){
				case 0x00:/*SLL*/
					r[rd]=r[rt]<<re;
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_SHIFT*3;
					break;
				case 0x02:/*SRL*/
					r[rd]=u[rt]>>re;
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_SHIFT*2;
					break;
				case 0x03:/*SRA*/
					r[rd]=r[rt]>>re;
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_SHIFT*3;
					break;
				case 0x04:/*SLLV*/
					r[rd]=r[rt]<<r[rs];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_SHIFT*2;
					break;
				case 0x06:/*SRLV*/
					r[rd]=u[rt]>>r[rs];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_SHIFT*2;
					break;
				case 0x07:/*SRAV*/
					r[rd]=r[rt]>>r[rs];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_SHIFT*2;
					break;
				case 0x08:/*JR*/
					s->jump_or_branch = 1;
					s->pc_next=r[rs];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
					break;
				case 0x09:/*JALR*/
					s->jump_or_branch = 1;
					r[rd]=s->pc_next;
					s->pc_next=r[rs];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
					break;
				case 0x0a:/*MOVZ*/
					if(!r[rt]) r[rd]=r[rs];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_MOVE;
					break;  /*IV*/
				case 0x0b:/*MOVN*/
					if(r[rt]) r[rd]=r[rs];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_MOVE;
					break;  /*IV*/
				case 0x0c:/*SYSCALL*/ epc|=1; s->exceptionId=1; break;
				case 0x0d:/*BREAK*/   epc|=1; s->exceptionId=1; break;
				case 0x0f:/*SYNC*/ s->wakeup=1;              break;
				case 0x10:/*MFHI*/
					r[rd]=s->hi;
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_MOVE;
					break;
				case 0x11:/*FTHI*/
					s->hi=r[rs];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_MOVE;
					break;
				case 0x12:/*MFLO*/
					r[rd]=s->lo;
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_MOVE;
					break;
				case 0x13:/*MTLO*/
					s->lo=r[rs];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_MOVE;
					break;
				case 0x18:/*MULT*/
					mult_big_signed(r[rs],r[rt],&s->hi,&s->lo);
					*pause_cycles = 31;
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC*32;
					break;
				case 0x19:/*MULTU*/
					mult_big_unsigned(r[rs],r[rt],&s->hi,&s->lo);
					*pause_cycles = 31;
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC*32;
					break;
				case 0x1a:/*DIV*/
					s->lo=r[rs]/r[rt];
					s->hi=r[rs]%r[rt];
					*pause_cycles = 31;
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC*32;
					break;
				case 0x1b:/*DIVU*/
					s->lo=u[rs]/u[rt];
					s->hi=u[rs]%u[rt];
					*pause_cycles = 31;
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC*32;
					break;
				case 0x20:/*ADD*/
					r[rd]=r[rs]+r[rt];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC;
					break;
				case 0x21:/*ADDU*/
					r[rd]=r[rs]+r[rt];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC;
					break;
				case 0x22:/*SUB*/
					r[rd]=r[rs]-r[rt];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC;
					break;
				case 0x23:/*SUBU*/
					r[rd]=r[rs]-r[rt];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC;
					break;
				case 0x24:/*AND*/
					r[rd]=r[rs]&r[rt];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOGIC;
					break;
				case 0x25:/*OR*/
					r[rd]=r[rs]|r[rt];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOGIC;
					break;
				case 0x26:/*XOR*/
					r[rd]=r[rs]^r[rt];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOGIC*2;
					break;
				case 0x27:/*NOR*/
					r[rd]=~(r[rs]|r[rt]);
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOGIC;
					break;
				case 0x2a:/*SLT*/
					r[rd]=r[rs]<r[rt];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC*2;
					break;
				case 0x2b:/*SLTU*/
					r[rd]=u[rs]<u[rt];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC*2;
					break;
				case 0x2d:/*DADDU*/
					r[rd]=r[rs]+u[rt];
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC;
					break;
				case 0x31:/*TGEU*/ break;
				case 0x32:/*TLT*/  break;
				case 0x33:/*TLTU*/ break;
				case 0x34:/*TEQ*/  break;
				case 0x36:/*TNE*/  break;
				default:
					printf("ERROR0(*0x%x~0x%x)\n",s->pc,opcode);
					fflush(stdout);
					s->wakeup=1;
			}
			ins_counter_func[func][cpu_n]++;
			break;
		case 0x01:/*REGIMM*/
			switch(rt){
				case 0x10:/*BLTZAL*/
					r[31]=s->pc_next;
				case 0x00:/*BLTZ*/
					if (r[rs]<0){
						s->jump_or_branch = 1;
						branch=r[rs]<0;
					}
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
					break;
				case 0x11:/*BGEZAL*/
					r[31]=s->pc_next;
				case 0x01:/*BGEZ*/
					if (r[rs]>=0){
						s->jump_or_branch = 1;
						branch=r[rs]>=0;
					}
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
					break;
				case 0x12:/*BLTZALL*/
					r[31]=s->pc_next;
				case 0x02:/*BLTZL*/
					if (r[rs]<0){
						s->jump_or_branch = 1;
						lbranch=r[rs]<0;
					}
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
					break;
				case 0x13:/*BGEZALL*/
					r[31]=s->pc_next;
				case 0x03:/*BGEZL*/
					if (r[rs]>=0){
						s->jump_or_branch = 1;
						lbranch=r[rs]>=0;
					}
					est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
					break;
				default:
					printf("ERROR1\n");
					fflush(stdout);
					s->wakeup=1;
			}
			ins_counter_rt[rt][cpu_n]++;
			break;
			case 0x03:/*JAL*/
				r[31]=s->pc_next;
			case 0x02:/*J*/
				s->jump_or_branch = 1;
				s->pc_next=(s->pc&0xf0000000)|target;
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
				break;
			case 0x04:/*BEQ*/
				if (r[rs]==r[rt]){
					s->jump_or_branch = 1;
					branch=r[rs]==r[rt];
				}
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
				break;
			case 0x05:/*BNE*/
				if (r[rs]!=r[rt]){
					s->jump_or_branch = 1;
					branch=r[rs]!=r[rt];
				}
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
				break;
			case 0x06:/*BLEZ*/
				if (r[rs]<=0){
					s->jump_or_branch = 1;				
					branch=r[rs]<=0;
				}
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
				break;
			case 0x07:/*BGTZ*/
				if (r[rs]>0){
					s->jump_or_branch = 1;
					branch=r[rs]>0;
				}
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
				break;
			case 0x08:/*ADDI*/
				r[rt]=r[rs]+(short)imm;
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC*2;
				break;
			case 0x09:/*ADDIU*/
				u[rt]=u[rs]+(short)imm;
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC*2;
				break;
			case 0x0a:/*SLTI*/
				r[rt]=r[rs]<(short)imm;
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC*2;
				break;
			case 0x0b:/*SLTIU*/
				u[rt]=u[rs]<(unsigned int)(short)imm;
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_ARITHMETIC*2;
				break;
			case 0x0c:/*ANDI*/
				r[rt]=r[rs]&imm;
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOGIC*2;
				break;
			case 0x0d:/*ORI*/
				r[rt]=r[rs]|imm;
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOGIC*2;
				break;
			case 0x0e:/*XORI*/
				r[rt]=r[rs]^imm;
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOGIC*2;
				break;
			case 0x0f:/*LUI*/
				r[rt]=(imm<<16);
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOGIC*2;
				break;
			case 0x10:/*COP0*/						// ??? not fully implemented
				if((opcode & (1<<23)) == 0){	//move from CP0
					if(rd == 12)
						r[rt]=s->status;
					else
						r[rt]=s->epc;
				}else{				//move to CP0
					s->status=r[rt]&1;
					if(s->processId && (r[rt]&2)){
						s->userMode|=r[rt]&2;
						printf("CpuStatus=%d %d %d\n", r[rt], s->status, s->userMode);
					}
				}
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_MOVE;
				break;
			case 0x14:/*BEQL*/
				if (r[rs]==r[rt]){
					s->jump_or_branch = 1;
					lbranch=r[rs]==r[rt];
				}
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
				break;
			case 0x15:/*BNEL*/
				if (r[rs]!=r[rt]){
					s->jump_or_branch = 1;
					lbranch=r[rs]!=r[rt];
				}
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
				break;
			case 0x16:/*BLEZL*/
				if (r[rs]<=0){
					s->jump_or_branch = 1;
					lbranch=r[rs]<=0;
				}
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
				break;
			case 0x17:/*BGTZL*/
				if (r[rs]>0){
					s->jump_or_branch = 1;
					lbranch=r[rs]>0;
				}
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_BRANCH_JUMP;
				break;
			case 0x20:/*LB*/
				r[rt]=(signed char)mem_read(s,1,ptr,cpu_n);
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOAD_STORE*2;
				*pause_cycles = 1;
				break;
			case 0x21:/*LH*/
				r[rt]=(signed short)mem_read(s,2,ptr,cpu_n);
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOAD_STORE*4;
				*pause_cycles = 1;
				break;
			case 0x22:/*LWL*/ break;
			case 0x23:/*LW*/
				r[rt]=mem_read(s,4,ptr,cpu_n);
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOAD_STORE*4;
				*pause_cycles = 1;
				break;
			case 0x24:/*LBU*/
				r[rt]=(unsigned char)mem_read(s,1,ptr,cpu_n);
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOAD_STORE*2;
				*pause_cycles = 1;
				break;
			case 0x25:/*LHU*/
				r[rt]=(unsigned short)mem_read(s,2,ptr,cpu_n);
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOAD_STORE*2;
				*pause_cycles = 1;
				break;
			case 0x26:/*LWR*/  break;
			case 0x28:/*SB*/
				mem_write(s,1,ptr,r[rt], std_out,cpu_n);
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOAD_STORE*2;
				*pause_cycles = 1;
				break;
			case 0x29:/*SH*/
				mem_write(s,2,ptr,r[rt], std_out,cpu_n);
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOAD_STORE*4;
				*pause_cycles = 1;
				break;
			case 0x2a:/*SWL*/ break;
			case 0x2b:/*SW*/
				mem_write(s,4,ptr,r[rt], std_out,cpu_n);
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOAD_STORE*4;
				*pause_cycles = 1;
				break;
			case 0x2e:/*SWR*/  break;
			case 0x2f:/*CACHE*/
//				break;
			case 0x30:/*LL*/
				r[rt]=mem_read(s,4,ptr,cpu_n);
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOAD_STORE*2;
				*pause_cycles = 1;
				break;
			case 0x38:/*SC*/
				mem_write(s,4,ptr,r[rt], std_out,cpu_n);
				est_energy[cpu_n]+=ENERGY_PER_CYCLE_LOAD_STORE*2;
				r[rt]=1;
				*pause_cycles = 1;
				break;
			default:
				printf("ERROR2 address=0x%x opcode=0x%x\n",s->pc,opcode);
				fflush(stdout);
				s->wakeup=1;
	}
	ins_counter_op[op][cpu_n]++;

	s->pc_next += (branch || lbranch == 1) ? imm_shift : 0;
	s->pc_next &= ~3;
	s->skip = (lbranch == 0);

	if(s->exceptionId){
		r[rt] = rSave;
		s->epc = epc; 
		s->pc_next = 0x10000050;
//		s->pc_next = 0x44;
		s->skip = 1; 
		s->exceptionId = 0;
		s->userMode = 0;
		return;
	}
}

int do_debug(State *s[], FILE *std_out[]){
	int ch;
	unsigned char mem;
	int i, j=0, watch=0, addr, k=0, l, m, n[MAX_N_CORES];
	int pause_cpu[MAX_N_CORES];
	int irq_counter[MAX_N_CORES];
	char report_string[]= "./reports/report\0\0\0\0\0\0\0\0\0\0";
	unsigned long long gcycles = 0;

	Core *core;
	NetworkInterface *ni;
	Buffer *buffer;
	Port *port;

	for(j=0;j<MAX_N_CORES;j++){
		l = -1;
		m = -1;
		n[j] = 0;
		pause_cpu[j] = 0;
		irq_counter[j] = 0;
	}

	for(j=0;j<n_cores;j++){
		s[j]->pc_next = s[j]->pc + 4;
		s[j]->skip = 0;
		s[j]->wakeup = 0;
		cycle(s[j], 0, j, std_out[j], &pause_cpu[j], &irq_counter[j]);
	}

	while(1){
		for(j=0;j<n_cores;j++){
			if (brkpt[j] == 0){
				if ((cpu_cycles[j] & ((long long)HWMemory[4][j] - 1)) == ((long long)HWMemory[4][j] - 1)){
					if (HWMemory[1][j] & (IRQ_COUNTER18 | IRQ_COUNTER18_NOT)){
//					if ((HWMemory[1][j] & (IRQ_COUNTER18 | IRQ_COUNTER18_NOT)) && ((HWMemory[2][j] & IRQ_NOC_READ) == 0) ){
						if(s[j]->status == 1) irq_counter[j] = 1;
					}
					if (HWMemory[2][j] & IRQ_COUNTER18){
						HWMemory[2][j] &= ~IRQ_COUNTER18;
						HWMemory[2][j] |= IRQ_COUNTER18_NOT;
					}else{
						HWMemory[2][j] &= ~IRQ_COUNTER18_NOT;
						HWMemory[2][j] |= IRQ_COUNTER18;
					}
				}

				if ((!(HWMemory[2][j] & IRQ_UART_WRITE_AVAILABLE)) && (uart_delay[j]) > 0){
					uart_delay[j]--;
					io_counter[j]++;
				}else{
					uart_delay[j] = UART_DELAY;
					HWMemory[2][j] |= IRQ_UART_WRITE_AVAILABLE;
				}

				core = getCore(j);
				port = &(core->port);
				ni = getNetworkInterface(j);
				buffer = getBuffer(ni, NOC);
				if(isFull(buffer) && port->in_request == ON && flits_remaining[j] == 0 )//&& irq_counter[j] == 0)
				// to create a noc interrupt the buffer need to be full and requesing to send the first flit,
				// there also can't be any thing on the idle buffer and a clock interrupt can't be generated at the same cycle
				{
					if(HWMemory[1][j] & IRQ_NOC_READ)
					// não mascarada					
					{
						if(s[j]->status == 1)
						// interrupções habilitadas
						{
							flits_remaining[j] = OS_PACKET_SIZE+1;
//							irq_counter[j] = 1;
							irq_counter[j] = 2;
							HWMemory[2][j] |= IRQ_NOC_READ;
						}
					}
				}
			}
		}

		gcycles++;

		for(j=0;j<n_cores;j++)
		{
			if(is_sending[j] == ON)
			{
				core = getCore(j);
				port = &(core->port);
				if(port->out_ack == ON)
				{
					port->out = 0;
					port->out_request = OFF;
					port->out_ack = OFF;
					is_sending[j] = OFF;
				}
			}
		}
		
#ifndef BUS
		for(j=0;j<N_CORES;j++){
			synchronizeRouter(j);
			synchronizeNetworkInterface(j);
			synchronizeCore(j);
		}
		
		for(j=0;j<N_CORES;j++){
			if (gcycles % CPU_NETWORK_CLK_RATIO == 0)
				cycleRouter(j);
			cycleNetworkInterface(j);
		}
#else
		synchronizeRouter(0);
		for(j=0;j<N_CORES;j++){
			synchronizeNetworkInterface(j);
			synchronizeCore(j);
		}
		if (gcycles % CPU_NETWORK_CLK_RATIO == 0)
			cycleRouter(0);
		for(j=0;j<N_CORES;j++){
			cycleNetworkInterface(j);
		}
#endif

		for(j=0;j<n_cores;j++){					
			if (brkpt[j] == 0){			
				if (pause_cpu[j] == 0 && is_sending[j] == OFF)
					cycle(s[j], 0, j, std_out[j], &pause_cpu[j], &irq_counter[j]);
				else if(pause_cpu[j] >= 1)
					pause_cpu[j]--;

				if (cpu_cycles[j] >= max_cycles){
					brkpt[j] = 1;
				}
				cpu_cycles[j]++;
			}else{
				n[j] = 1;
			}		
		}
		for(j=0;j<n_cores;j++)
			if (n[j] == 0) break;
		if (j == n_cores){
			printf("\n");
			for(j=0;j<n_cores;j++){
				show_cpu_stats(strcat(strcat(report_string, itoa(j)),".txt"),j);
				strcpy(report_string, "./reports/report\0\0\0\0\0\0\0\0\0\0\0");
			}
			show_mpsoc_stats("./reports/mpsoc.txt");
			return 0;
		}
	}
}

int main(int argc,char *argv[]){
	State context[MAX_N_CORES];
	State *s[MAX_N_CORES];
	FILE *in[MAX_N_CORES];
	FILE *std_out[MAX_N_CORES];
	int bytes, index;
	clock_t time;
	int i,j;
	char filename_string[] = "./objects/code\0\0\0\0\0\0\0\0\0\0\0";
	char stdout_string[] = "./reports/stdout\0\0\0\0\0\0\0\0\0\0\0";

	printf("\nN-MIPS MPSoC Simulator");
	printf("\nEmbedded Systems Group - GSE, PUCRS [2007 - 2011]\n");
	fflush(stdout);

	for(j=0;j<MAX_N_CORES;j++){
		is_sending[j] = 0;
		is_reading[j] = 0;
		flits_remaining[j] = 0;
		
		s[j] = &context[j];
		memset(s[j], 0, sizeof(State));

		HWMemory[0][j] = 0;
		HWMemory[1][j] = 0;
		HWMemory[2][j] = (IRQ_GPIO31_NOT | IRQ_GPIO30_NOT | IRQ_UART_WRITE_AVAILABLE);
		HWMemory[3][j] = reference_clock;
		HWMemory[4][j] = 0x40000;

		for(i=0;i<MEM_SIZE;i++)
			SRAM[j*MEM_SIZE+i] = 0;
		GPIOAIN[j] = 0;
		GPIO0OUT[j] = 0;
		cpu_cycles[j] = 0;
		for(i=0;i<64;i++)
			ins_counter_op[i][j] = 0;
		for(i=0;i<64;i++)
			ins_counter_rt[i][j] = 0;
		for(i=0;i<32;i++)
			ins_counter_func[i][j] = 0;
		est_energy[j] = 0.0;
		ins_counter[j] = 0;
		for(i=0;i<6;i++)
			ins_class_counter[i][j] = 0;
		io_counter[j] = 0;
		brkpt[j] = 0;
		flits_sent[j] = 0;
		flits_received[j] = 0;
		broadcasts[j] = 0;
	}	

	if(argc <= 1){
		printf("\nUsage: mpsoc_sim [n_cycles] [frequency]");
		printf("\n         or");
		printf("\n       mpsoc_sim [time unit] e.g. 1000 ns 10 us, 50 ms, 1 s");
		printf("\n - Object codes must be in /objects directory and named");
		printf("\n   code0.bin, code1.bin, code2.bin...");
		printf("\n   There must be between 1 and 128 object codes in this directory.");
		printf("\n - Reports will be saved in /reports directory.\n\n");
		fflush(stdout);

		return 0;
	}

	if(argc == 3 && argv[2][0] != '\0'){
		max_cycles = atoll(argv[1]+'\0');
		if (argv[2][0] == 'c'){
			sim_metric = 'c';
		}
		if (argv[2][0] == 's'){
			max_cycles *= reference_clock;
			sim_metric = 's';
		}
		if (argv[2][0] == 'm'){
			max_cycles *= ((double)reference_clock / 1000.0);
			sim_metric = 'm';
		}
		if (argv[2][0] == 'u'){
			max_cycles *= ((double)reference_clock / 1000000.0);
			sim_metric = 'u';
		}
		if (argv[2][0] == 'n'){
			max_cycles *= ((double)reference_clock / 1000000000.0);
			sim_metric = 'n';
		}
	}else{
		printf("\nType mpsoc_emu for help.\n");
		fflush(stdout);
		return (-1);
	}

	for(j=0;j<MAX_N_CORES;j++){
		in[j] = fopen(strcat(strcat(filename_string, itoa(j)),".bin"), "rb");
		strcpy(filename_string, "./objects/code\0\0\0\0\0\0\0\0\0\0\0");
		if (in[j] == NULL){
			if (j == 0){
				printf("\nCould not find at least one object file in ./objects/");
				printf("\nFiles must be named code0.bin, code1.bin, code2.bin... in sequence\n");
				fflush(stdout);

				return(-1);
			}else{
				n_cores = j;
				break;
			}
		}
	}

	for(j=0;j<n_cores;j++){
		std_out[j] = fopen(strcat(strcat(stdout_string, itoa(j)),".txt"), "wb");
		strcpy(stdout_string, "./reports/stdout\0\0\0\0\0\0\0\0\0\0\0");
		if (std_out[j] == NULL){
			printf("\nCould not open ./reports/stdout%d.txt for writing.\n", j);
			fflush(stdout);

			return (-1);
		}
	}

	for(j=0;j<n_cores;j++){
		bytes = fread(&SRAM[j*MEM_SIZE], 1, MEM_SIZE, in[j]);
		fclose(in[j]);
	}

	for(j=n_cores;j<MAX_N_CORES;j++)
		brkpt[j] = 1;
	
	for(j=0;j<n_cores;j++){
		s[j]->pc = 0x0;
		s[j]->irqStatus = 0;
		s[j]->big_endian = 1;
		s[j]->jump_or_branch = 0;
		s[j]->no_execute_branch_delay_slot = 0;
		s[j]->mem = &SRAM[j*MEM_SIZE];
		index = mem_read(s[j], 4, 0, j);
		if(index == 0x3c1c1000)
			s[j]->pc = RAM_EXTERNAL_BASE;
	}
	
	for(j=0;j<n_cores;j++){
		log_out[j] = fopen(strcat(strcat(logout_string, itoa(j)),".txt"), "wb");
		out_out[j] = fopen(strcat(strcat(outout_string, itoa(j)),".txt"), "wb");
		strcpy(logout_string, "./reports/logout\0\0\0\0\0\0\0\0\0\0\0");
		strcpy(outout_string, "./reports/out\0\0\0\0\0\0\0\0\0\0\0");
		if (log_out[j] == NULL){
			printf("\nCould not open ./reports/logout%d.txt for writing.\n", j);
			fflush(stdout);

			return (-1);
		}
		if (out_out[j] == NULL){
 			printf("\nCould not open ./reports/out%d.txt for writing.\n", j);
 			fflush(stdout);
 
 			return (-1);
 		}
	}

	load_architecture();	
	
	time = clock();
	
	do_debug(s, std_out);

	time = clock() - time;
	printf("\nSimulation time: %ld.%.3lds\n", time/CLOCKS_PER_SEC,(time%CLOCKS_PER_SEC)*1000/CLOCKS_PER_SEC);

	for(j=0;j<n_cores;j++){
		fclose(std_out[j]);
		fclose(log_out[j]);
		fclose(out_out[j]);
	}

	unload_architecture();

	return(0);
}

