/**
 * @file noc_rpc.h
 * @author Sergio Johann Filho
 * @date October 2018
 *
 * @section LICENSE
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file 'doc/license/gpl-2.0.txt' for more details.
 *
 * @section DESCRIPTION
 *
 * A basic RPC mechanism on top of the Network-on-Chip primitives.
 */

#ifndef _NOC_RPC
#define _NOC_RPC

#define RPC_MAX_PARAM_SIZE	1024
#define RPC_MAX_PARALLEL_CALLS	20
#define RPC_STACK_SIZE		2048
#define RPC_SCHANNEL		65534
#define RPC_PORT		65535

struct noc_rpc_s {
	uint16_t thread_id;
	struct list *proc_list;
};

struct noc_rpc_s noc_rpcdrv;

struct proc_param_s {
	uint32_t prognum;
	uint32_t procnum;
	int32_t (*proc_ptr)(int8_t *, int8_t *);
	uint16_t in_size;
	uint16_t out_size;
	int8_t *in_data;
	int8_t *out_data;
};

struct proc_pkt_s {
	uint32_t prognum;
	uint32_t procnum;
	uint16_t in_size;
	uint16_t out_size;
	int32_t ecode;
};

union proc_pkt_u {
	struct proc_pkt_s proc_hdr;
	int8_t proc_data[sizeof(struct proc_pkt_s) + RPC_MAX_PARAM_SIZE];
};

mutex_t rpc_lock;

int32_t hf_register(uint32_t prognum, uint32_t procnum, int32_t (*pname)(int8_t *, int8_t *), uint16_t in_size, uint16_t out_size);
int32_t hf_call(uint16_t cpu, uint32_t prognum, uint32_t procnum, int8_t *in, uint16_t in_size, int8_t *out, uint16_t out_size);

#endif
