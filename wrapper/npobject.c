#include <inttypes.h>
#include <stdlib.h>

#include "npapi.h"
#include "npruntime.h"

#include "logger.h"
#include "npobject.h"

typedef struct Entry Entry;

struct Entry {
	NPObject obj;
	uintptr_t ident;
	Entry *next;
	Entry *prev;
};

static NPObject *
np_allocate(NPP npp, NPClass *aclass)
{
	logger_debug("start");
	logger_debug("npp :%p", npp);
	logger_debug("aclass :%p", aclass);

	return malloc(sizeof(Entry));
}

static void
np_deallocate(NPObject *npobj)
{
	logger_debug("start(%p)", npobj);
}

static void
np_invalidate(NPObject *npobj)
{
	logger_debug("start(%p)", npobj);
}

static bool
np_hasMethod(NPObject *npobj, NPIdentifier name)
{
	logger_debug("start(%p)", npobj);
	logger_debug("name: %p", name);
	return false;
}

static bool
np_invoke(NPObject *npobj, NPIdentifier name, const NPVariant *args,
		uint32_t argCount, NPVariant *result)
{
	uint32_t i;

	logger_debug("start(%p)", npobj);
	logger_debug("name: %p", name);
	logger_debug("argCount: %"PRIu32, argCount);
	for (i = 0; i < argCount; i++) {
		logger_debug("args[%"PRIu32"] {", i);
		logger_debug("  type: %u", args[i].type);
		logger_debug("}");
	}
	logger_debug("result: %p", result);
	return false;
}

static bool
np_invokeDefault(NPObject *npobj, const NPVariant *args, uint32_t argCount,
		NPVariant *result)
{
	uint32_t i;

	logger_debug("start(%p)", npobj);
	logger_debug("argCount: %"PRIu32, argCount);
	for (i = 0; i < argCount; i++) {
		logger_debug("args[%"PRIu32"] {", i);
		logger_debug("  type: %u", args[i].type);
		logger_debug("}");
	}
	logger_debug("result: %p", result);
	return false;
}

static bool
np_hasProperty(NPObject *npobj, NPIdentifier name)
{
	logger_debug("start(%p)", npobj);
	logger_debug("name: %p", name);
	return false;
}

static bool
np_getProperty(NPObject *npobj, NPIdentifier name, NPVariant *result)
{
	logger_debug("start(%p)", npobj);
	logger_debug("name: %p", name);
	logger_debug("result: %p", result);
	return false;
}

static bool
np_setProperty(NPObject *npobj, NPIdentifier name, const NPVariant *value)
{
	logger_debug("start(%p)", npobj);
	logger_debug("name: %p", name);
	logger_debug("value: %p", value);
	return false;
}

static bool
np_removeProperty(NPObject *npobj, NPIdentifier name)
{
	logger_debug("start(%p)", npobj);
	logger_debug("name: %p", name);
	return false;
}

static bool
np_enumerate(NPObject *npobj, NPIdentifier **value, uint32_t *count)
{
	logger_debug("start(%p)", npobj);
	logger_debug("value: %p", value);
	logger_debug("count: %p", count);
	return false;
}

static bool
np_construct(NPObject *npobj, const NPVariant *args, uint32_t argCount,
		NPVariant *result)
{
	uint32_t i;

	logger_debug("start(%p)", npobj);
	logger_debug("argCount: %"PRIu32, argCount);
	for (i = 0; i < argCount; i++) {
		logger_debug("args[%"PRIu32"]", i);
		logger_debug("  type: %u", args[i].type);
		logger_debug("}");
	}
	logger_debug("result: %p", result);
	return false;
}


static NPClass obj_class = {
	.structVersion = NP_CLASS_STRUCT_VERSION,
	.allocate = np_allocate,
	.deallocate = np_deallocate,
	.invalidate = np_invalidate,
	.hasMethod = np_hasMethod,
	.invoke = np_invoke,
	.invokeDefault = np_invokeDefault,
	.hasProperty = np_hasProperty,
	.getProperty = np_getProperty,
	.setProperty = np_setProperty,
	.removeProperty = np_removeProperty,
	.enumerate = np_enumerate,
	.construct = np_construct
};

static Entry *head = NULL;

NPObject *
npobject_create(uintptr_t ident)
{
	Entry *e;

	e = malloc(sizeof(*e));
	if (e == NULL)
		return NULL;

	e->obj._class = &obj_class;
	e->obj.referenceCount = 1;
	e->ident = ident;
	e->next = head;
	e->prev = NULL;
	head = e;

	return &e->obj;
}

void
npboject_destroy(NPObject *obj)
{
	Entry *e = (Entry *)obj;

	if (e->prev)
		e->prev->next = e->next;
	else
		head = e->next;
	if (e->next)
		e->next->prev = e->prev;

	free(e);
}

NPObject *
npobject_find(uintptr_t ident)
{
	Entry *e;

	for (e = head; e != NULL; e = e->next)
		if (e->ident == ident)
			return &e->obj;

	return NULL;
}

uintptr_t
npobject_ident(NPObject *obj)
{
	Entry *e = (Entry *)obj;

	return e->ident;
}
