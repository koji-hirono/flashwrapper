#ifndef BROWSER_H__
#define BROWSER_H__

#include "npapi.h"
#include "npfunctions.h"

#include "rpc.h"

typedef struct Browser Browser;

struct Browser {
	RPCSess *sess;
	void *window;
	NPNetscapeFuncs funcs;
};

extern int browser_init(Browser *, RPCSess *);

#endif
