#include <hellfire.h>
#include <interrupt.h>
#include <noc.h>
#include <ni.h>

void ni_init(void)
{
	int32_t i;
	void *ptr;

	kprintf("\nKERNEL: NoC queue init, %d packets", NOC_PACKET_SLOTS);
	
	pktdrv_queue = hf_queue_create(NOC_PACKET_SLOTS);
	if (pktdrv_queue == NULL) panic(PANIC_OOM);
	
	for (i = 0; i < NOC_PACKET_SLOTS; i++){
		ptr = malloc(sizeof(int16_t) * NOC_PACKET_SIZE);
		if (ptr == NULL) panic(PANIC_OOM);
		hf_queue_addtail(pktdrv_queue, ptr);
	}

	for(i = 0; i < NOC_PACKET_SIZE; i++)
		_ni_read();

	_irq_register(IRQ_NOC_READ, (funcptr)recv_isr);
	_irq_mask_set(IRQ_NOC_READ);

	kprintf("\nKERNEL: NoC driver registered");
}

void recv_isr(void *arg)
{
	uint16_t target_cpu, payload, source_cpu, source_task, target_task, msg_size, seq, channel;
	int32_t i;
	uint16_t *buf_ptr;

	_di();
	
	_ni_read();
	target_cpu = _ni_read();
	payload = _ni_read();
	
	if (payload != NOC_PACKET_SIZE - 2)
		return;
	
	source_cpu = _ni_read();
	source_task = _ni_read();
	target_task = _ni_read();
	msg_size = _ni_read();
	seq = _ni_read();
	channel = _ni_read();

	if (target_task < MAX_TASKS && krnl_tcb[target_task].ptask){
		if (pktdrv_tqueue[target_task]){
			buf_ptr = hf_queue_remhead(pktdrv_queue);
			if (buf_ptr){
				buf_ptr[PKT_TARGET_CPU] = target_cpu;
				buf_ptr[PKT_PAYLOAD] = payload;
				buf_ptr[PKT_SOURCE_CPU] = source_cpu;
				buf_ptr[PKT_SOURCE_TASK] = source_task;
				buf_ptr[PKT_TARGET_TASK] = target_task;
				buf_ptr[PKT_MSG_SIZE] = msg_size;
				buf_ptr[PKT_SEQ] = seq;
				buf_ptr[PKT_CHANNEL] = channel;

				for (i = PKT_HEADER_SIZE; i < NOC_PACKET_SIZE; i++)
					buf_ptr[i] = _ni_read();

				if (hf_queue_addtail(pktdrv_tqueue[target_task], buf_ptr)){
					kprintf("\nKERNEL: task %d queue full! dropping packet...", target_task);
					hf_queue_addtail(pktdrv_queue, buf_ptr);
				}
			}else{
				kprintf("\nKERNEL: NoC queue full! dropping packet...");
				for (i = PKT_HEADER_SIZE; i < NOC_PACKET_SIZE; i++)
					_ni_read();
			}
		}else{
			kprintf("KERNEL: task %d has no comm created! dropping packet...", target_task);
			for (i = PKT_HEADER_SIZE; i < NOC_PACKET_SIZE; i++)
				_ni_read();
		}
	}else{
		kprintf("\nKERNEL: no task with id %d (offender: cpu %d task %d) - dropping packet...", target_task, source_cpu, source_task);
		for (i = PKT_HEADER_SIZE; i < NOC_PACKET_SIZE; i++)
			_ni_read();
	}

	return;
}

void sysmon(void)
{
	for (;;){
	}
}

int32_t hf_comm_create(uint16_t id, uint16_t packets)
{
	if (id < MAX_TASKS){
		if (krnl_tcb[id].ptask == 0)
			return HF_INVALID_ID;
	}else{
		return HF_INVALID_ID;
	}
	
	if (packets > NOC_PACKET_SLOTS || packets == 0)
		packets = NOC_PACKET_SLOTS;
	
	pktdrv_tqueue[id] = hf_queue_create(packets);
	if (pktdrv_tqueue[id] == 0)
		return HF_OUT_OF_MEMORY;
	else
		return HF_OK;
}

int32_t hf_comm_destroy(uint16_t id)
{
	int32_t status;

	if (id < MAX_TASKS){
		if (krnl_tcb[id].ptask == 0)
			return HF_INVALID_ID;
	}else{
		return HF_INVALID_ID;
	}
	
	status = _di();
	while (hf_queue_count(pktdrv_tqueue[id]))
		hf_queue_addtail(pktdrv_queue, hf_queue_remhead(pktdrv_tqueue[id]));
	_ei(status);
	
	if (hf_queue_destroy(pktdrv_tqueue[id]))
		return HF_ERROR;
	else
		return HF_OK;
		
}

int32_t hf_recv(uint16_t *source_cpu, uint16_t *source_id, int8_t *buf, uint16_t *size, uint16_t channel)
{
	uint16_t target_cpu, payload, target_task, seq = 0, id;
	uint16_t packet = 0, packets, payload_bytes;
	uint32_t status;
	int32_t i, k, e, p = 0, error = HF_OK;
	uint16_t *buf_ptr;

	id = hf_selfid();
	if (pktdrv_tqueue[id] == NULL) return HF_COMM_UNFEASIBLE;
	
	while (1){
		k = hf_queue_count(pktdrv_tqueue[id]);
		if (k){
			buf_ptr = hf_queue_get(pktdrv_tqueue[id], 0);
			if (buf_ptr)
				if (buf_ptr[PKT_CHANNEL] == channel) break;
		}
	}
		
	status = _di();
	buf_ptr = hf_queue_remhead(pktdrv_tqueue[id]);
	_ei(status);
	
	target_cpu = buf_ptr[PKT_TARGET_CPU];
	payload = buf_ptr[PKT_PAYLOAD];
	*source_cpu = buf_ptr[PKT_SOURCE_CPU];
	*source_id = buf_ptr[PKT_SOURCE_TASK];
	target_task = buf_ptr[PKT_TARGET_TASK];
	*size = buf_ptr[PKT_MSG_SIZE];
	seq = buf_ptr[PKT_SEQ];
			
	payload_bytes = (NOC_PACKET_SIZE - PKT_HEADER_SIZE) * sizeof(uint16_t);
	(*size % payload_bytes == 0)?(packets = *size / payload_bytes):(packets = *size / payload_bytes + 1);
			
	while (++packet < packets){
		if (buf_ptr[PKT_SEQ] != seq++)
			error = HF_SEQ_ERROR;
			
		for (i = PKT_HEADER_SIZE; i < NOC_PACKET_SIZE; i++){
			buf[p++] = (uint8_t)(buf_ptr[i] >> 8);
			buf[p++] = (uint8_t)(buf_ptr[i] & 0xff);
		}
		status = _di();
		hf_queue_addtail(pktdrv_queue, buf_ptr);
		_ei(status);
		
		while (1){
			k = hf_queue_count(pktdrv_tqueue[id]);
			if (k){
				buf_ptr = hf_queue_get(pktdrv_tqueue[id], 0);
				if (buf_ptr)
					if (buf_ptr[PKT_CHANNEL] == channel) break;
			}
		}
		status = _di();
		buf_ptr = hf_queue_remhead(pktdrv_tqueue[id]);
		_ei(status);
	}
	
	if (buf_ptr[PKT_SEQ] != seq++)
		error = HF_SEQ_ERROR;

	for (i = PKT_HEADER_SIZE; i < NOC_PACKET_SIZE && p < *size; i++){
		buf[p++] = (uint8_t)(buf_ptr[i] >> 8);
		buf[p++] = (uint8_t)(buf_ptr[i] & 0xff);
	}
	status = _di();
	hf_queue_addtail(pktdrv_queue, buf_ptr);
	_ei(status);
	
	return error;
}

int32_t hf_send(uint16_t target_cpu, uint16_t target_id, int8_t *buf, uint16_t size, uint16_t channel)
{
	uint16_t source_cpu, payload, source_task, target_task;
	uint16_t packet = 0, packets, payload_bytes;
	uint32_t status;
	int32_t i, p = 0;
	uint16_t out_buf[NOC_PACKET_SIZE];

	payload_bytes = (NOC_PACKET_SIZE - PKT_HEADER_SIZE) * sizeof(uint16_t);
	(size % payload_bytes == 0)?(packets = size / payload_bytes):(packets = size / payload_bytes + 1);

	while (++packet < packets){
		out_buf[PKT_TARGET_CPU] = (NOC_COLUMN(target_cpu) << 4) | NOC_LINE(target_cpu);
		out_buf[PKT_PAYLOAD] = NOC_PACKET_SIZE - 2;
		out_buf[PKT_SOURCE_CPU] = hf_cpuid();
		out_buf[PKT_SOURCE_TASK] = hf_selfid();
		out_buf[PKT_TARGET_TASK] = target_id;
		out_buf[PKT_MSG_SIZE] = size;
		out_buf[PKT_SEQ] = packet;
		out_buf[PKT_CHANNEL] = channel;
		
		for (i = PKT_HEADER_SIZE; i < NOC_PACKET_SIZE; i++, p+=2)
			out_buf[i] = (buf[p] << 8) | buf[p+1];

again1:		while ((_ni_status() & 0x1) == 0);
		status = _di();
		if ((_ni_status() & 0x1) == 0){
			_ei(status);
			goto again1;
		}
		for (i = 0; i < NOC_PACKET_SIZE; i++)
			_ni_write(out_buf[i]);
		_ei(status);
	}

	out_buf[PKT_TARGET_CPU] = (NOC_COLUMN(target_cpu) << 4) | NOC_LINE(target_cpu);
	out_buf[PKT_PAYLOAD] = NOC_PACKET_SIZE - 2;
	out_buf[PKT_SOURCE_CPU] = hf_cpuid();
	out_buf[PKT_SOURCE_TASK] = hf_selfid();
	out_buf[PKT_TARGET_TASK] = target_id;
	out_buf[PKT_MSG_SIZE] = size;
	out_buf[PKT_SEQ] = packet;
	out_buf[PKT_CHANNEL] = channel;

	for (i = PKT_HEADER_SIZE; i < NOC_PACKET_SIZE && (p < size); i++, p+=2)
		out_buf[i] = (buf[p] << 8) | buf[p+1];
	for(; i < NOC_PACKET_SIZE; i++)
		out_buf[i] = 0xdead;

again2:	while ((_ni_status() & 0x1) == 0);
	status = _di();
	if ((_ni_status() & 0x1) == 0){
		_ei(status);
		goto again2;
	}
	for (i = 0; i < NOC_PACKET_SIZE; i++)
		_ni_write(out_buf[i]);
	_ei(status);
	
	return HF_OK;
}

int32_t hf_queryid(uint16_t target_cpu, int8_t *desc)
{
}

int32_t hf_getid(uint16_t *cpu, int32_t *tid)
{
}
