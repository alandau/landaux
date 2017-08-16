#ifndef LIST_H
#define LIST_H

typedef struct list_struct
{
	struct list_struct *prev, *next;
} list_t;

static inline void list_init(list_t *list)
{
	list->next = list->prev = list;
}

#define LIST_INIT(list) {&list, &list}

static inline void list_add(list_t *where, list_t *what)
{
	what->prev = where;
	what->next = where->next;
	where->next->prev = what;
	where->next = what;
}

#define list_add_after list_add
#define list_add_tail list_add_before

static inline void list_add_before(list_t *where, list_t *what)
{
	list_add_after(where->prev, what);
}

static inline void list_del(list_t *what)
{
	what->prev->next = what->next;
	what->next->prev = what->prev;
}

static inline int list_empty(list_t *list)
{
	return (list->next == list);
}

#define list_get(list, struct_type, member) ((struct_type *)((unsigned long)(list) - ((unsigned long)&((struct_type *)0)->member)))

#define list_for_each(list, p) for (p = (list)->next; p != (list); p = p->next)

#endif
