#ifndef PLUGIN_H__
#define PLUGIN_H__

#include "npapi.h"
#include "npfunctions.h"

typedef struct Plugin Plugin;

struct Plugin {
	const char *path;
	const char *version;
	const char *mimedesc;
	void *h;
	NP_InitializeFunc np_init;
	NP_ShutdownFunc np_shutdown;
	NP_GetValueFunc np_get_value;
	NP_GetMIMEDescriptionFunc np_get_mimedesc;
	NP_GetPluginVersionFunc np_get_pluginversion;
	NPPluginFuncs funcs;
};


extern int plugin_open(Plugin *, const char *);
extern void plugin_close(Plugin *);

#endif
