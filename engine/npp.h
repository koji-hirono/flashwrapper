#ifndef NPP_H__
#define NPP_H__

#include "npapi.h"


extern NPP npp_create(uintptr_t);
extern void npp_destroy(NPP);
extern NPP npp_find(uintptr_t);
extern uintptr_t npp_ident(NPP);

#endif
