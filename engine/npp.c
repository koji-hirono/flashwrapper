#include <stdlib.h>

#include "npapi.h"

typedef struct Entry Entry;

struct Entry {
	NPP_t np;
	Entry *next;
	Entry *prev;
	uintptr_t ident;
};

static Entry *head = NULL;

NPP
npp_create(uintptr_t ident)
{
	Entry *e;

	e = malloc(sizeof(*e));
	if (e == NULL)
		return NULL;

	e->np.ndata = NULL;
	e->np.pdata = NULL;
	e->next = head;
	e->prev = NULL;
	e->ident = ident;
	head = e;

	return &e->np;
}

void
npp_destroy(NPP npp)
{
	Entry *e = (Entry *)npp;

	if (!e)
		return;

	if (e->prev)
		e->prev->next = e->next;
	else
		head = e->next;

	if (e->next)
		e->next->prev = e->prev;

	free(e);
}

NPP
npp_find(uintptr_t ident)
{
	Entry *e;

	for (e = head; e != NULL; e = e->next)
		if (e->ident == ident)
			return &e->np;

	return NULL;
}

uintptr_t
npp_ident(NPP npp)
{
	Entry *e = (Entry *)npp;

	if (!e)
		return 0;

	return e->ident;
}
