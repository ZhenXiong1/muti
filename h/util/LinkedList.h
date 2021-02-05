#ifndef _LIST_LINUX_LINKED_LIST_H
#define _LIST_LINUX_LINKED_LIST_H

#include <stdlib.h>

#ifdef WIN64
#define OFFSET_POINTER unsigned long long int
#else
#define OFFSET_POINTER unsigned long
#endif

#define offsetOf(struct_type, memb) \
	((OFFSET_POINTER)(&((struct_type *)0)->memb))

#define containerOf(obj, struct_type, memb) \
	((struct_type *)(((char *)obj) - offsetOf(struct_type, memb)))

#define NULL_POISON	NULL

typedef struct list_head ListElement;
typedef struct list_head ListHead;

struct list_head {
	ListElement *next, *prev;
};

static inline void listHeadInit(ListHead *head) {
    head->next = head;
    head->prev = head;
}

static inline void __listAdd(ListElement *element, ListElement *prev, ListElement *next) {
	next->prev = element;
	element->next = next;
	element->prev = prev;
	prev->next = element;
}

static inline void listAdd(ListElement *element, ListHead *head) {
	__listAdd(element, head, head->next);
}

static inline void listAddTail(ListElement *element, ListHead *head) {
	__listAdd(element, head->prev, head);
}

static inline void __listDel(ListElement * prev, ListElement * next) {
	next->prev = prev;
	prev->next = next;
}

static inline void __listDelEntry(ListElement *element) {
	__listDel(element->prev, element->next);
}

static inline void listDel(ListElement *element) {
	__listDel(element->prev, element->next);
	element->next = NULL_POISON;
	element->prev = NULL_POISON;
}

static inline void listReplace(ListElement *old_element, ListElement *new_element) {
	new_element->next = old_element->next;
	new_element->next->prev = new_element;
	new_element->prev = old_element->prev;
	new_element->prev->next = new_element;
}

static inline void listReplaceInit(ListElement *old_element, ListElement *new_element) {
	listReplace(old_element, new_element);
	listHeadInit(old_element);
}

static inline void listDelInit(ListElement *element)
{
	__listDelEntry(element);
	listHeadInit(element);
}

static inline void listMove(ListElement *element, ListElement *head) {
	__listDelEntry(element);
	listAdd(element, head);
}

static inline void listMoveTail(ListElement *element, ListElement *head) {
	__listDelEntry(element);
	listAddTail(element, head);
}

static inline int listIsLast(const ListElement *list, const ListElement *head) {
	return list->next == head;
}

static inline int listIsFirst(const ListElement *list, const ListElement *head) {
    return list->prev == head;
}

static inline int listEmpty(const ListElement *head) {
	return head->next == head;
}

static inline void listRotateLeft(ListElement *head) {
	ListElement *first;

	if (!listEmpty(head)) {
		first = head->next;
		listMoveTail(first, head);
	}
}

static inline int listIsSingular(const ListElement *head) {
	return !listEmpty(head) && (head->next == head->prev);
}

static inline void __listCutPosition(ListElement *element,
		ListHead *head, ListElement *entry) {
	ListElement *new_first = entry->next;
	element->next = head->next;
	element->next->prev = element;
	element->prev = entry;
	entry->next = element;
	head->next = new_first;
	new_first->prev = head;
}

static inline void listCutPosition(ListElement *element, ListHead *head, ListElement *entry) {
	if (listEmpty(head))
		return;
	if (listIsSingular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		listHeadInit(element);
	else
		__listCutPosition(element, head, entry);
}

static inline void __listJoin(const ListElement *element, ListElement *prev, ListElement *next) {
	ListElement *first = element->next;
	ListElement *last = element->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

static inline void listJoin(const ListHead *list_from, ListHead *list_to) {
	if (!listEmpty(list_from))
		__listJoin(list_from, list_to, list_to->next);
}

static inline void listJoinTail(ListHead *list_from, ListHead *list_to) {
	if (!listEmpty(list_from))
		__listJoin(list_from, list_to->prev, list_to);
}

static inline void listJoinInit(ListHead *list_from, ListHead *list_to) {
	if (!listEmpty(list_from)) {
		__listJoin(list_from, list_to, list_to->next);
		listHeadInit(list_from);
	}
}

static inline void listJoinTailInit(ListHead *list_from, ListHead *list_to) {
	if (!listEmpty(list_from)) {
		__listJoin(list_from, list_to->prev, list_to);
		listHeadInit(list_from);
	}
}

#define listEntry(element_ptr, struct_type, member) \
        containerOf(element_ptr, struct_type, member)

#define listFirstEntry(head, struct_type, member) \
	listEntry((head)->next, struct_type, member)

#define listFirstEntryOrNull(head, struct_type, member) \
	(!listEmpty(head) ? listFirstEntry(head, struct_type, member) : NULL)

#define listLastEntry(head, struct_type, member) \
	listEntry((head)->prev, struct_type, member)

#define listLastEntryOrNull(head, struct_type, member) \
	(!listEmpty(head) ? listLastEntry(head, struct_type, member) : NULL)

#define listForEach(element_ptr, head) \
	for (element_ptr = (head)->next; element_ptr != (head); element_ptr = element_ptr->next)

#define listForEachPrev(element_ptr, head) \
	for (element_ptr = (head)->prev; element_ptr != (head); element_ptr = pos->prev)

#define listForEachSafe(element_ptr, n, head) \
	for (element_ptr = (head)->next, n = element_ptr->next; element_ptr != (head); \
	    element_ptr = n, n = element_ptr->next)

#define listForEachPrevSafe(element_ptr, n, head) \
	for (element_ptr = (head)->prev, n = element_ptr->prev; \
	    element_ptr != (head); element_ptr = n, n = element_ptr->prev)

#define listForEachEntry(obj, head, member)				\
	for (obj = listEntry((head)->next, typeof(*obj), member);	\
	     &obj->member != (head); 	\
	     obj = listEntry(obj->member.next, typeof(*obj), member))

#define listForEachEntryReverse(obj, head, member)			\
	for (obj = listEntry((head)->prev, typeof(*obj), member);	\
	     &obj->member != (head); 	\
	     obj = listEntry(obj->member.prev, typeof(*obj), member))

#define listPrepareEntry(obj, head, member) \
	((obj) ? : listEntry(head, typeof(*obj), member))

#define listForEachEntryContinue(obj, head, member) 		\
	for (obj = listEntry(obj->member.next, typeof(*obj), member);	\
	     &obj->member != (head);	\
	     obj = listEntry(obj->member.next, typeof(*obj), member))

#define listForEachEntryContinueReverse(obj, head, member)		\
	for (obj = listEntry(obj->member.prev, typeof(*obj), member);	\
	     &obj->member != (head);	\
	     obj = listEntry(obj->member.prev, typeof(*obj), member))

#define listForEachEntryFrom(obj, head, member) 			\
	for (; &obj->member != (head);	\
	     obj = listEntry(obj->member.next, typeof(*obj), member))

#define listForEachEntrySafe(obj, n, head, member)			\
	for (obj = listEntry((head)->next, typeof(*obj), member),	\
		n = listEntry(obj->member.next, typeof(*obj), member);	\
	     &obj->member != (head); 					\
	     obj = n, n = listEntry(n->member.next, typeof(*n), member))

#define listForEachEntrySafeContinue(obj, n, head, member) 		\
	for (obj = listEntry(obj->member.next, typeof(*obj), member), 		\
		n = listEntry(obj->member.next, typeof(*obj), member);		\
	     &obj->member != (head);						\
	     obj = n, n = listEntry(n->member.next, typeof(*n), member))

#define listForEachEntrySafeFrom(obj, n, head, member) 			\
	for (n = listEntry(obj->member.next, typeof(*obj), member);		\
	     &obj->member != (head);						\
	     obj = n, n = listEntry(n->member.next, typeof(*n), member))

#define listForEachEntrySafeReverse(obj, n, head, member)		\
	for (obj = listEntry((head)->prev, typeof(*obj), member),	\
		n = listEntry(obj->member.prev, typeof(*obj), member);	\
	     &obj->member != (head); 					\
	     obj = n, n = listEntry(n->member.prev, typeof(*n), member))

#define listSafeResetNext(obj, n, member)				\
	n = listEntry(obj->member.next, typeof(*obj), member)

#endif
