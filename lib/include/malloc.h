#if MEM_ALLOC == 0
typedef struct{
	uint32_t size;
} mem_chunk;

typedef struct{
	mem_chunk *free;
	mem_chunk *heap;
} mem_chunk_ptr;

mem_chunk_ptr krnl_heap_ptr;
#endif

#if MEM_ALLOC == 1
#define MIN_POOL_ALLOC_QUANTAS 16

typedef uint32_t align;

union mem_header_union{
	struct {
		union mem_header_union *next;
		uint32_t size; 
	} s;
	align align_dummy;
};

typedef union mem_header_union mem_header_t;
#endif

void free(void *ptr);
void *malloc(uint32_t size);
void heapinit(void *heap, uint32_t len);
void *calloc(uint32_t qty, uint32_t type_size);
void *realloc(void *ptr, uint32_t size);
