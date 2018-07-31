/* file:          hf_riscv64_sim.c
 * description:   HF-RISCV64 simulator
 * date:          09/2016
 * author:        Sergio Johann Filho <sergio.filho@pucrs.br>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MEM_SIZE			0x00100000
#define SRAM_BASE			0x40000000
#define EXIT_TRAP			0xe0000000
#define IRQ_VECTOR			0xf0000000
#define IRQ_CAUSE			0xf0000010
#define IRQ_MASK			0xf0000020
#define IRQ_STATUS			0xf0000030
#define IRQ_EPC				0xf0000040
#define COUNTER				0xf0000050
#define COMPARE				0xf0000060
#define COMPARE2			0xf0000070
#define EXTIO_IN			0xf0000080
#define EXTIO_OUT			0xf0000090
#define DEBUG_ADDR			0xf00000d0
#define UART_WRITE			0xf00000e0
#define UART_READ			0xf00000e0
#define UART_DIVISOR			0xf00000f0

#define ntohs(A) ( ((A)>>8) | (((A)&0xff)<<8) )
#define htons(A) ntohs(A)
#define ntohl(A) ( ((A)>>24) | (((A)&0xff0000)>>8) | (((A)&0xff00)<<8) | ((A)<<24) )
#define htonl(A) ntohl(A)

typedef struct {
	int64_t r[32];
	uint64_t pc, pc_next;
	int8_t *mem;
	int64_t vector, cause, mask, status, status_dly[4], epc, counter, compare, compare2;
} state;

int8_t sram[MEM_SIZE];

FILE *fptr;
int32_t log_enabled = 0;

void dumpregs(state *s){
	int32_t i;

	for (i = 0; i < 32; i+=2){
		printf("\nr%02d [%016x] r%02d [%016x]", \
		i, s->r[i], i+1, s->r[i+1]);
	}
	printf("\n");
}

void bp(state *s, uint64_t ir){
	printf("\npc: %016x, ir: %016x", s->pc, ir);
	dumpregs(s);
	getchar();
}

static int32_t mem_fetch(state *s, uint32_t address){
	uint32_t value=0;
	uint32_t *ptr;

	ptr = (uint32_t *)(s->mem + (address % MEM_SIZE));
	value = *ptr;

	return(value);
}

static int64_t mem_read(state *s, int32_t size, uint64_t address){
	uint64_t value=0;
	uint64_t *ptr;

	switch(address){
		case IRQ_VECTOR:	return s->vector;
		case IRQ_CAUSE:		return s->cause | 0x0080 | 0x0040;
		case IRQ_MASK:		return s->mask;
		case IRQ_STATUS:	return s->status;
		case IRQ_EPC:		return s->epc;
		case COUNTER:		return s->counter;
		case COMPARE:		return s->compare;
		case COMPARE2:		return s->compare2;
		case UART_READ:		return getchar();
		case UART_DIVISOR:	return 0;
	}

	ptr = (uint64_t *)(s->mem + (address % MEM_SIZE));

	switch(size){
		case 8:
			if(address & 3){
				printf("\nunaligned access (load word) pc=0x%x addr=0x%x", s->pc, address);
				dumpregs(s);
				exit(1);
			}else{
				value = *(int64_t *)ptr;
			}
			break;
		case 4:
			if(address & 3){
				printf("\nunaligned access (load word) pc=0x%x addr=0x%x", s->pc, address);
				dumpregs(s);
				exit(1);
			}else{
				value = *(int32_t *)ptr;
			}
			break;
		case 2:
			if(address & 1){
				printf("\nunaligned access (load halfword) pc=0x%x addr=0x%x", s->pc, address);
				dumpregs(s);
				exit(1);
			}else{
				value = *(int16_t *)ptr;
			}
			break;
		case 1:
			value = *(int8_t *)ptr;
			break;
		default:
			printf("\nerror");
	}
//	printf("\nload(%d): %x, addr: %08x", size, value, address);

	return(value);
}

static void mem_write(state *s, int32_t size, uint64_t address, uint64_t value){
	uint64_t *ptr;
	uint64_t i;

	switch(address){
		case IRQ_VECTOR:	s->vector = value; return;
		case IRQ_CAUSE:		s->cause = value; return;
		case IRQ_MASK:		s->mask = value; return;
		case IRQ_STATUS:	if (value == 0){ s->status = 0; for (i = 0; i < 4; i++) s->status_dly[i] = 0; }else{ s->status_dly[3] = value; } return;
		case IRQ_EPC:		s->epc = value; return;
		case COUNTER:		s->counter = value; return;
		case COMPARE:		s->compare = value; s->cause &= 0xffef; return;
		case COMPARE2:		s->compare2 = value; s->cause &= 0xffdf; return;
		case EXIT_TRAP:
			fflush(stdout);
			if (log_enabled)
				fclose(fptr);
			printf("\nend of simulation - %d cycles.\n", s->counter);
			exit(0);
		case DEBUG_ADDR:
			if (log_enabled)
				fprintf(fptr, "%c", (int8_t)(value & 0xff));
			return;
		case UART_WRITE:
			fprintf(stderr, "%c", (int8_t)(value & 0xff));
			return;
		case UART_DIVISOR:
			return;
	}

	ptr = (uint64_t *)(s->mem + (address % MEM_SIZE));

	switch(size){
		case 8:
//			printf("\nstore(%d): %x, addr: %08x", size, value, address);
			if(address & 3){
				printf("\nunaligned access (store word) pc=0x%x addr=0x%x", s->pc, address);
				dumpregs(s);
				exit(1);
			}else{
//				value = htonl(value);
				*(int64_t *)(intptr_t)ptr = value;
			}
			break;
		case 4:
//			printf("\nstore(%d): %x, addr: %08x", size, value, address);
			if(address & 3){
				printf("\nunaligned access (store word) pc=0x%x addr=0x%x", s->pc, address);
				dumpregs(s);
				exit(1);
			}else{
//				value = htonl(value);
				*(int32_t *)(intptr_t)ptr = value;
			}
			break;
		case 2:
//			printf("\nstore(%d): %x, addr: %08x", size, (uint16_t)value, address);
			if(address & 1){
				printf("\nunaligned access (store halfword) pc=0x%x addr=0x%x", s->pc, address);
				dumpregs(s);
				exit(1);
			}else{
//				value = htons((uint16_t)value);
				*(int16_t *)(intptr_t)ptr = (uint16_t)value;
			}
			break;
		case 1:
//			printf("\nstore(%d): %x, addr: %08x", size, (uint8_t)value, address);
			*(int8_t *)(intptr_t)ptr = (uint8_t)value;
			break;
		default:
			printf("\nerror");
	}
}

void cycle(state *s){
	uint32_t inst, i;
	uint32_t opcode, rd, rs1, rs2, funct3, funct7;
	int64_t imm_i, imm_s, imm_sb, imm_u, imm_uj;
	int64_t *r = s->r;
	uint64_t *u = (uint64_t *)s->r;
	uint32_t ptr_l, ptr_s;

	if (s->status && (s->cause & s->mask)){
		s->epc = s->pc_next;
		s->pc = s->vector;
		s->pc_next = s->vector + 4;
		s->status = 0;
		for (i = 0; i < 4; i++)
			s->status_dly[i] = 0;
	}

	inst = mem_fetch(s, s->pc);

	opcode = inst & 0x7f;
	rd = (inst >> 7) & 0x1f;
	rs1 = (inst >> 15) & 0x1f;
	rs2 = (inst >> 20) & 0x1f;
	funct3 = (inst >> 12) & 0x7;
	funct7 = (inst >> 25) & 0x7f;
	imm_i = (inst & 0xfff00000) >> 20;
	imm_s = ((inst & 0xf80) >> 7) | ((inst & 0xfe000000) >> 20);
	imm_sb = ((inst & 0xf00) >> 7) | ((inst & 0x7e000000) >> 20) | ((inst & 0x80) << 4) | ((inst & 0x80000000) >> 19);
	imm_u = inst & 0xfffff000;
	imm_uj = ((inst & 0x7fe00000) >> 20) | ((inst & 0x100000) >> 9) | (inst & 0xff000) | ((inst & 0x80000000) >> 11);
	if (inst & 0x80000000){
		imm_i |= 0xfffffffffffff000;
		imm_s |= 0xfffffffffffff000;
		imm_sb |= 0xffffffffffffe000;
		imm_uj |= 0xffffffffffe00000;
	}
	ptr_l = r[rs1] + imm_i;
	ptr_s = r[rs1] + imm_s;
	r[0] = 0;

	switch(opcode){
		case 0x37: r[rd] = (int64_t)(int32_t)imm_u; break;										/* LUI */
		case 0x17: r[rd] = s->pc + (int64_t)(int32_t)imm_u; break;									/* AUIPC */
		case 0x6f: r[rd] = s->pc_next; s->pc_next = s->pc + imm_uj; break;						/* JAL */
		case 0x67: r[rd] = s->pc_next; s->pc_next = (r[rs1] + imm_i); break;				/* JALR */
		case 0x63:
			switch(funct3){
				case 0x0: if (r[rs1] == r[rs2]){ s->pc_next = s->pc + imm_sb; } break;				/* BEQ */
				case 0x1: if (r[rs1] != r[rs2]){ s->pc_next = s->pc + imm_sb; } break;				/* BNE */
				case 0x4: if (r[rs1] < r[rs2]){ s->pc_next = s->pc + imm_sb; } break;				/* BLT */
				case 0x5: if (r[rs1] >= r[rs2]){ s->pc_next = s->pc + imm_sb; } break;				/* BGE */
				case 0x6: if (u[rs1] < u[rs2]){ s->pc_next = s->pc + imm_sb; } break;				/* BLTU */
				case 0x7: if (u[rs1] >= u[rs2]){ s->pc_next = s->pc + imm_sb; } break;				/* BGEU */
				default: goto fail;
			}
			break;
		case 0x3:
			switch(funct3){
				case 0x0: r[rd] = (int8_t)mem_read(s,1,ptr_l); break;						/* LB */
				case 0x1: r[rd] = (int16_t)mem_read(s,2,ptr_l); break;						/* LH */
				case 0x2: r[rd] = mem_read(s,4,ptr_l); break;							/* LW */
				case 0x3: r[rd] = mem_read(s,8,ptr_l); break;							/* LD */
				case 0x4: r[rd] = (uint8_t)mem_read(s,1,ptr_l); break;						/* LBU */
				case 0x5: r[rd] = (uint16_t)mem_read(s,2,ptr_l); break;						/* LHU */
				case 0x6: r[rd] = (uint32_t)mem_read(s, 4, ptr_l); break;					/* LWU */
				default: goto fail;
			}
			break;
		case 0x23:
			switch(funct3){
				case 0x0: mem_write(s,1,ptr_s,r[rs2]); break;							/* SB */
				case 0x1: mem_write(s,2,ptr_s,r[rs2]); break;							/* SH */
				case 0x2: mem_write(s,4,ptr_s,r[rs2]); break;							/* SW */
				case 0x3: mem_write(s,8,ptr_s,r[rs2]); break;							/* SD */
				default: goto fail;
			}
			break;
		case 0x13:
			switch(funct3){
				case 0x0: r[rd] = r[rs1] + (int64_t)imm_i; break;						/* ADDI */
				case 0x1: r[rd] = u[rs1] << (rs2 & 0x3f); break;						/* SLLI */
				case 0x2: r[rd] = r[rs1] < (int64_t)imm_i; break;		 				/* SLTI */
				case 0x3: r[rd] = u[rs1] < (uint64_t)imm_i; break;						/* SLTIU */
				case 0x4: r[rd] = r[rs1] ^ (int64_t)imm_i; break;						/* XORI */
				case 0x6: r[rd] = r[rs1] | (int64_t)imm_i; break;						/* ORI */
				case 0x7: r[rd] = r[rs1] & (int64_t)imm_i; break;						/* ANDI */
				case 0x5:
//					switch((funct7 & 0xfe) << 1){				// ?????
					switch(funct7){
						case 0x0: r[rd] = u[rs1] >> (rs2 & 0x3f); break;				/* SRLI */
						case 0x20: r[rd] = r[rs1] >> (rs2 & 0x3f); break;				/* SRAI */
						default: goto fail;
					}
					break;
				default: goto fail;
			}
			break;
		case 0x1b:
			switch(funct3){
				case 0x0: r[rd] = (int64_t)(int32_t)((r[rs1] + imm_i) & 0xffffffff); break;						/* ADDIW */
				case 0x1: r[rd] = (int64_t)(int32_t)(u[rs1] << (rs2 & 0x3f)); break;					/* SLLIW */
				case 0x5:
					switch(funct7){
						case 0x0: r[rd] = (int64_t)(int32_t)((u[rs1] & 0xffffffff) >> (rs2 & 0x3f)); break;			/* SRLIW */
						case 0x20: r[rd] = (int64_t)((int32_t)(r[rs1] & 0xffffffff) >> (rs2 & 0x3f)); break;			/* SRAIW */
						default: goto fail;
					}
					break;
				default: goto fail;
			}
			break;
		case 0x33:
			switch(funct3){
				case 0x0:
					switch(funct7){
						case 0x0: r[rd] = r[rs1] + r[rs2]; break;					/* ADD */
						case 0x20: r[rd] = r[rs1] - r[rs2]; break;					/* SUB */
						default: goto fail;
					}
					break;
				case 0x1: r[rd] = r[rs1] << r[rs2]; break;							/* SLL */
				case 0x2: r[rd] = r[rs1] < r[rs2]; break;		 					/* SLT */
				case 0x3: r[rd] = u[rs1] < u[rs2]; break;		 					/* SLTU */
				case 0x4: r[rd] = r[rs1] ^ r[rs2]; break;							/* XOR */
				case 0x5:
					switch(funct7){
						case 0x0: r[rd] = u[rs1] >> u[rs2]; break;					/* SRL */
						case 0x20: r[rd] = r[rs1] >> r[rs2]; break;					/* SRA */
						default: goto fail;
					}
					break;
				case 0x6: r[rd] = r[rs1] | r[rs2]; break;							/* OR */
				case 0x7: r[rd] = r[rs1] & r[rs2]; break;							/* AND */
				default: goto fail;
			}
			break;
		case 0x3b:
			switch(funct3){
				case 0x0:
					switch(funct7){
						case 0x0: r[rd] = (int32_t)r[rs1] + (int32_t)r[rs2]; break;			/* ADDW */
						case 0x20: r[rd] = (int32_t)r[rs1] - (int32_t)r[rs2]; break;			/* SUBW */
						default: goto fail;
					}
					break;
				case 0x1: r[rd] = (int32_t)r[rs1] << (r[rs2] & 0x3f); break;					/* SLLW */
				case 0x5:
					switch(funct7){
						case 0x0: r[rd] = (uint32_t)r[rs1] >> (uint32_t)r[rs2]; break;		/* SRLW */
						case 0x20: r[rd] = (int32_t)r[rs1] >> (int32_t)r[rs2]; break;			/* SRAW */
						default: goto fail;
					}
					break;
				default: goto fail;
			}
			break;
		case 0x73:
			switch(funct3){
				case 0:
					switch(imm_i){
//						case 0:	break;									/* SCALL */
//						case 1:	bp(s, inst); break;							/* SBREAK */
//						default: goto fail;
					}
					break;
				case 2:
					switch(imm_i){
						case 0xc00: break;								/* RDCYCLE */
						case 0xc80: break;								/* RDCYCLEH */
						case 0xc01: break;								/* RDTIME */
						case 0xc81: break;								/* RDTIMEH */
						case 0xc02: break;								/* RDINSTRET */
						case 0xc82: break;								/* RDINSTRETH */
						default: goto fail;
					};
					break;
				default: goto fail;
			}
			break;
		default: goto fail;
	}

	s->pc = s->pc_next;
	s->pc_next = s->pc_next + 4;
	s->status = s->status_dly[0];
	for (i = 0; i < 3; i++)
		s->status_dly[i] = s->status_dly[i+1];

	s->counter++;
	if ((s->compare2 & 0xffffff) == (s->counter & 0xffffff)) s->cause |= 0x20;		/*IRQ_COMPARE2*/
	if (s->compare == s->counter) s->cause |= 0x10;						/*IRQ_COMPARE*/
	if (!(s->counter & 0x10000)) s->cause |= 0x8; else s->cause &= 0xfff7;			/*IRQ_COUNTER2_NOT*/
	if (s->counter & 0x10000) s->cause |= 0x4; else s->cause &= 0xfffb;			/*IRQ_COUNTER2*/
	if (!(s->counter & 0x40000)) s->cause |= 0x2; else s->cause &= 0xfffd;			/*IRQ_COUNTER_NOT*/
	if (s->counter & 0x40000) s->cause |= 0x1; else s->cause &= 0xfffe;			/*IRQ_COUNTER*/

	return;
fail:
	printf("\ninvalid opcode (pc=0x%x opcode=0x%x)", s->pc, inst);
	dumpregs(s);
	exit(0);
}

int main(int argc, char *argv[]){
	state context;
	state *s;
	FILE *in;
	int bytes, i;

	s = &context;
	memset(s, 0, sizeof(state));
	memset(sram, 0xff, sizeof(MEM_SIZE));

	if (argc >= 2){
		in = fopen(argv[1], "rb");
		if (in == 0){
			printf("\nerror opening binary file.\n");
			return 1;
		}
		bytes = fread(&sram, 1, MEM_SIZE, in);
		fclose(in);
		if (bytes == 0){
			printf("\nerror reading binary file.\n");
			return 1;
		}
		if (argc == 3){
			fptr = fopen(argv[2], "wb");
			if (!fptr){
				printf("\nerror reading binary file.\n");
				return 1;
			}
			log_enabled = 1;
		}
	}else{
		printf("\nsyntax: hf_risc_sim [file.bin] [logfile.txt]\n");
		return 1;
	}

	s->pc = SRAM_BASE;
	s->pc_next = s->pc + 4;
	s->mem = &sram[0];
	s->vector = 0;
	s->cause = 0;
	s->mask = 0;
	s->status = 0;
	for (i = 0; i < 4; i++)
		s->status_dly[i] = 0;
	s->epc = 0;
	s->counter = 0;
	s->compare = 0;
	s->compare2 = 0;

	for(;;){
		cycle(s);
	}

	return(0);
}

