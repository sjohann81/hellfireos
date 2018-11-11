/**
 * @file noc_rpcdrv.c
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
 * A basic RPC mechanism on top of the Network-on-Chip primitives. This driver implements
 * the RPC semantics for remote calls in a NoC environment. Callbacks can be registered and
 * the driver waits for remote calls. Remote calls are placed on a queue, and handled in
 * first-come-first-served (FIFO) order by the noc_rpcdrv_service thread.
 */

#include <hellfire.h>
#include <noc.h>
#include <noc_rpc.h>

static int32_t rpc_callback(uint16_t *buf_ptr)
{
	int32_t rpc_driver;
	
	if (hf_queue_addtail(pktdrv_tqueue[noc_rpcdrv.thread_id], buf_ptr)){
		kprintf("\nKERNEL: NoC RPC service queue full!");
		hf_queue_addtail(pktdrv_queue, buf_ptr);
	} else {
/*		rpc_driver = hf_id("NoC RPC");
		if (rpc_driver >= 0 && rpc_driver < MAX_TASKS)
			krnl_tcb[rpc_driver].critical = 1;
*/
	}
	
	return 0;
}

// -look for prognum/procnum
// -if found, compare in/out size, which should match to packet data
// -take packet data and fill the list entry input parameters
// -call (passing the address of both in/out parameters)
// -copy out parameters to packet data, and send it back or send an error code on fail
// send data structured as:
// 4 bytes (prognum)
// 4 bytes (procnum)
// 2 bytes (in_size)
// 2 bytes (out_size)
// 4 bytes (ecode)
// (output parameters)
static void noc_rpcdrv_service(void)
{
	union proc_pkt_u proc_pkt;
	struct proc_param_s *proc_param;
	uint16_t cpu, port, size;
	int32_t channel, i, k;
	
	hf_comm_create(hf_selfid(), 0xffff, 0);
	
	for (;;) {
		channel = hf_recvprobe();
		if (channel >= 0) {
			hf_recv(&cpu, &port, proc_pkt.proc_data, &size, channel);

			if (size > RPC_MAX_PARAM_SIZE) {
				kprintf("\nKERNEL: RPC data > %d bytes, this is not right!", RPC_MAX_PARAM_SIZE);
				continue;
			}
			
			k = hf_list_count(noc_rpcdrv.proc_list);
			for (i = 0; i < k; i++) {
				proc_param = hf_list_get(noc_rpcdrv.proc_list, i);
				if (proc_param) {
					if (proc_param->prognum == proc_pkt.proc_hdr.prognum &&
					proc_param->procnum == proc_pkt.proc_hdr.procnum)
						break;
				}
			}
			
			if (i < k) {
				if (proc_param->in_size == proc_pkt.proc_hdr.in_size &&
				proc_param->out_size == proc_pkt.proc_hdr.out_size) {
					memcpy(proc_param->in_data, proc_pkt.proc_data + sizeof(struct proc_pkt_s), proc_param->in_size);
					proc_param->proc_ptr(proc_param->in_data, proc_param->out_data);
					memcpy(proc_pkt.proc_data + sizeof(struct proc_pkt_s), proc_param->out_data, proc_param->out_size);
				} else {
					kprintf("\nKERNEL: RPC parameters size mismatch!");
					proc_pkt.proc_hdr.out_size = 0;
					proc_pkt.proc_hdr.ecode = -1;
				}
			} else {
				kprintf("\nKERNEL: RPC prognum/procnum not found!");
				proc_pkt.proc_hdr.out_size = 0;
				proc_pkt.proc_hdr.ecode = -1;
			}
			
			hf_send(cpu, port, proc_pkt.proc_data, sizeof(struct proc_pkt_s) + proc_pkt.proc_hdr.out_size, channel);
		}
	}
}

// init data structures
// init rpc_service queue
// set noc_rpcdrv_thread_id
// configure noc driver (callback handler) to put callback packets on service queue
static int32_t noc_rpcdrv_init(void)
{
	noc_rpcdrv.proc_list = hf_list_init();
	noc_rpcdrv.thread_id = hf_spawn(noc_rpcdrv_service, 0, 0, 0, "NoC RPC", RPC_MAX_PARAM_SIZE + RPC_STACK_SIZE);

	if (noc_rpcdrv.thread_id > 0) {
		pktdrv_callback = rpc_callback;		
		kprintf("\nKERNEL: NoC RPC driver registered");
	}else{
		kprintf("\nKERNEL: NoC RPC init failed");
		
		return -1;
	}
	
	return 0;
}

// -check if the RPC subsystem is initialized (hook registered to the NoC packet driver callback). if not, register it.
// -compare the list of registered procedures to the prognum/procnum pair and abort if already used.
// -add a list entry with prognum procnum, proc pointer and allocate in and out data structures (register it on the list)
int32_t hf_register(uint32_t prognum, uint32_t procnum, int32_t (*pname)(int8_t *, int8_t *), uint16_t in_size, uint16_t out_size)
{
	int32_t i, k;
	struct proc_param_s *proc_param;
	int8_t *input, *output;
	
	if (in_size > RPC_MAX_PARAM_SIZE || out_size > RPC_MAX_PARAM_SIZE)
		return -1;
	
	if (noc_rpcdrv.thread_id == 0) {
		if (noc_rpcdrv_init())
			return -1;
	}
	
	k = hf_list_count(noc_rpcdrv.proc_list);
	for (i = 0; i < k; i++) {
		proc_param = hf_list_get(noc_rpcdrv.proc_list, i);
		if (proc_param) {
			if (proc_param->prognum == prognum && proc_param->procnum == procnum)
				return -1;
		}
	}
	
	proc_param = (struct proc_param_s *)hf_malloc(sizeof(struct proc_param_s));
	input = (int8_t *)hf_malloc(in_size);
	output = (int8_t *)hf_malloc(out_size);
	if (!proc_param || !input || !output) return -1;
	
	proc_param->prognum = prognum;
	proc_param->procnum = procnum;
	proc_param->proc_ptr = pname;
	proc_param->in_size = in_size;
	proc_param->out_size = out_size;
	proc_param->in_data = input;
	proc_param->out_data = output;
	hf_list_append(noc_rpcdrv.proc_list, proc_param);
	
	kprintf("\nKERNEL: RPC registered prognum %d procnum %d at %x (in size %d, out size %d)", prognum, procnum, (uint32_t)pname, in_size, out_size);
	
	return 0;
}

// send data structured as:
// 4 bytes (prognum)
// 4 bytes (procnum)
// 2 bytes (in_size)
// 2 bytes (out_size)
// 4 bytes (ecode)
// (input parameters)
// 
// it is possible for multiple threads in the same CPU to make RPC calls in parallel to different CPUs, but
// threads are blocked if they remotelly call the same CPU. This mechanism (using cpu, port and channel) avoid
// conflicts of sustained multiple transactions involving multi-packet calls. remote CPUs are kept temporarily in
// a queue. each CPU talks to the RPC remote driver in a different channel, so conflicts are avoided.
int32_t hf_call(uint16_t cpu, uint32_t prognum, uint32_t procnum, int8_t *in, uint16_t in_size, int8_t *out, uint16_t out_size)
{
	union proc_pkt_u proc_pkt;
	uint16_t rcpu, rport, rsize;
	
	if (in_size > RPC_MAX_PARAM_SIZE || out_size > RPC_MAX_PARAM_SIZE)
		return -1;
		
//	if (rpc_cpu_queue == NULL) {
//		rpc_cpu_queue = hf_queue_create(RPC_MAX_PARALLEL_CALLS);
//		if (rpc_cpu_queue == NULL) panic(PANIC_OOM);
//	}
	
	/* TODO: improve this so multiple threads may be making parallel RPC calls */
	
		
	proc_pkt.proc_hdr.prognum = prognum;
	proc_pkt.proc_hdr.procnum = procnum;
	proc_pkt.proc_hdr.in_size = in_size;
	proc_pkt.proc_hdr.out_size = out_size;
	proc_pkt.proc_hdr.ecode = 0;
	memcpy(proc_pkt.proc_data + sizeof(struct proc_pkt_s), in, in_size);
	
	/* TODO: use a better / more resilient protocol!
	 * this is a blocking primitive, and will hang if no response is received.
	 */
	hf_send(cpu, RPC_PORT, proc_pkt.proc_data, sizeof(struct proc_pkt_s) + in_size, RPC_SCHANNEL - hf_cpuid());
	hf_recv(&rcpu, &rport, proc_pkt.proc_data, &rsize, RPC_SCHANNEL - hf_cpuid());
	
	if (proc_pkt.proc_hdr.ecode != 0)
		return proc_pkt.proc_hdr.ecode;
	
	if (out_size != proc_pkt.proc_hdr.out_size)
		return -1;
		
	memcpy(out, proc_pkt.proc_data + sizeof(struct proc_pkt_s), out_size);
		
	return 0;
}
