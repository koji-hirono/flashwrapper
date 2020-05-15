#include <inttypes.h>
#include <stdlib.h>

#include "npapi.h"

typedef struct Entry Entry;

struct Entry {
	NPStream stream;
	Entry *next;
	Entry *prev;
	uintptr_t ident;
};

static Entry *head = NULL;

NPStream *
npstream_create(uintptr_t ident)
{
	Entry *e;

	e = malloc(sizeof(*e));
	if (e == NULL)
		return NULL;

	e->stream.ndata = (void *)ident;
	e->stream.pdata = NULL;
	e->next = head;
	e->prev = NULL;
	e->ident = ident;
	head = e;

	return &e->stream;
}

void
npstream_destroy(NPStream *stream)
{
	Entry *e = (Entry *)stream;

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

NPStream *
npstream_find(uintptr_t ident)
{
	Entry *e;

	for (e = head; e != NULL; e = e->next)
		if (e->ident == ident)
			return &e->stream;

	return NULL;
}

uintptr_t
npstream_ident(NPStream *stream)
{
	Entry *e = (Entry *)stream;

	if (!e)
		return 0;

	return e->ident;
}
