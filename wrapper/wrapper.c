#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>

#include "npapi.h"
#include "npfunctions.h"

#include "logger.h"
#include "util.h"
#include "buf.h"
#include "rpc.h"
#include "engine.h"


typedef struct Wrapper Wrapper;
typedef struct Plugin Plugin;

struct Wrapper {
	NPNetscapeFuncs *bfuncs;
	RPCSess sess;
	Engine engine;
	int inited;
};

struct Plugin {
	void *a;
};


static Wrapper g_wrapper;


static void
NPN_UserAgent_handle(Wrapper *wrapper, RPCMsg *msg)
{
	RPCSess *sess = &wrapper->sess;
	NPP instance;
	BufReader r;
	BufWriter w;
	uint32_t len;
	const char *s;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);
	buf_uint64_decode(&r, &instance);

	logger_debug("instance: %p");

	s = wrapper->bfuncs->uagent(instance);

	logger_debug("uagent: %s", s);

	buf_writer_open(&w, 0);
	if (s) {
		len = strlen(s) + 1;
		buf_uint32_encode(&w, len);
		buf_bytes_encode(&w, s, len);
	} else {
		buf_uint32_encode(&w, 0);
	}

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NPN_GetValue_handle(Wrapper *wrapper, RPCMsg *msg)
{
	RPCSess *sess = &wrapper->sess;
	NPP instance;
	BufWriter w;
	BufReader r;
	NPNVariable variable;
	NPError error;
	uint32_t len;
	NPNToolkitType toolkit;
	NPBool boolval;
	NPObject *npobj;
	char *s;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);

	buf_uint64_decode(&r, &instance);
	logger_debug("instance: %p");

	if (buf_uint32_decode(&r, &variable) == NULL) {
		logger_debug("buf_uint32_decode failed.");
		return;
	}

	logger_debug("variable: [%u]%s", variable, npn_varstr(variable));

	buf_writer_open(&w, 0);

	switch (variable) {
	case NPNVSupportsXEmbedBool:
	case NPNVSupportsWindowless:
	case NPNVprivateModeBool:
	case NPNVmuteAudioBool:
		error = wrapper->bfuncs->getvalue(instance, variable,
				(void *)&boolval);
		logger_debug("value: %u", boolval);
		buf_uint16_encode(&w, error);
		buf_uint8_encode(&w, boolval);
		break;
	case NPNVToolkit:
		error = wrapper->bfuncs->getvalue(instance, variable,
				(void *)&toolkit);
		logger_debug("value: %u", toolkit);
		buf_uint16_encode(&w, error);
		buf_uint32_encode(&w, toolkit);
		break;
	case NPNVdocumentOrigin:
		error = wrapper->bfuncs->getvalue(instance, variable,
				(void *)&s);
		logger_debug("value: '%s'", s);
		buf_uint16_encode(&w, error);
		if (s) {
			len = strlen(s) + 1;
			buf_uint32_encode(&w, len);
			buf_bytes_encode(&w, s, len);
		} else {
			buf_uint32_encode(&w, 0);
		}
		break;
	case NPNVWindowNPObject:
		error = wrapper->bfuncs->getvalue(instance, variable,
				(void *)&npobj);
		logger_debug("value: %p", npobj);
		buf_uint16_encode(&w, error);
		buf_uint64_encode(&w, (uintptr_t)npobj);
		break;
	default:
		logger_debug("unknown variable: %d", variable);
		error = NPERR_GENERIC_ERROR;
		buf_uint16_encode(&w, error);
		break;
	}

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NPN_SetValue_handle(Wrapper *wrapper, RPCMsg *msg)
{
	RPCSess *sess = &wrapper->sess;
	NPP instance;
	BufWriter w;
	BufReader r;
	NPPVariable variable;
	NPError error;
	NPBool boolval;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);
	buf_uint64_decode(&r, &instance);

	logger_debug("instance: %p", instance);

	if (buf_uint32_decode(&r, &variable) == NULL) {
		logger_debug("buf_uint32_decode failed.");
		return;
	}

	logger_debug("variable: [%u]%s", variable, npp_varstr(variable));

	buf_writer_open(&w, 0);

	switch (variable) {
	case NPPVpluginWindowBool:
	case NPPVpluginTransparentBool:
		if (buf_uint8_decode(&r, &boolval) == NULL) {
			logger_debug("buf_uint8_decode failed.");
			return;
		}
		logger_debug("value: %u", boolval);
		error = wrapper->bfuncs->setvalue(instance, variable,
				boolval ? &boolval : NULL);
		logger_debug("error: %s(%d)", np_errorstr(error), error);
		buf_uint16_encode(&w, error);
		break;
	default:
		logger_debug("unknown variable: %d", variable);
		error = NPERR_GENERIC_ERROR;
		buf_uint16_encode(&w, error);
		break;
	}

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NPN_ReleaseObject_handle(Wrapper *wrapper, RPCMsg *msg)
{
	RPCSess *sess = &wrapper->sess;
	BufReader r;
	NPObject *npobj;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);
	if (buf_uint64_decode(&r, &npobj) == NULL) {
		logger_debug("buf_uint64_decode failed.");
		return;
	}

	logger_debug("npobj: %p", npobj);

	wrapper->bfuncs->releaseobject(npobj);

	rpc_return(sess, msg);

	logger_debug("end");
}


static void
npn_dispatch(RPCSess *sess, RPCMsg *msg, void *ctxt)
{
	Wrapper *wrapper = ctxt;

	switch (msg->method) {
	case RPC_NPN_UserAgent:
		NPN_UserAgent_handle(wrapper, msg);
		break;
	case RPC_NPN_GetValue:
		NPN_GetValue_handle(wrapper, msg);
		break;
	case RPC_NPN_SetValue:
		NPN_SetValue_handle(wrapper, msg);
		break;
	case RPC_NPN_ReleaseObject:
		NPN_ReleaseObject_handle(wrapper, msg);
		break;
	default:
		break;
	}
}


static int
wrapper_open(Wrapper *wrapper)
{
	const char *engine_path = "/home/paleman/proj/myflashwrapper/np_engine";
	const char *plugin_path = "/usr/local/lib/browser_plugins/"
		"linux-flashplayer/libflashplayer.so";

	if (engine_open(&wrapper->engine, engine_path, plugin_path) != 0) {
		logger_debug("engine_open failed.");
		return -1;
	}

	rpcsess_init(&wrapper->sess, wrapper->engine.fd, npn_dispatch, wrapper);

	wrapper->inited = 1;

	return 0;
}

static void
wrapper_close(Wrapper *wrapper)
{
	engine_close(&wrapper->engine);
	wrapper->inited = 0;
}

static Wrapper *
wrapper_get(void)
{
	if (!g_wrapper.inited)
		if (wrapper_open(&g_wrapper) != 0)
			return NULL;

	return &g_wrapper;
}


static void
print_variables(NPNetscapeFuncs *bfuncs, NPP npp)
{
	NPError error;
	void *p;
	NPBool b;
	NPNToolkitType toolkit;
	NPObject *obj;
	char *str;
	double d;

	error = bfuncs->getvalue(npp, NPNVxDisplay, &p);
	if (error) {
		logger_debug("NPNVxDisplay: error %d", error);
	} else {
		logger_debug("NPNVxDisplay: %p", p);
	}

	/* NPNVxtAppContext */
	/* NPNVnetscapeWindow */

	error = bfuncs->getvalue(npp, NPNVjavascriptEnabledBool, &b);
	if (error) {
		logger_debug("NPNVjavascriptEnabledBool: error %d", error);
	} else {
		logger_debug("NPNVjavascriptEnabledBool: %d", b);
	}

	error = bfuncs->getvalue(npp, NPNVasdEnabledBool, &b);
	if (error) {
		logger_debug("NPNVasdEnabledBool: error %d", error);
	} else {
		logger_debug("NPNVasdEnabledBool: %d", b);
	}

	error = bfuncs->getvalue(npp, NPNVisOfflineBool, &b);
	if (error) {
		logger_debug("NPNVisOfflineBool: error %d", error);
	} else {
		logger_debug("NPNVisOfflineBool: %d", b);
	}

	error = bfuncs->getvalue(npp, NPNVToolkit, &toolkit);
	if (error) {
		logger_debug("NPNVToolkit: error %d", error);
	} else {
		logger_debug("NPNVToolkit: %d", toolkit);
	}

	error = bfuncs->getvalue(npp, NPNVSupportsXEmbedBool, &b);
	if (error) {
		logger_debug("NPNVSupportsXEmbedBool: error %d", error);
	} else {
		logger_debug("NPNVSupportsXEmbedBool: %d", b);
	}

	error = bfuncs->getvalue(npp, NPNVWindowNPObject, &obj);
	if (error) {
		logger_debug("NPNVWindowNPObject: error %d", error);
	} else {
		logger_debug("NPNVWindowNPObject: %p", obj);
	}

	error = bfuncs->getvalue(npp, NPNVPluginElementNPObject, &obj);
	if (error) {
		logger_debug("NPNVPluginElementNPObject: error %d", error);
	} else {
		logger_debug("NPNVPluginElementNPObject: %p", obj);
	}

	error = bfuncs->getvalue(npp, NPNVSupportsWindowless, &b);
	if (error) {
		logger_debug("NPNVSupportsWindowless: error %d", error);
	} else {
		logger_debug("NPNVSupportsWindowless: %d", b);
	}

	error = bfuncs->getvalue(npp, NPNVprivateModeBool, &b);
	if (error) {
		logger_debug("NPNVprivateModeBool: error %d", error);
	} else {
		logger_debug("NPNVprivateModeBool: %d", b);
	}

	error = bfuncs->getvalue(npp, NPNVdocumentOrigin, &str);
	if (error) {
		logger_debug("NPNVdocumentOrigin: error %d", error);
	} else {
		logger_debug("NPNVdocumentOrigin: %s", str);
	}

	/* NPNVpluginDrawingModel */
	/* NPNVsupportsQuickDrawBool */
	/* NPNVsupportsCoreGraphicsBool */
	/* NPNVsupportsCoreAnimationBool */
	/* NPNVsupportsInvalidatingCoreAnimationBool */
	/* NPNVsupportsCompositingCoreAnimationPluginsBool */
	/* NPNVsupportsCarbonBool */
	/* NPNVsupportsCocoaBool */
	/* NPNVsupportsUpdatedCocoaTextInputBool */

	/* NPNVcontentsScaleFactor */

	error = bfuncs->getvalue(npp, NPNVCSSZoomFactor, &d);
	if (error) {
		logger_debug("NPNVCSSZoomFactor: error %d", error);
	} else {
		logger_debug("NPNVCSSZoomFactor: %f", d);
	}

	error = bfuncs->getvalue(npp, NPNVDOMElement, &p);
	if (error) {
		logger_debug("NPNVDOMElement: error %d", error);
	} else {
		logger_debug("NPNVDOMElement: %p", p);
	}

	error = bfuncs->getvalue(npp, NPNVDOMWindow, &p);
	if (error) {
		logger_debug("NPNVDOMWindow: error %d", error);
	} else {
		logger_debug("NPNVDOMWindow: %p", p);
	}

	error = bfuncs->getvalue(npp, NPNVserviceManager, &p);
	if (error) {
		logger_debug("NPNVserviceManager: error %d", error);
	} else {
		logger_debug("NPNVserviceManager: %p", p);
	}
}


static NPError
pNPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode,
		int16_t argc, char *argn[], char *argv[], NPSavedData *saved)
{
	Wrapper *wrapper;
	RPCMsg msg = {
		.method = RPC_NPP_New,
	};
	BufWriter w;
	BufReader r;
	NPError error;
	uint32_t len;
	int16_t i;

	logger_debug("start");

	wrapper = wrapper_get();
	if (wrapper == NULL)
		return NPERR_GENERIC_ERROR;

	buf_writer_open(&w, 0);

	logger_debug("pluginType: '%s'", pluginType);
	if (pluginType) {
		len = strlen(pluginType) + 1;
		buf_uint32_encode(&w, len);
		buf_bytes_encode(&w, pluginType, len);
	} else {
		buf_uint32_encode(&w, 0);
	}
	logger_debug("instance: {%p, %p}", instance->pdata, instance->ndata);
	buf_uint64_encode(&w, (uintptr_t)instance);
	logger_debug("mode: %u", mode);
	buf_uint16_encode(&w, mode);
	logger_debug("argc: %u", argc);
	buf_uint16_encode(&w, argc);

	for (i = 0; i < argc; i++) {
		logger_debug("argn[%u]: '%s'", i, argn[i]);
		if (argn[i]) {
			len = strlen(argn[i]) + 1;
			buf_uint32_encode(&w, len);
			buf_bytes_encode(&w, argn[i], len);
		} else {
			buf_uint32_encode(&w, 0);
		}
	}
	for (i = 0; i < argc; i++) {
		logger_debug("argv[%u]: '%s'", i, argv[i]);
		if (argv[i]) {
			len = strlen(argv[i]) + 1;
			buf_uint32_encode(&w, len);
			buf_bytes_encode(&w, argv[i], len);
		} else {
			buf_uint32_encode(&w, 0);
		}
	}

	logger_debug("saved: %p", saved);
	if (saved) {
		logger_debug("saved len: %"PRIu32, saved->len);
		buf_uint32_encode(&w, saved->len);
		logger_dump("saved", saved->buf, saved->len);
		buf_bytes_encode(&w, saved->buf, saved->len);
	}

	msg.param = w.data;
	msg.param_size = w.len;

	rpc_invoke(&wrapper->sess, &msg);

	buf_writer_close(&w);

	buf_reader_init(&r, msg.ret, msg.ret_size);
	if (buf_uint16_decode(&r, &error) == NULL) {
		logger_debug("buf_uint16_decode failed.");
		return NPERR_GENERIC_ERROR;
	}

	if (error != NPERR_NO_ERROR)
		return error;

	instance->pdata = malloc(sizeof(Plugin));

#if 0
	print_variables(wrapper->bfuncs, instance);

	NPError e;
	NPBool b;
	b = true;
	e = wrapper->bfuncs->setvalue(instance, NPPVpluginWindowBool, &b);
	logger_debug("setvalue: %d", e);

	b = true;
	e = wrapper->bfuncs->setvalue(instance, NPPVpluginTransparentBool, &b);
	logger_debug("setvalue: %d", e);
#endif

	if (saved) {
		free(saved->buf);
		free(saved);
	}

	logger_debug("end");

	return NPERR_NO_ERROR;
}

static NPError
pNPP_Destroy(NPP instance, NPSavedData **save)
{
	Wrapper *wrapper;
	RPCMsg msg = {
		.method = RPC_NPP_Destroy,
	};
	BufWriter w;
	BufReader r;
	NPError error;
	NPSavedData *saved;
	uint32_t len;
	void *data;

	logger_debug("start");

	wrapper = wrapper_get();
	if (wrapper == NULL)
		return NPERR_GENERIC_ERROR;

	logger_debug("instance: {%p, %p}", instance->pdata, instance->ndata);
	logger_debug("save: %p", save);
	if (save && *save)
		logger_dump("*save", (*save)->buf, (*save)->len);

	buf_writer_open(&w, 0);
	buf_uint64_encode(&w, (uintptr_t)instance);

	msg.param = w.data;
	msg.param_size = w.len;

	rpc_invoke(&wrapper->sess, &msg);

	buf_writer_close(&w);

	buf_reader_init(&r, msg.ret, msg.ret_size);
	if (buf_uint16_decode(&r, &error) == NULL) {
		logger_debug("buf_uint16_decode failed.");
		return NPERR_GENERIC_ERROR;
	}

	if (error != NPERR_NO_ERROR)
		return error;

	if (buf_uint32_decode(&r, &len) == NULL) {
		logger_debug("buf_uint16_decode failed.");
		return NPERR_GENERIC_ERROR;
	}

	if (len) {
		data = buf_bytes_decode(&r, len);
		saved = malloc(sizeof(*saved));
		if (saved != NULL) {
			saved->len = len;
			saved->buf = malloc(len);
			if (saved->buf == NULL) {
				free(saved);
			} else {
				memcpy(saved->buf, data, len);
				*save = saved;
			}
		}
	}

	free(instance->pdata);

	logger_debug("end");
	return NPERR_NO_ERROR;
}

static NPError
pNPP_SetWindow(NPP instance, NPWindow *window)
{
	logger_debug("start");
	logger_debug("instance: {%p, %p}", instance->pdata, instance->ndata);
	logger_debug("window: {");
	logger_debug("  window: %p", window->window);
	logger_debug("  x: %"PRId32, window->x);
	logger_debug("  y: %"PRId32, window->y);
	logger_debug("  width: %"PRIu32, window->width);
	logger_debug("  height: %"PRIu32, window->height);
	logger_debug("  clipRect: {");
	logger_debug("    top: %"PRIu16, window->clipRect.top);
	logger_debug("    left: %"PRIu16, window->clipRect.left);
	logger_debug("    bottom: %"PRIu16, window->clipRect.bottom);
	logger_debug("    right: %"PRIu16, window->clipRect.right);
	logger_debug("  }");
	logger_debug("  ws_info: %p", window->ws_info);
	logger_debug("  type: %d", window->type);
	logger_debug("}");
	logger_debug("end");
	return NPERR_NO_ERROR;
}

static NPError
pNPP_NewStream(NPP instance, NPMIMEType type, NPStream *stream,
		NPBool seekable, uint16_t *stype)
{
	logger_debug("start");
	logger_debug("instance: {%p, %p}", instance->pdata, instance->ndata);
	logger_debug("type: '%s'", type);
	logger_debug("stream {");
	logger_debug("  pdata: %p", stream->pdata);
	logger_debug("  ndata: %p", stream->ndata);
	logger_debug("  url: '%s'", stream->url);
	logger_debug("  end: %"PRIu32, stream->end);
	logger_debug("  lastmodified: %"PRIu32, stream->lastmodified);
	logger_debug("  notifyData: %p", stream->notifyData);
	logger_debug("  headers: '%s'", stream->headers);
	logger_debug("}");
	logger_debug("seekable: %d", seekable);
	logger_debug("stype: %p", stype);
	logger_debug("end");
	return NPERR_NO_ERROR;
}

static NPError
pNPP_DestroyStream(NPP instance, NPStream *stream, NPReason reason)
{
	logger_debug("start");
	logger_debug("instance: {%p, %p}", instance->pdata, instance->ndata);
	logger_debug("stream {");
	logger_debug("  pdata: %p", stream->pdata);
	logger_debug("  ndata: %p", stream->ndata);
	logger_debug("  url: '%s'", stream->url);
	logger_debug("  end: %"PRIu32, stream->end);
	logger_debug("  lastmodified: %"PRIu32, stream->lastmodified);
	logger_debug("  notifyData: %p", stream->notifyData);
	logger_debug("  headers: '%s'", stream->headers);
	logger_debug("}");
	logger_debug("reason: %u", reason);
	logger_debug("end");
	return NPERR_NO_ERROR;
}

static void
pNPP_StreamAsFile(NPP instance, NPStream *stream, const char *fname)
{
	logger_debug("start");
	logger_debug("instance: {%p, %p}", instance->pdata, instance->ndata);
	logger_debug("stream {");
	logger_debug("  pdata: %p", stream->pdata);
	logger_debug("  ndata: %p", stream->ndata);
	logger_debug("  url: '%s'", stream->url);
	logger_debug("  end: %"PRIu32, stream->end);
	logger_debug("  lastmodified: %"PRIu32, stream->lastmodified);
	logger_debug("  notifyData: %p", stream->notifyData);
	logger_debug("  headers: '%s'", stream->headers);
	logger_debug("}");
	logger_debug("fname: %s", fname);
	logger_debug("end");
}

static int32_t
pNPP_WriteReady(NPP instance, NPStream *stream)
{
	logger_debug("start");
	logger_debug("instance: {%p, %p}", instance->pdata, instance->ndata);
	logger_debug("stream {");
	logger_debug("  pdata: %p", stream->pdata);
	logger_debug("  ndata: %p", stream->ndata);
	logger_debug("  url: '%s'", stream->url);
	logger_debug("  end: %"PRIu32, stream->end);
	logger_debug("  lastmodified: %"PRIu32, stream->lastmodified);
	logger_debug("  notifyData: %p", stream->notifyData);
	logger_debug("  headers: '%s'", stream->headers);
	logger_debug("}");
	logger_debug("end");
	return 0;
}

static int32_t
pNPP_Write(NPP instance, NPStream *stream, int32_t offset,
		int32_t len, void *buffer)
{
	logger_debug("start");
	logger_debug("instance: {%p, %p}", instance->pdata, instance->ndata);
	logger_debug("stream {");
	logger_debug("  pdata: %p", stream->pdata);
	logger_debug("  ndata: %p", stream->ndata);
	logger_debug("  url: '%s'", stream->url);
	logger_debug("  end: %"PRIu32, stream->end);
	logger_debug("  lastmodified: %"PRIu32, stream->lastmodified);
	logger_debug("  notifyData: %p", stream->notifyData);
	logger_debug("  headers: '%s'", stream->headers);
	logger_debug("}");
	logger_debug("end");
	logger_debug("offset: %"PRId32, offset);
	logger_dump("buffer", buffer, len);
	logger_debug("end");
	return 0;
}

static void
pNPP_Print(NPP instance, NPPrint *platformPrint)
{
	logger_debug("start");
	logger_debug("instance: {%p, %p}", instance->pdata, instance->ndata);
	logger_debug("platformPrint {");
	logger_debug("  mode: %u", platformPrint->mode);
	logger_debug("  print {");
	logger_debug("    fullPrint {");
	logger_debug("      pluginPrinted: %d",
			platformPrint->print.fullPrint.pluginPrinted);
	logger_debug("      printOne: %d",
			platformPrint->print.fullPrint.printOne);
	logger_debug("      platformPrint: %p",
			platformPrint->print.fullPrint.platformPrint);
	logger_debug("    }");
	logger_debug("    embedPrint {");
	logger_debug("      window: ...");
	logger_debug("      platformPrint: %p",
			platformPrint->print.embedPrint.platformPrint);
	logger_debug("    }");
	logger_debug("  }");
	logger_debug("}");
	logger_debug("end");
}

static int16_t
pNPP_HandleEvent(NPP instance, void *event)
{
	logger_debug("start");
	logger_debug("instance: {%p, %p}", instance->pdata, instance->ndata);
	logger_debug("event: %p", event);
	logger_debug("end");
	return 0;
}

static void
pNPP_URLNotify(NPP instance, const char *url, NPReason reason, void *notifyData)
{
	logger_debug("start");
	logger_debug("instance: {%p, %p}", instance->pdata, instance->ndata);
	logger_debug("url: '%s'", url);
	logger_debug("reason: %u", reason);
	logger_debug("notifyData: %p", notifyData);
	logger_debug("end");
}

static NPError
pNPP_GetValue(NPP instance, NPPVariable variable, void *ret_value)
{
	logger_debug("start");
	logger_debug("instance: {%p, %p}", instance->pdata, instance->ndata);
	logger_debug("variable: %u", variable);
	logger_debug("ret_value: %p", ret_value);

	switch (variable) {
	case NPPVpluginNameString:
	case NPPVpluginDescriptionString:
		return NP_GetValue(NULL, variable, ret_value);
	default:
		logger_debug("end");
		return NPERR_GENERIC_ERROR;
	}
}

static NPError
pNPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
	logger_debug("start");
	logger_debug("instance: {%p, %p}", instance->pdata, instance->ndata);
	logger_debug("variable: %u", variable);
	logger_debug("value: %p", value);
	logger_debug("end");
	return NPERR_NO_ERROR;
}

/* NPP_GotFocus */
/* NPP_LostFocus */

static void
pNPP_URLRedirectNotify(NPP instance, const char *url, int32_t status,
		void *notifyData)
{
	logger_debug("start");
	logger_debug("instance: {%p, %p}", instance->pdata, instance->ndata);
	logger_debug("url: '%s'", url);
	logger_debug("status: %u", status);
	logger_debug("notifyData: %p", notifyData);
	logger_debug("end");
}

static NPError
pNPP_ClearSiteData(const char *site, uint64_t flags, uint64_t maxAge)
{
	logger_debug("start");
	logger_debug("site: '%s'", site);
	logger_debug("flags: %"PRIx64, flags);
	logger_debug("maxAge: %"PRIu64, maxAge);
	logger_debug("end");
	return NPERR_NO_ERROR;
}

static char **
pNPP_GetSitesWithData(void)
{
	logger_debug("start");
	return NULL;
}


NP_EXPORT(NPError)
NP_Initialize(NPNetscapeFuncs *bfuncs, NPPluginFuncs *pfuncs)
{
	Wrapper *wrapper;
	RPCMsg msg = {
		.method = RPC_NP_Initialize,
	};
	BufReader r;
	NPError error;

	logger_debug("start");

	logger_debug("NPNetscapeFuncs size: %u/%zu",
			bfuncs->size, sizeof(*bfuncs));
	logger_debug("NPPluginFuncs size: %u/%zu",
			pfuncs->size, sizeof(*pfuncs));

	wrapper = wrapper_get();
	if (wrapper == NULL)
		return NPERR_GENERIC_ERROR;

	rpc_invoke(&wrapper->sess, &msg);

	buf_reader_init(&r, msg.ret, msg.ret_size);
	if (buf_uint16_decode(&r, &error) == NULL) {
		logger_debug("buf_uint16_decode failed.");
		return NPERR_GENERIC_ERROR;
	}

	if (error != NPERR_NO_ERROR)
		return error;

	wrapper->bfuncs = bfuncs;

	pfuncs->newp = pNPP_New;
	pfuncs->destroy = pNPP_Destroy;
	pfuncs->setwindow = pNPP_SetWindow;
	pfuncs->newstream = pNPP_NewStream;
	pfuncs->destroystream = pNPP_DestroyStream;
	pfuncs->asfile = pNPP_StreamAsFile;
	pfuncs->writeready = pNPP_WriteReady;
	pfuncs->write = pNPP_Write;
	pfuncs->print = pNPP_Print;
	pfuncs->event = pNPP_HandleEvent;
	pfuncs->urlnotify = pNPP_URLNotify;
	pfuncs->javaClass = NULL;
	pfuncs->getvalue = pNPP_GetValue;
	pfuncs->setvalue = pNPP_SetValue;
	pfuncs->gotfocus = NULL;
	pfuncs->lostfocus = NULL;
	pfuncs->urlredirectnotify = pNPP_URLRedirectNotify;
	pfuncs->clearsitedata = pNPP_ClearSiteData;
	pfuncs->getsiteswithdata = pNPP_GetSitesWithData;
	pfuncs->didComposite = NULL;

	return NPERR_NO_ERROR;
}

NP_EXPORT(NPError)
NP_Shutdown(void)
{
	Wrapper *wrapper;
	RPCMsg msg = {
		.method = RPC_NP_Shutdown,
	};
	BufReader r;
	NPError error;

	logger_debug("start");

	wrapper = wrapper_get();
	if (wrapper == NULL)
		return NPERR_GENERIC_ERROR;

	rpc_invoke(&wrapper->sess, &msg);

	buf_reader_init(&r, msg.ret, msg.ret_size);
	if (buf_uint16_decode(&r, &error) == NULL) {
		logger_debug("buf_uint16_decode failed.");
		return NPERR_GENERIC_ERROR;
	}

	if (error != NPERR_NO_ERROR)
		return error;

	wrapper_close(wrapper);

	return NPERR_NO_ERROR;
}

NP_EXPORT(char *)
NP_GetPluginVersion(void)
{
	const char *str = "32,0,0,363";

	logger_debug("start");
	logger_debug("end('%s')", str);

	return (char *)str;
}

NP_EXPORT(const char *)
NP_GetMIMEDescription(void)
{
	const char *str = 
		"application/x-shockwave-flash:swf:Shockwave Flash;"
		"application/futuresplash:spl:FutureSplash Player";

	logger_debug("start");
	logger_debug("end('%s')", str);

	return str;
}

NP_EXPORT(NPError)
NP_GetValue(void *future, NPPVariable aVariable, void *aValue)
{
	logger_debug("start %p %d", future, aVariable);

	switch (aVariable) {
	case NPPVpluginNameString:
		logger_debug("NPPVpluginNameString");
		*((char **)aValue) = "Shockwave Flash";
		logger_debug("aValue: '%s'", *(char **)aValue);
		return NPERR_NO_ERROR;
	case NPPVpluginDescriptionString:
		logger_debug("NPPVpluginDescriptionString");
		*((char **)aValue) = "Shockwave Flash 32.0 r0";
		logger_debug("aValue: '%s'", *(char **)aValue);
		return NPERR_NO_ERROR;
	default:
		logger_debug("unknown variable: %d", aVariable);
		return NPERR_INVALID_PARAM;
	}
}
