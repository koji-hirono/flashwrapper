#ifndef NPOBJECT_H__
#define NPOBJECT_H__

#include <inttypes.h>

#include "npapi.h"
#include "npruntime.h"

extern NPObject *npobject_create(uintptr_t);
extern void npboject_destroy(NPObject *);
extern NPObject *npobject_find(uintptr_t);
extern uintptr_t npobject_ident(NPObject *);

#endif
