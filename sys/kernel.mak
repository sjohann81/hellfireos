KERNEL_VER = v2.17.03

kernel:
	$(CC) $(CFLAGS) \
		$(SRC_DIR)/sys/lib/kprintf.c \
		$(SRC_DIR)/sys/lib/malloc.c \
		$(SRC_DIR)/sys/kernel/panic.c \
		$(SRC_DIR)/sys/sync/mutex.c \
		$(SRC_DIR)/sys/sync/semaphore.c \
		$(SRC_DIR)/sys/sync/condvar.c \
		$(SRC_DIR)/sys/lib/queue.c \
		$(SRC_DIR)/sys/lib/list.c \
		$(SRC_DIR)/sys/kernel/task.c \
		$(SRC_DIR)/sys/kernel/scheduler.c \
		$(SRC_DIR)/sys/kernel/processor.c \
		$(SRC_DIR)/sys/kernel/main.c
