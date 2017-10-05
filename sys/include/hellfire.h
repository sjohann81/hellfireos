/**
 * @file hellfire.h
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
 * Default system wide include file and error code definitions.
 * 
 */
/*
include default stuff
*/
#include <hal.h>
#include <interrupt.h>
#include <libc.h>
#include <crc.h>
#if FLOATING_POINT == 1
#include <math.h>
#endif
#include <kprintf.h>
#include <malloc.h>
#include <queue.h>
#include <list.h>
#include <semaphore.h>
#include <mutex.h>
#include <condvar.h>
#include <kernel.h>
#include <panic.h>
#include <scheduler.h>
#include <task.h>
#include <processor.h>
#include <main.h>
#include <ecodes.h>
