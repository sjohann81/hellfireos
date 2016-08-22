/**
 * @file mutex.c
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
 * Mutex synchronization primitives.
 * 
 */

#include <hal.h>
#include <libc.h>
#include <mutex.h>
#include <ecodes.h>

#if MUTEX_TYPE == 0
/* type 0: spinlock
 */
static int32_t tsl(mutex_t *m)
{
	volatile int32_t status, init;
	
	status = _di();
	init = m->lock;
	m->lock = 1;
	_ei(status);

	return init;
}

/**
 * @brief Initializes a mutex, defining its initial value.
 * 
 * @param s is a pointer to a mutex.
 */
void hf_mtxinit(mutex_t *m)
{
	m->lock = 0;
}

/**
 * @brief Locks a mutex.
 * 
 * @param s is a pointer to a mutex.
 * 
 * If the mutex is not locked, the calling task continues execution. Otherwise,
 * the task spins.
 */
void hf_mtxlock(mutex_t *m)
{
	while (tsl(m) == 1);
}

/**
 * @brief Unlocks a mutex.
 * 
 * @param s is a pointer to a mutex.
 */
void hf_mtxunlock(mutex_t *m)
{
	m->lock = 0;
}
#endif

#if MUTEX_TYPE == 1
/* type 1: Peterson's algorithm (software only!)
 */
void hf_mtxinit(mutex_t *m)
{
	int32_t i;
	
	for (i = 0; i < MAX_TASKS; i++)
		m->level[i] = 0;
	for (i = 0; i < MAX_TASKS-1; i++)
		m->waiting[i] = 0;
}

void hf_mtxlock(mutex_t *m)
{
	int32_t i, k, l;

	i = hf_selfid();
	for (l = 1; l < MAX_TASKS; ++l){
		m->level[i] = l;
		m->waiting[l] = i;
		for (k = 0; k < MAX_TASKS; k++)
			while (k != i && m->level[k] >= l && m->waiting[l] == i);
	}
}

void hf_mtxunlock(mutex_t *m)
{
	int32_t i;

	i = hf_selfid();
	m->level[i] = 0;
}
#endif

