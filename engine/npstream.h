#ifndef NPSTREAM_H__
#define NPSTREAM_H__

#include <inttypes.h>

#include "npapi.h"


extern NPStream *npstream_create(uintptr_t);
extern void npstream_destroy(NPStream *);
extern NPStream *npstream_find(uintptr_t);
extern uintptr_t npstream_ident(NPStream *);

#endif
