#include <sys/types.h>
#include <sys/resource.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>

#include "npapi.h"
#include "npfunctions.h"

#include "logger.h"
#include "plugin.h"


int
plugin_open(Plugin *p, const char *path)
{
	struct rlimit rlim;

	rlim.rlim_cur = 0;
	rlim.rlim_max = 0;
	if (setrlimit(RLIMIT_CORE, &rlim) != 0) {
		logger_debug("setrlimit %s", strerror(errno));
		return -1;
	}

	p->h = dlopen(path, RTLD_LAZY);
	if (p->h == NULL) {
		logger_debug("dlopen %s", dlerror());
		return -1;
	}
	p->np_init = dlsym(p->h, "NP_Initialize");
	p->np_shutdown = dlsym(p->h, "NP_Shutdown");
	p->np_get_value = dlsym(p->h, "NP_GetValue");
	p->np_get_mimedesc = dlsym(p->h, "NP_GetMIMEDescription");
	p->np_get_pluginversion = dlsym(p->h, "NP_GetPluginVersion");

	p->mimedesc = (p->np_get_mimedesc) ? p->np_get_mimedesc() : NULL;
	p->version = (p->np_get_pluginversion) ? p->np_get_pluginversion() : NULL;

	memset(&p->funcs, 0, sizeof(p->funcs));
	p->funcs.version = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
	p->funcs.size = sizeof(p->funcs);

	return 0;
}

void
plugin_close(Plugin *p)
{
	dlclose(p->h);
}
