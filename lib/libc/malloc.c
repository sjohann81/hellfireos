#include <hellfire.h>

extern uint32_t krln_free;
static mutex_t krnl_malloc;

#if MEM_ALLOC == 0
/*
 * very simple and fast memory allocator.
 * causes lots of fragmentation, but its no big deal if memory is rarely freed.
 */

static mem_chunk *compact(mem_chunk *p, uint32_t nsize)
{
	uint32_t bsize, psize;
	mem_chunk *best_fit;

	best_fit = p;
	bsize = 0;

	while(psize = p->size, psize){
		if(psize & 1){
			if(bsize != 0){
				best_fit->size = bsize;
				if(bsize >= nsize) return best_fit;
			}
			bsize = 0;
			best_fit = p = (mem_chunk *)((uint32_t)p + (psize & ~1));
		}else{
			bsize += psize;
			p = (mem_chunk *)((uint32_t)p + psize);
		}
	}

	if(bsize != 0){
		best_fit->size = bsize;
		if(bsize >= nsize) return best_fit;
	}

	return NULL;
}

void free(void *ptr)
{
	mem_chunk *p;

	hf_mtxlock(&krnl_malloc);
	if(ptr){
		p = (mem_chunk *)((uint32_t)ptr - sizeof(mem_chunk));
		p->size &= ~1;
	}
	hf_mtxunlock(&krnl_malloc);
	krnl_free = krnl_heap_ptr.free->size;
}

void *malloc(uint32_t size)
{
	uint32_t fsize;
	mem_chunk *p;

	if(size == 0) return NULL;

	hf_mtxlock(&krnl_malloc);
	size  += 3 + sizeof(mem_chunk);
	size >>= 2;
	size <<= 2;

	if((krnl_heap_ptr.free == 0) || (size >krnl_heap_ptr.free->size)){
		krnl_heap_ptr.free = compact(krnl_heap_ptr.heap, size);
		if(krnl_heap_ptr.free == 0){
			hf_mtxunlock(&krnl_malloc);
			return NULL;
		}
	}

	p = krnl_heap_ptr.free;
	fsize = krnl_heap_ptr.free->size;

	if(fsize >= size + sizeof(mem_chunk)){
		krnl_heap_ptr.free = (mem_chunk *)((uint32_t)p + size);
		krnl_heap_ptr.free->size = fsize - size;
	}else{
		krnl_heap_ptr.free = 0;
		size = fsize;
	}

	p->size = size | 1;
	hf_mtxunlock(&krnl_malloc);
	krnl_free = krnl_heap_ptr.free->size;

	return (void *)((uint32_t)p + sizeof(mem_chunk));
}

void heapinit(void *heap, uint32_t len)
{
	len  += 3;
	len >>= 2;
	len <<= 2;
	krnl_heap_ptr.free = krnl_heap_ptr.heap = (mem_chunk *) heap;
	krnl_heap_ptr.free->size = krnl_heap_ptr.heap->size = len - sizeof(mem_chunk);
	*(uint32_t *)((int8_t *)heap + len - 4) = 0;
	krnl_free = krnl_heap_ptr.free->size;
	hf_mtxinit(&krnl_malloc);
}
#endif

#if MEM_ALLOC == 1
/*
 * another simple memory allocator, based on the K&R C book.
 * memory is allocated using multiples of MIN_POOL_ALLOC_QUANTAS.
 * each 'quanta' is sizeof(mem_header_t), which is 8 bytes.
 * calls to alloc_update_free() on malloc() can be avoided to improve speed.
 */

static mem_header_t base;
static mem_header_t *freep = 0;
static uint32_t pool_free_pos = 0;

static void alloc_print_stats(void)
{
	mem_header_t *p;

	printf("memory manager stats:\n\n");
	printf("pool: free_pos = %d (%d bytes left)\n\n", pool_free_pos, HEAP_SIZE - pool_free_pos);

	p = (mem_header_t *)krnl_heap;
	while (p < (mem_header_t *) (krnl_heap + pool_free_pos)){
		printf("  * addr: 0x%x; size: %d\n", p, p->s.size);
		p += p->s.size;
	}
	printf("\nfree list:\n\n");
	if (freep){
		p = freep;

		while (1){
			printf("  * addr: 0x%x; size: %d; next: 0x%x\n", p, p->s.size, p->s.next);
			p = p->s.next;
			if (p == freep)
				break;
		}
	}else{
		printf("empty\n");
	}
	printf("\n");
}

static void alloc_update_free(void)
{
	mem_header_t *q;
	
	q = (mem_header_t *)krnl_heap;
	krnl_free = HEAP_SIZE;
	while (q < (mem_header_t *) (krnl_heap + pool_free_pos)){
		q += q->s.size;
		krnl_free -= q->s.size * sizeof(mem_header_t);
	}
}

static void free2(void *ptr)
{
	mem_header_t *block;
	mem_header_t *p;

	block = ((mem_header_t *)ptr) - 1;

	for (p = freep; !(block > p && block < p->s.next); p = p->s.next){
		if (p >= p->s.next && (block > p || block < p->s.next))
			break;
	}

	if (block + block->s.size == p->s.next){
		block->s.size += p->s.next->s.size;
		block->s.next = p->s.next->s.next;
	}else{
		block->s.next = p->s.next;
	}

	if (p + p->s.size == block){
		p->s.size += block->s.size;
		p->s.next = block->s.next;
	}else{
		p->s.next = block;
	}

	freep = p;
}

void free(void *ptr)
{
	hf_mtxlock(&krnl_malloc);
	free2(ptr);
	hf_mtxunlock(&krnl_malloc);
}

static mem_header_t *morecore(uint32_t nquantas){
	uint32_t total_req_size;
	mem_header_t *h;

	if (nquantas < MIN_POOL_ALLOC_QUANTAS)
		nquantas = MIN_POOL_ALLOC_QUANTAS;
	total_req_size = nquantas * sizeof(mem_header_t);

	if (pool_free_pos + total_req_size <= HEAP_SIZE){
		h = (mem_header_t *)(krnl_heap + pool_free_pos);
		h->s.size = nquantas;
		free2((void*)(h + 1));
		pool_free_pos += total_req_size;
	}else{
		return 0;
	}

	return freep;
}

void *malloc(uint32_t size)
{
	mem_header_t *p;
	mem_header_t *prevp;

	uint32_t nquantas = (size + sizeof(mem_header_t) - 1) / sizeof(mem_header_t) + 1;
	
	hf_mtxlock(&krnl_malloc);

	if ((prevp = freep) == 0){
		base.s.next = freep = prevp = &base;
		base.s.size = 0;
	}

	for (p = prevp->s.next; ; prevp = p, p = p->s.next){
		if (p->s.size >= nquantas){
			if (p->s.size == nquantas){
				prevp->s.next = p->s.next;
			}else{
				p->s.size -= nquantas;
				p += p->s.size;
				p->s.size = nquantas;
			}
			freep = prevp;
			alloc_update_free();
			hf_mtxunlock(&krnl_malloc);
			
			return (void*) (p + 1);
		}
		if (p == freep){
			if ((p = morecore(nquantas)) == 0){
				hf_mtxunlock(&krnl_malloc);
				return 0;
			}
		}
	}
}

void heapinit(void *heap, uint32_t len)
{
	base.s.next = 0;
	base.s.size = 0;
	freep = 0;
	pool_free_pos = 0;
	krnl_free = HEAP_SIZE;
	hf_mtxinit(&krnl_malloc);
}
#endif

void *calloc(uint32_t qty, uint32_t type_size)
{
	void *buf;
	
	buf = (void *)malloc((qty * type_size));
	if (buf)
		memset(buf, 0, (qty * type_size));

	return (void *)buf;
}

void *realloc(void *ptr, uint32_t size){
	void *buf;

	if ((int32_t)size < 0) return NULL;
	if (ptr == NULL)
		return (void *)malloc(size);

	buf = (void *)malloc(size);
	if (buf){
		memcpy(buf, ptr, size);
		free(ptr);
	}

	return (void *)buf;
}

