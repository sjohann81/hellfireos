/**
 * @file mailbox.c
 * @author Sergio Johann Filho
 * @date January 2016
 * 
 * @section LICENSE
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file 'doc/license/gpl-2.0.txt' for more details.
 * 
 * @section DESCRIPTION
 * 
 * Mailbox IPC primitives.
 */

#include <hellfire.h>

/**
 * @brief Initializes a mailbox.
 * 
 * @param mbox is a pointer to a mailbox structure.
 * @param n_waiting_tasks is the number of tasks that will wait for messages on the mailbox.
 */
void hf_mboxinit(mail_t *mbox, uint16_t n_waiting_tasks)
{
	uint32_t status;

	status = _di();
	mbox->msg = NULL;
	mbox->n_waiting_tasks = n_waiting_tasks;
	mbox->count = n_waiting_tasks;
	hf_seminit(&mbox->msend, 1);
	hf_seminit(&mbox->mrecv, 0);
	_ei(status);
}

/**
 * @brief Posts a message on a mailbox.
 * 
 * @param mbox is a pointer to a mailbox structure.
 * @param msg is a pointer to a message buffer.
 * 
 * The number of messages sent depends on the number of waiting tasks in
 * the mailbox.
 */
void hf_mboxsend(mail_t *mbox, void *msg)
{
	uint32_t i;

	for(i = 0; i < mbox->n_waiting_tasks; i++){
		hf_semwait(&mbox->msend);
		mbox->msg = msg;
		hf_sempost(&mbox->mrecv);
	}
}

/**
 * @brief Gets a message from a mailbox.
 * 
 * @param mbox is a pointer to a mailbox structure.
 * 
 * @return pointer to a message buffer.
 * 
 * A message is received and the counter controlling the number of messages
 * left in this mailbox is decremented. If the count reaches zero, it is
 * reinitialized.
 */
void *hf_mboxrecv(mail_t *mbox)
{
	void *msg;

	hf_semwait(&mbox->mrecv);
	msg = mbox->msg;
	mbox->count--;
	if (mbox->count == 0){
		mbox->msg = NULL;
		mbox->count = mbox->n_waiting_tasks;
	}
	hf_sempost(&mbox->msend);

	return msg;
}

/**
 * @brief Tries to gets a message from a mailbox.
 * 
 * @param mbox is a pointer to a mailbox structure.
 * 
 * @return pointer to a message buffer.
 * 
 * This is the non-blocking version of hf_mboxrecv();
 */
void *hf_mboxaccept(mail_t *mbox)
{
	uint32_t status;
	void *msg;

	status = _di();
	msg = mbox->msg;
	if (msg != NULL){
		mbox->count--;
		if (mbox->count == 0){
			mbox->msg = NULL;
			mbox->count = mbox->n_waiting_tasks;
		}
		hf_sempost(&mbox->msend);
	}
	_ei(status);

	return msg;
}
