/**
 * @internal
 * @file malloc.h
 * @author Sergio Johann Filho
 * @date February 2016
 * 
 * @section LICENSE
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file 'doc/license/gpl-2.0.txt' for more details.
 * 
 * @section DESCRIPTION
 * 
 * Data structures of several memory allocators.
 * 
 */

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

#if MEM_ALLOC == 2
#define align4(x) ((((x) + 3) >> 2) << 2)

struct mem_block {
	struct mem_block *next;		/* pointer to the next block */
	size_t size;			/* aligned block size. the LSB is used to define if the block is used */
};

struct mem_block *ff;
#endif

#if MEM_ALLOC == 3
#define align4(x) ((((x) + 3) >> 2) << 2)

struct mem_block {
	struct mem_block *next;		/* pointer to the next block */
	size_t size;			/* aligned block size. the LSB is used to define if the block is used */
};

struct mem_block *first_free;
struct mem_block *last_free;
#endif

void hf_free(void *ptr);
void *hf_malloc(uint32_t size);
void heapinit(void *heap, uint32_t len);
void *hf_calloc(uint32_t qty, uint32_t type_size);
void *hf_realloc(void *ptr, uint32_t size);
