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

/**
 * @brief RPC callback
 * 
 * @param buf_ptr is a pointer to packet data.
 * 
 * @return ERR_OK.
 * 
 * This is called when RPC packets arrive. This routine just places the packet (pointer to
 * a buffer taken from the NoC message queue pool) on the RPC thread message queue. On error
 * (queue full), the pointer is put back to the NoC pool. TODO: treat RPC service as a critical
 * event? The RR scheduler is behaving ok, but this is not enough!
 */
static int32_t rpc_callback(uint16_t *buf_ptr)
{
	int32_t rpc_driver;
	
	if (hf_queue_addtail(pktdrv_tqueue[noc_rpcdrv.thread_id], buf_ptr)){
		kprintf("\nKERNEL: NoC RPC service queue full!");
		hf_queue_addtail(pktdrv_queue, buf_ptr);
	} else {
/*		krnl_tcb[noc_rpcdrv.thread_id].critical = 1; */
	}
	
	return ERR_OK;
}

/**
 * @brief Handles RPC calls from remote processors / threads.
 *
 * The service runs as a best effort task and waits for messages on port 0xffff (special case
 * on the NoC driver. Data is received (composed of a header containing program and procedure
 * identification and procedure parameters / size), and the remote call is handled:
 * 
 * 1) look for the prognum / procnum pair in a list (for a registered procedure);
 * 2) compare input and output parameter sizes, which should match;
 * 3) take packet data and fill input parameters;
 * 4) call the procedure, passing input and output parameters by reference;
 * 5) copy output parameters to packet data, and send it back or send an error code on fail.
 *
 * Data is sent structured as:
 * - 4 bytes (prognum)
 * - 4 bytes (procnum)
 * - 2 bytes (in_size)
 * - 2 bytes (out_size)
 * - 4 bytes (ecode)
 * - (output parameters)
 */
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

/**
 * @brief Initializes the NoC RPC driver
 * 
 * @return ERR_OK on success and ERR_ERROR on fail.
 *
 * Data structures related to the RPC driver are initialized, the RPC service thread is spawned
 * and the RPC callback is registered for incoming RPC packets.
 */
static int32_t noc_rpcdrv_init(void)
{
	noc_rpcdrv.proc_list = hf_list_init();
	noc_rpcdrv.thread_id = hf_spawn(noc_rpcdrv_service, 0, 0, 0, "NoC RPC", RPC_MAX_PARAM_SIZE + RPC_STACK_SIZE);

	if (noc_rpcdrv.thread_id > 0) {
		pktdrv_callback = rpc_callback;		
		kprintf("\nKERNEL: NoC RPC driver registered");
	}else{
		kprintf("\nKERNEL: NoC RPC init failed");
		
		return ERR_ERROR;
	}
	
	return ERR_OK;
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
			return ERR_ERROR;
	}
	
	k = hf_list_count(noc_rpcdrv.proc_list);
	for (i = 0; i < k; i++) {
		proc_param = hf_list_get(noc_rpcdrv.proc_list, i);
		if (proc_param) {
			if (proc_param->prognum == prognum && proc_param->procnum == procnum)
				return ERR_ERROR;
		}
	}
	
	proc_param = (struct proc_param_s *)hf_malloc(sizeof(struct proc_param_s));
	input = (int8_t *)hf_malloc(in_size);
	output = (int8_t *)hf_malloc(out_size);
	if (!proc_param || !input || !output) return ERR_OUT_OF_MEMORY;
	
	proc_param->prognum = prognum;
	proc_param->procnum = procnum;
	proc_param->proc_ptr = pname;
	proc_param->in_size = in_size;
	proc_param->out_size = out_size;
	proc_param->in_data = input;
	proc_param->out_data = output;
	hf_list_append(noc_rpcdrv.proc_list, proc_param);
	
	kprintf("\nKERNEL: RPC registered prognum %d procnum %d at %x (in size %d, out size %d)", prognum, procnum, (uint32_t)pname, in_size, out_size);
	
	return ERR_OK;
}

// send data structured as:
// 4 bytes (prognum)
// 4 bytes (procnum)
// 2 bytes (in_size)
// 2 bytes (out_size)
// 4 bytes (ecode)
// (input parameters)
int32_t hf_call(uint16_t cpu, uint32_t prognum, uint32_t procnum, int8_t *in, uint16_t in_size, int8_t *out, uint16_t out_size)
{
	union proc_pkt_u proc_pkt;
	uint16_t rcpu, rport, rsize;
	static volatile int8_t init = 0;
	
	if (in_size > RPC_MAX_PARAM_SIZE || out_size > RPC_MAX_PARAM_SIZE)
		return ERR_ERROR;
		
	if (!init) {
		hf_mtxinit(&rpc_lock);
		init = 1;
	}
	
	hf_mtxlock(&rpc_lock);
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
	hf_mtxunlock(&rpc_lock);
	
	hf_recv(&rcpu, &rport, proc_pkt.proc_data, &rsize, RPC_SCHANNEL - hf_cpuid());
	
	if (proc_pkt.proc_hdr.ecode != 0)
		return proc_pkt.proc_hdr.ecode;
	
	if (out_size != proc_pkt.proc_hdr.out_size)
		return ERR_ERROR;
		
	memcpy(out, proc_pkt.proc_data + sizeof(struct proc_pkt_s), out_size);
		
	return ERR_OK;
}
