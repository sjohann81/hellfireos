/* 
 * malloctest.c
 *
 * This program contains a variety of tests for malloc and free.
 * XXX most tests leak on error.
 *
 * These tests (subject to restrictions and limitations noted below) should
 * work once user-level malloc is implemented.
 */

//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <err.h>
#include <hellfire.h>

#define _PATH_RANDOM   "random:"

#define SMALLSIZE   72
#define MEDIUMSIZE  896
#define BIGSIZE     16384
//#define HUGESIZE    (1024 * 1024 * 1024)
#define HUGESIZE    65536

/* Maximum amount of space per block we allow for indexing structures */
#define OVERHEAD         32

/* Point past which we assume something else is going on */
#define ABSURD_OVERHEAD  256

static
int
geti(void)
{
	int val=0;
	int ch, digits=0;

	while (1) {
		ch = getchar();
		if (ch=='\n' || ch=='\r') {
			putchar('\n');
			break;
		}
		else if ((ch=='\b' || ch==127) && digits>0) {
			printf("\b \b");
			val = val/10;
			digits--;
		}
		else if (ch>='0' && ch<='9') {
			putchar(ch);
			val = val*10 + (ch-'0');
			digits++;
		}
		else {
			putchar('\a');
		}
	}

	if (digits==0) {
		return -1;
	}
	return val;
}

////////////////////////////////////////////////////////////

/*
 * Fill a block of memory with a test pattern.
 */
static
void
markblock(volatile void *ptr, size_t size, unsigned bias, int doprint)
{
	size_t n, i;
	unsigned long *pl;
	unsigned long val;

	pl = (unsigned long *)ptr;
	n = size / sizeof(unsigned long);

	for (i=0; i<n; i++) {
		val = ((unsigned long)i ^ (unsigned long)bias);
		pl[i] = val;
		if (doprint && (i%64==63)) {
			printf(".");
		}
	}
	if (doprint) {
		printf("\n");
	}
}

/*
 * Check a block marked with markblock()
 */
static
int
checkblock(volatile void *ptr, size_t size, unsigned bias, int doprint)
{
	size_t n, i;
	unsigned long *pl;
	unsigned long val;

	pl = (unsigned long *)ptr;
	n = size / sizeof(unsigned long);

	for (i=0; i<n; i++) {
		val = ((unsigned long)i ^ (unsigned long)bias);
		if (pl[i] != val) {
			if (doprint) {
				printf("\n");
			}
			printf("FAILED: data mismatch at offset %d of block "
			       "at 0x%lx: %d vs. %d\n",
			       (unsigned long) (i*sizeof(unsigned long)),
			       (unsigned long)(uint32_t *)pl,
			       pl[i], val);
			return -1;
		}
		if (doprint && (i%64==63)) {
			printf(".");
		}
	}
	if (doprint) {
		printf("\n");
	}

	return 0;
}

////////////////////////////////////////////////////////////

/*
 * Test 1
 *
 * This test checks if all the bytes we asked for are getting
 * allocated.
 */

static
void
test1(void)
{
	volatile unsigned *x;

	printf("*** Malloc test 1 ***\n");
	printf("Allocating %d bytes\n", BIGSIZE);
	x = malloc(BIGSIZE);
	if (x==NULL) {
		printf("FAILED: malloc failed\n");
		return;
	}

	markblock(x, BIGSIZE, 0, 0);
	if (checkblock(x, BIGSIZE, 0, 0)) {
		printf("FAILED: data corrupt\n");
		return;
	}

	free((void *)x);

	printf("Passed malloc test 1.\n");
}


////////////////////////////////////////////////////////////

/* 
 * Test 2
 *
 * Tests if malloc gracefully handles failing requests.
 *
 * This test assumes that one of the following conditions holds:
 *    1. swap is not overcommitted; or
 *    2. user processes are limited to some maximum size, and enough
 *       swap exists to hold a maximal user process.
 *
 * That is, it assumes that malloc returns NULL when out of memory,
 * and that the process will not be killed for running out of
 * memory/swap at other times.
 *
 * If mallocing more memory than the system can actually provide
 * backing for succeeds, this test will blow up. That's ok, but please
 * provide a way to switch on one of the above conditions so this test
 * can be run.
 *
 * This test works by trying a huge malloc, and then trying
 * successively smaller mallocs until one works. Then it touches the
 * whole block to make sure the memory is actually successfully
 * allocated. Then it frees the block and allocates it again, which
 * should succeed.
 *
 * Note that this test may give spurious failures if anything else is
 * running at the same time and changing the amount of memory
 * available.
 */

static
void
test2(void)
{
	volatile unsigned *x;
	size_t size;
	
	printf("Entering malloc test 2.\n");
	printf("Make sure you read and understand the comment in malloctest.c "
	       "that\nexplains the conditions this test assumes.\n\n");

	printf("Testing how much memory we can allocate:\n");
	
	for (size = HUGESIZE; (x = malloc(size))==NULL; size = size/2) {
		printf("  %d bytes: failed\n", (unsigned long) size);
	}
	printf("  %d bytes: succeeded\n", (unsigned long) size);

	printf("Passed part 1\n");

	printf("Touching all the words in the block.\n");
	markblock(x, size, 0, 1);

	printf("Validating the words in the block.\n");
	if (checkblock(x, size, 0, 1)) {
		printf("FAILED: data corrupt\n");
		return;
	}
	printf("Passed part 2\n");


	printf("Freeing the block\n");
	free((void *)x);
	printf("Passed part 3\n");
	printf("Allocating another block\n");
	
	x = malloc(size);
	if (x==NULL) {
		printf("FAILED: free didn't return the memory?\n");
		return;
	}
	free((void *)x);

	printf("Passed malloc test 2.\n");
}


////////////////////////////////////////////////////////////

/* 
 * Test 3
 *
 * Tests if malloc gracefully handles failing requests.
 *
 * This test assumes the same conditions as test 2.
 *
 * This test works by mallocing a lot of small blocks in a row until
 * malloc starts failing.
 */

struct test3 {
	struct test3 *next;
	char junk[(SMALLSIZE - sizeof(struct test3 *))];
};

static
void
test3(void)
{
	struct test3 *list = NULL, *tmp;
	size_t tot=0;
	int ct=0, failed=0;
	void *x;

	printf("Entering malloc test 3.\n");
	printf("Make sure you read and understand the comment in malloctest.c "
	       "that\nexplains the conditions this test assumes.\n\n");

	printf("Testing how much memory we can allocate:\n");

	while ((tmp = malloc(sizeof(struct test3))) != NULL) {

		tmp->next = list;
		list = tmp;

		tot += sizeof(struct test3);

		markblock(list->junk, sizeof(list->junk), (uint32_t *)list, 0);

		ct++;
		if (ct%128==0) {
			printf(".");
		}
	}

	printf("Allocated %d bytes\n", (unsigned long) tot);

	printf("Trying some more allocations which I expect to fail...\n");

	x = malloc(SMALLSIZE);
	if (x != NULL) {
		printf("FAILED: malloc(%d) succeeded\n", SMALLSIZE);
		return;
	}

	x = malloc(MEDIUMSIZE);
	if (x != NULL) {
		printf("FAILED: malloc(%d) succeeded\n", MEDIUMSIZE);
		return;
	}

	x = malloc(BIGSIZE);
	if (x != NULL) {
		printf("FAILED: malloc(%d) succeeded\n", BIGSIZE);
		return;
	}

	printf("Ok, now I'm going to free everything...\n");

	while (list != NULL) {
		tmp = list->next;

		if (checkblock(list->junk, sizeof(list->junk), 
			       (uint32_t *)list, 0)) {
			failed = 1;
		}

		free(list);
		list = tmp;
	}

	if (failed) {
		printf("FAILED: data corruption\n");
		return;
	}

	printf("Let me see if I can allocate some more now...\n");

	x = malloc(MEDIUMSIZE);
	if (x == NULL) {
		printf("FAIL: Nope, I couldn't.\n");
		return;
	}
	free(x);
	
	printf("Passed malloc test 3\n");
}

////////////////////////////////////////////////////////////

/*
 * Test 4
 *
 * Tries to test in detail if malloc coalesces the free list properly.
 *
 * This test will likely fail if something other than a basic first-fit/
 * next-fit/best-fit algorithm is used.
 */

static
void
test4(void)
{
	void *x, *y, *z;
	unsigned long lx, ly, lz, overhead, zsize;

	printf("Entering malloc test 4.\n");
	printf("This test is intended for first/best-fit based mallocs.\n");
	printf("This test may not work correctly if run after other tests.\n");

	printf("Testing free list coalescing:\n");

	x = malloc(SMALLSIZE);
	if (x==NULL) {
		printf("FAILED: malloc(%d) failed\n", SMALLSIZE);
		return;
	}

	y = malloc(MEDIUMSIZE);
	if (y==NULL) {
		printf("FAILED: malloc(%d) failed\n", MEDIUMSIZE);
		return;
	}

	if (sizeof(unsigned long) < sizeof(void *)) {
		printf("Buh? I can't fit a void * in an unsigned long\n");
		printf("ENVIRONMENT FAILED...\n");
		return;
	}

	lx = (unsigned long)x;
	ly = (unsigned long)y;

	printf("x is 0x%x; y is 0x%x\n", lx, ly);

	/*
	 * The memory should look something like this:
	 *
	 *     OXXXOYYYYYYYYYYY
	 *
	 * where O are optional overhead (indexing) blocks.
	 */

	/* This is obviously wrong. */
	if (lx == ly) {
		printf("FAIL: x == y\n");
		return;
	}

	/*
	 * Check for overlap. It is sufficient to check if the start
	 * of each block is within the other block. (If the end of a
	 * block is within the other block, either the start is too,
	 * or the other block's start is within the first block.)
	 */
	if (lx < ly && lx + SMALLSIZE > ly) {
		printf("FAIL: y starts within x\n");
		return;
	}
	if (ly < lx && ly + MEDIUMSIZE > lx) {
		printf("FAIL: x starts within y\n");
		return;
	}

	/*
	 * If y is lower than x, some memory scheme we don't
	 * understand is in use, or else there's already stuff on the
	 * free list.
	 */
	if (ly < lx) {
		printf("TEST UNSUITABLE: y is below x\n");
		return;
	}

	/*
	 * Compute the space used by index structures.
	 */
	overhead = ly - (lx + SMALLSIZE);
	printf("Apparent block overhead: %d\n", overhead);

	if (overhead > ABSURD_OVERHEAD) {
		printf("TEST UNSUITABLE: block overhead absurdly large.\n");
		return;
	}
	if (overhead > OVERHEAD) {
		printf("FAIL: block overhead is too large.\n");
		return;
	}

	printf("Freeing blocks...\n");
	free(x);
	free(y);

	zsize = SMALLSIZE + MEDIUMSIZE + overhead;

	printf("Now allocating %d bytes... should reuse the space.\n", zsize);
	z = malloc(zsize);
	if (z == NULL) {
		printf("FAIL: Allocation failed...\n");
		return;
	}

	lz = (unsigned long) z;

	printf("z is 0x%x (x was 0x%x, y 0x%x)\n", lz, lx, ly);

	if (lz==lx) {
		printf("Passed.\n");
	}
	else {
		printf("Failed.\n");
	}

	free(z);
}

////////////////////////////////////////////////////////////

/*
 * Test 5/6/7
 *
 * Generally beats on malloc/free.
 *
 * Test 5 uses random seed 0.
 * Test 6 seeds the random number generator from random:.
 * Test 7 asks for a seed.
 */

static
void
test567(int testno, unsigned long seed)
{
	static const int sizes[8] = { 13, 17, 69, 176, 433, 871, 1150, 6060 };
	
	void *ptrs[32];
	int psizes[32];
	int i, n, size, failed=0;

	srand(seed);
	printf("Seeded random number generator with %d.\n", seed);

	for (i=0; i<32; i++) {
		ptrs[i] = NULL;
		psizes[i] = 0;
	}

	for (i=0; i<100000; i++) {
		n = random()%32;
		if (ptrs[n] == NULL) {
			size = sizes[random()%8];
			ptrs[n] = malloc(size);
			psizes[n] = size;
			if (ptrs[n] == NULL) {
				printf("\nmalloc %d failed\n", size);
				failed = 1;
				break;
			}
			markblock(ptrs[n], size, n, 0);
		}
		else {
			size = psizes[n];
			if (checkblock(ptrs[n], size, n, 0)) {
				failed = 1;
				break;
			}
			free(ptrs[n]);
			ptrs[n] = NULL;
			psizes[n] = 0;
		}
		if (i%256==0) {
			printf(".");
		}
	}
	printf("\n");

	for (i=0; i<32; i++) {
		if (ptrs[i] != NULL) {
			free(ptrs[i]);
		}
	}

	if (failed) {
		printf("FAILED malloc test %d\n", testno);
	}
	else {
		printf("Passed malloc test %d\n", testno);
	}
}

static
void
test5(void)
{
	printf("Beginning malloc test 5\n");
	test567(5, 0);
}

static
void
test6(void)
{
	int fd, len;
	unsigned long seed = _readcounter();

	printf("Beginning malloc test 6\n");

/*	fd = open(_PATH_RANDOM, O_RDONLY);
	if (fd < 0) {
		err(1, "%s", _PATH_RANDOM);
	}
	len = read(fd, &seed, sizeof(seed));
	if (len < 0) {
		err(1, "%s", _PATH_RANDOM);
	}
	else if (len < (int)sizeof(seed)) {
		errx(1, "%s: Short read", _PATH_RANDOM);
	}
	close(fd);
*/
	test567(6, seed);
}

static
void
test7(void)
{
	unsigned long seed;

	printf("Beginning malloc test 7\n");

	printf("Enter random seed: ");
	seed = geti();

	test567(7, seed);
}

////////////////////////////////////////////////////////////

static struct {
	int num;
	const char *desc;
	void (*func)(void);
} tests[] = {
	{ 1, "Simple allocation test", test1 },
	{ 2, "Allocate all memory in a big chunk", test2 },
	{ 3, "Allocate all memory in small chunks", test3 },
	{ 4, "Free list coalescing test (first/next/best-fit only)", test4 },
	{ 5, "Stress test", test5 },
	{ 6, "Randomized stress test", test6 },
	{ 7, "Stress test with particular seed", test7 },
	{ -1, NULL, NULL }
};

static
int
dotest(int tn)
{
	int i;
	for (i=0; tests[i].num>=0; i++) {
		if (tests[i].num == tn) {
			tests[i].func();
			return 0;
		}
	}
	return -1;
}

void thread(void)
{
	int i, tn, menu=1;


	while (1) {
		if (menu) {
			for (i=0; tests[i].num>=0; i++) {
				printf("  %2d  %s\n", tests[i].num, 
				       tests[i].desc);
			}
			menu = 0;
		}
		printf("malloctest: ");
		tn = geti();
		if (tn < 0) {
			break;
		}

		if (dotest(tn)) {
			menu = 1;
		}
	}

	return 0;
}

void thread2(void)
{
	int i, tn, menu=1;


	while (1) {
		if (menu) {
			for (i=0; tests[i].num>=0; i++) {
				printf("  %2d  %s\n", tests[i].num, 
				       tests[i].desc);
			}
			menu = 0;
		}
		printf("malloctest thread %d: ", hf_selfid());
		tn = 6;
		if (tn < 0) {
			break;
		}

		if (dotest(tn)) {
			printf("test failed, thread %d\n", hf_selfid());
			for(;;);
		}
	}

	return 0;
}

void app_main(void){
	int i;
	
	printf("\n1 - iterative test\n");
	printf("2 - multiple threads, stress test\n");
	
	i = geti();
	if (i == 1){	
		hf_spawn(thread, 0, 0, 0, "memtest", 2048);
	}else{
		hf_spawn(thread2, 0, 0, 0, "memtest", 2048);
		hf_spawn(thread2, 0, 0, 0, "memtest", 2048);
		hf_spawn(thread2, 0, 0, 0, "memtest", 2048);
		hf_spawn(thread2, 0, 0, 0, "memtest", 2048);
		hf_spawn(thread2, 0, 0, 0, "memtest", 2048);
	}
}
