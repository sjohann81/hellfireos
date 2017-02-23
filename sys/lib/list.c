/**
 * @file list.c
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
 * List manipulation primitives and auxiliary functions. List structures are allocated
 * dynamically at runtime, which makes them very flexible. Memory is allocated / deallocated
 * on demand, so additional memory management penalties are incurred.
 */

#include <hal.h>
#include <libc.h>
#include <malloc.h>
#include <list.h>

/**
 * @brief Initializes a list.
 * 
 * @return a pointer to a list structure.
 */
struct list *hf_list_init(void)
{
	struct list *lst;
	
	lst = (struct list *)hf_malloc(sizeof(struct list));
	
	if (lst){
		lst->next = NULL;
		lst->elem = NULL;
	}

	return lst;
}

/**
 * @brief Appends a new node to the end of the list.
 * 
 * @param lst is a pointer to a list structure.
 * @param item is a pointer to data belonging to the list node.
 * 
 * @return 0 when successful and -1 otherwise.
 */
int32_t hf_list_append(struct list *lst, void *item)
{
	struct list *t1, *t2;

	t1 = (struct list *)hf_malloc(sizeof(struct list));
	if (t1){
		t1->elem = item;
		t1->next = NULL;
		t2 = lst;

		while (t2->next)
			t2 = t2->next;

		t2->next = t1;

		return 0;
	}else{
		return -1;
	}
}

/**
 * @brief Inserts a new node to an arbitrary position in a list.
 * 
 * @param lst is a pointer to a list structure.
 * @param item is a pointer to data belonging to the list node.
 * @param pos is the n-th element position in the list.
 * 
 * @return 0 when successful and -1 otherwise.
 */
int32_t hf_list_insert(struct list *lst, void *item, int32_t pos)
{
	struct list *t1, *t2;
	int32_t i = 0;

	t1 = (struct list *)hf_malloc(sizeof(struct list));
	if (t1){
		t1->elem = item;
		t1->next = NULL;
		t2 = lst;

		while (t2->next){
			t2 = t2->next;
			if (++i == pos) break;
		}

		t1->next = t2->next;
		t2->next = t1;

		return 0;
	}else{
		return -1;
	}
}

/**
 * @brief Removes an arbitrary node from a list.
 * 
 * @param lst is a pointer to a list structure.
 * @param pos is the n-th element position in the list.
 * 
 * @return 0 when successful and -1 otherwise.
 */
int32_t hf_list_remove(struct list *lst, int32_t pos)
{
	struct list *t1, *t2;
	int32_t i = 0;

	t1 = lst;
	t2 = t1;
	while ((t1 = t1->next)){
		if (i++ == pos){
			t2->next = t1->next;
			hf_free(t1);
			return 0;
		}
		t2 = t1;
	}
	
	return -1;
}

/**
 * @brief Returns the address of the data belonging to a list node.
 * 
 * @param lst is a pointer to a list structure.
 * @param pos is the n-th element position in the list.
 * 
 * @return 0 when the element is not found and the address to data otherwise.
 */
void *hf_list_get(struct list *lst, int32_t pos)
{
	struct list *t1;
	int32_t i = 0;

	t1 = lst;
	while ((t1 = t1->next)){
		if (i++ == pos)
			return (void *)t1->elem;
	}
	
	return 0;
}

/**
 * @brief Changes the address of the data belonging to a list node.
 * 
 * @param lst is a pointer to a list structure.
 * @param item is an address to data belonging to the list node.
 * @param pos is the n-th element position in the list.
 * 
 * @return -1 when the element is not found and 0 if the element was updated.
 */
int32_t hf_list_set(struct list *lst, void *item, int32_t pos)
{
	struct list *t1;
	int32_t i = 0;

	t1 = lst;
	while ((t1 = t1->next)){
		if (i++ == pos){
			t1->elem = item;
			return 0;
		}
	}
	
	return -1;
}

/**
 * @brief Returns the number of nodes in a list.
 * 
 * @param lst is a pointer to a list structure.
 * 
 * @return The number of elements in the list.
 */
int32_t hf_list_count(struct list *lst)
{
	struct list *t1;
	int32_t i = 0;

	t1 = lst;
	while ((t1 = t1->next))
		i++;

	return i;
}
