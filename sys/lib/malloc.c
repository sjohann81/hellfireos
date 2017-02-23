#include <hal.h>
#include <libc.h>
#include <malloc.h>
#include <mutex.h>
#include <kernel.h>

static mutex_t krnl_malloc;

#if MEM_ALLOC == 0
/*
 * very simple and fast memory allocator.
 * causes lots of fragmentation, but its no big deal if memory is rarely freed.
 *
 * this allocator is not recommended.
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

void hf_free(void *ptr)
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

void *hf_malloc(uint32_t size)
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
 * calls to alloc_update_hf_free() on malloc() can be avoided to improve speed.
 * 
 * this allocator is not recommended.
 */

static mem_header_t base;
static mem_header_t *freep = 0;
static uint32_t pool_free_pos = 0;

static void alloc_print_stats(void)
{
	mem_header_t *p;

	kprintf("memory manager stats:\n\n");
	kprintf("pool: free_pos = %d (%d bytes left)\n\n", pool_free_pos, HEAP_SIZE - pool_free_pos);

	p = (mem_header_t *)krnl_heap;
	while (p < (mem_header_t *) (krnl_heap + pool_free_pos)){
		kprintf("  * addr: 0x%x; size: %d\n", p, p->s.size);
		p += p->s.size;
	}
	kprintf("\nfree list:\n\n");
	if (freep){
		p = freep;

		while (1){
			kprintf("  * addr: 0x%x; size: %d; next: 0x%x\n", p, p->s.size, p->s.next);
			p = p->s.next;
			if (p == freep)
				break;
		}
	}else{
		kprintf("empty\n");
	}
	kprintf("\n");
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

void hf_free(void *ptr)
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

void *hf_malloc(uint32_t size)
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
			krnl_free -= p->s.size * sizeof(mem_header_t);
			hf_mtxunlock(&krnl_malloc);
			
			return (void*) (p + 1);
		}else{
			if (p == freep){
				if ((p = morecore(nquantas)) == 0){
					hf_mtxunlock(&krnl_malloc);
					return 0;
				}
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

#if MEM_ALLOC == 2
/*
 * memory allocator using first-fit
 * 
 * simple linked list of used/free areas. malloc() is very fast on sucessive
 * allocations as a pointer to the last allocated area is kept, while hf_free()
 * is slow. malloc() performance suffers when hf_free() is performed. the next
 * allocator performs better on the average case.
 */
 
void hf_free(void *ptr)
{
	struct mem_block *p, *q;
	
	hf_mtxlock(&krnl_malloc);
	p = ((struct mem_block *)ptr) - 1;
	p->size &= 0xfffffffe;
	krnl_free += p->size + sizeof(struct mem_block);

	p = (struct mem_block *)krnl_heap;
	ff = p;
	q = p;
	while (p->next != NULL){
		while (p->size & 1){
			p = p->next;
			q = p;
		}
		while (!(p->size & 1) && p->next != NULL)
			p = p->next;
		if (p){
			q->size = (size_t)p - (size_t)q - sizeof(struct mem_block);
			q->next = p;
		}
	}
	
	hf_mtxunlock(&krnl_malloc);
}

void *hf_malloc(uint32_t size)
{
	struct mem_block *p, *r, n;
	size_t psize;
	
	size = align4(size);
	
	hf_mtxlock(&krnl_malloc);

	p = ff;
	while (p->size < size + sizeof(struct mem_block) || p->size & 1){
		if (p->next == NULL && p->size < size){
			hf_mtxunlock(&krnl_malloc);
			return 0;
		}
		p = p->next;
	}
	ff = p;
	psize = (p->size & 0xfffffffe) - size - sizeof(struct mem_block);
	
	r = p->next;
	p->next = (struct mem_block *)((size_t)p + size + sizeof(struct mem_block));
	p->size = size | 1;
	
	n.next = r;
	n.size = psize;
	*p->next = n;
	krnl_free -= size + sizeof(struct mem_block);
	hf_mtxunlock(&krnl_malloc);

	return (void *)(p + 1);
}

void heapinit(void *heap, uint32_t len)
{
	struct mem_block *p = (struct mem_block *)heap;
	struct mem_block *q = (struct mem_block *)((size_t)(struct mem_block *)heap + len - (sizeof(struct mem_block)));
	
	len = align4(len);
	p->next = q;
	p->size = len - sizeof(struct mem_block) - sizeof(struct mem_block);
	q->next = NULL;
	q->size = 0;
	ff = (struct mem_block *)krnl_heap;
	krnl_free = p->size;
	hf_mtxinit(&krnl_malloc);
}
#endif

#if MEM_ALLOC == 3
/*
 * memory allocator using first-fit
 * 
 * simple linked list of used/free areas. malloc() is slower, because free areas
 * are searched from the beginning of the heap and are coalesced on demand. yet,
 * just one sweep through memory areas is performed, making it faster than the
 * previous allocator on the average case. hf_free() is very fast, as memory areas
 * are just marked as unused.
 */
void hf_free(void *ptr)
{
	struct mem_block *p;
	
	hf_mtxlock(&krnl_malloc);
	p = ((struct mem_block *)ptr) - 1;
	p->size &= ~1L;
	last_free = first_free;
	krnl_free += p->size + sizeof(struct mem_block);
	hf_mtxunlock(&krnl_malloc);
}

void *hf_malloc(uint32_t size)
{
	struct mem_block *p, *q, *r, n;
	
	size = align4(size);
	
	hf_mtxlock(&krnl_malloc);

	p = last_free;
	q = p;

	while (p->next){
		while (p->size & 1){
			p = p->next;
			q = p;
		}
		while (!(p->size & 1) && p->next)
			p = p->next;
		if (p){
			q->size = (size_t)p - (size_t)q - sizeof(struct mem_block);
			q->next = p;
		}
		if (q->size >= size + sizeof(struct mem_block)){
			p = q;
			break;
		}
	}

	if (p->next == NULL){
		hf_mtxunlock(&krnl_malloc);
		return 0;
	}
	
	last_free = p;
	r = p->next;
	p->next = (struct mem_block *)((size_t)p + size + sizeof(struct mem_block));
	p->size = size | 1;
	n.next = r;
	n.size = (p->size & ~1L) - size - sizeof(struct mem_block);
	*p->next = n;
	krnl_free -= size + sizeof(struct mem_block);
	
	hf_mtxunlock(&krnl_malloc);

	return (void *)(p + 1);
}

void heapinit(void *heap, uint32_t len)
{
	struct mem_block *p = (struct mem_block *)heap;
	struct mem_block *q = (struct mem_block *)((size_t)(struct mem_block *)heap + len - (sizeof(struct mem_block)));
	
	len = align4(len);
	p->next = q;
	p->size = len - sizeof(struct mem_block) - sizeof(struct mem_block);
	q->next = NULL;
	q->size = 0;
	first_free = (struct mem_block *)heap;
	last_free = (struct mem_block *)heap;
	krnl_free = p->size;
	hf_mtxinit(&krnl_malloc);
}
#endif

void *hf_calloc(uint32_t qty, uint32_t type_size)
{
	void *buf;
	
	buf = (void *)hf_malloc((qty * type_size));
	if (buf)
		memset(buf, 0, (qty * type_size));

	return (void *)buf;
}

void *hf_realloc(void *ptr, uint32_t size){
	void *buf;

	if ((int32_t)size < 0) return NULL;
	if (ptr == NULL)
		return (void *)hf_malloc(size);

	buf = (void *)hf_malloc(size);
	if (buf){
		memcpy(buf, ptr, size);
		hf_free(ptr);
	}

	return (void *)buf;
}

