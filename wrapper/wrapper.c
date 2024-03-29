#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>

#include <gtk/gtk.h>

#include "npapi.h"
#include "npfunctions.h"

#include "logger.h"
#include "util.h"
#include "buf.h"
#include "rpc.h"
#include "npobject.h"
#include "engine.h"


typedef struct Wrapper Wrapper;
typedef struct Plugin Plugin;

struct Wrapper {
	NPNetscapeFuncs *bfuncs;
	RPCSess sess;
	RPCSess rsess;
	Engine engine;
	int inited;
};

struct Plugin {
	void *a;
};


static Wrapper g_wrapper;

static void
wrapper_flush(NPP instance, Wrapper *wrapper)
{
	Display *display = NULL;
	NPError error;

	logger_debug("start");

	error = wrapper->bfuncs->getvalue(instance, NPNVxDisplay,
			(void *)&display);
	if (error != NPERR_NO_ERROR)
		return;
	if (display == NULL)
		return;
	XSync(display, 0);

	logger_debug("end");
}

static void
wrapper_ungrab(NPP instance, Wrapper *wrapper, Time time)
{
	Display *display = NULL;
	NPError error;

	logger_debug("start");

	error = wrapper->bfuncs->getvalue(instance, NPNVxDisplay,
			(void *)&display);
	if (error != NPERR_NO_ERROR)
		return;
	if (display == NULL)
		return;
	XUngrabPointer(display, time);

	logger_debug("end");
}


static void
NPN_UserAgent_handle(Wrapper *wrapper, RPCMsg *msg)
{
	RPCSess *sess = &wrapper->rsess;
	NPP instance;
	BufReader r;
	BufWriter w;
	uint32_t len;
	const char *s;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);
	buf_uint64_decode(&r, (uint64_t *)&instance);

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
	RPCSess *sess = &wrapper->rsess;
	NPP instance;
	BufWriter w;
	BufReader r;
	NPNVariable variable;
	NPError error;
	uint32_t len;
	NPNToolkitType toolkit;
	/* GdkNativeWindow  xid */
	uintptr_t xid;
	NPBool boolval;
	NPObject *npobj;
	char *s;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);

	buf_uint64_decode(&r, (uint64_t *)&instance);
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
	case NPNVnetscapeWindow:
		error = wrapper->bfuncs->getvalue(instance, variable,
				(void *)&xid);
		logger_debug("value: %p", xid);
		buf_uint16_encode(&w, error);
		buf_uint64_encode(&w, xid);
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
	RPCSess *sess = &wrapper->rsess;
	NPP instance;
	BufWriter w;
	BufReader r;
	NPPVariable variable;
	NPError error;
	NPBool boolval;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);
	buf_uint64_decode(&r, (uint64_t *)&instance);

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
	RPCSess *sess = &wrapper->rsess;
	BufReader r;
	NPObject *npobj;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);
	if (buf_uint64_decode(&r, (uint64_t *)&npobj) == NULL) {
		logger_debug("buf_uint64_decode failed.");
		return;
	}

	logger_debug("npobj: %p", npobj);

	wrapper->bfuncs->releaseobject(npobj);

	rpc_return(sess, msg);

	logger_debug("end");
}

static void
NPN_GetStringIdentifier_handle(Wrapper *wrapper, RPCMsg *msg)
{
	RPCSess *sess = &wrapper->rsess;
	BufReader r;
	BufWriter w;
	uint32_t len;
	const char *s;
	NPIdentifier id;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);
	buf_uint32_decode(&r, &len);
	if (len == 0)
		s = NULL;
	else
		s = buf_bytes_decode(&r, len);

	logger_debug("name: %s", s);

	id = wrapper->bfuncs->getstringidentifier(s);

	buf_writer_open(&w, 0);
	buf_uint64_encode(&w, (uintptr_t)id);

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NPN_GetProperty_handle(Wrapper *wrapper, RPCMsg *msg)
{
	RPCSess *sess = &wrapper->rsess;
	BufReader r;
	BufWriter w;
	NPP instance;
	NPObject *npobj;
	NPIdentifier property;
	NPVariant result;
	NPString *stringValue;
	bool ret;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);

	buf_uint64_decode(&r, (uint64_t *)&instance);
	logger_debug("instance: %p", instance);

	buf_uint64_decode(&r, (uint64_t *)&npobj);
	logger_debug("npobj: %p", npobj);

	buf_uint64_decode(&r, (uint64_t *)&property);
	logger_debug("property: %p", property);

	VOID_TO_NPVARIANT(result);

	ret = wrapper->bfuncs->getproperty(instance, npobj, property, &result);

	logger_debug("ret: %u", ret);

	buf_writer_open(&w, 0);
	buf_uint8_encode(&w, ret);

	buf_uint32_encode(&w, result.type);
	switch (result.type) {
	case NPVariantType_Void:
		logger_debug("void");
		break;
	case NPVariantType_Null:
		logger_debug("null");
		break;
	case NPVariantType_Bool:
		logger_debug("bool: %u", result.value.boolValue);
		buf_uint8_encode(&w, result.value.boolValue);
		break;
	case NPVariantType_Int32:
		logger_debug("int: %"PRId32, result.value.intValue);
		buf_uint32_encode(&w, result.value.intValue);
		break;
	case NPVariantType_Double:
		logger_debug("double: %f", result.value.doubleValue);
		buf_double_encode(&w, result.value.doubleValue);
		break;
	case NPVariantType_String:
		stringValue = &result.value.stringValue;
		logger_debug("string: %s", stringValue->UTF8Characters);
		buf_uint32_encode(&w, stringValue->UTF8Length + 1);
		buf_bytes_encode(&w, stringValue->UTF8Characters,
				stringValue->UTF8Length + 1);
		break;
	case NPVariantType_Object:
		logger_debug("object: %p", result.value.objectValue);
		buf_uint64_encode(&w, (uintptr_t)result.value.objectValue);
		break;
	default:
		break;
	}

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NPN_Evaluate_handle(Wrapper *wrapper, RPCMsg *msg)
{
	RPCSess *sess = &wrapper->rsess;
	BufReader r;
	BufWriter w;
	NPP instance;
	NPObject *npobj;
	NPString script;
	NPVariant result;
	NPString *stringValue;
	uint32_t len;
	bool ret;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);

	buf_uint64_decode(&r, (uint64_t *)&instance);
	logger_debug("instance: %p", instance);

	buf_uint64_decode(&r, (uint64_t *)&npobj);
	logger_debug("npobj: %p", npobj);

	buf_uint32_decode(&r, &len);
	script.UTF8Length = len - 1;
	script.UTF8Characters = buf_bytes_decode(&r, len);
	logger_debug("script: '%s'", script.UTF8Characters);

	VOID_TO_NPVARIANT(result);

	ret = wrapper->bfuncs->evaluate(instance, npobj, &script, &result);

	logger_debug("ret: %u", ret);

	buf_writer_open(&w, 0);
	buf_uint8_encode(&w, ret);

	buf_uint32_encode(&w, result.type);
	switch (result.type) {
	case NPVariantType_Void:
		logger_debug("void");
		break;
	case NPVariantType_Null:
		logger_debug("null");
		break;
	case NPVariantType_Bool:
		logger_debug("bool: %u", result.value.boolValue);
		buf_uint8_encode(&w, result.value.boolValue);
		break;
	case NPVariantType_Int32:
		logger_debug("int: %"PRId32, result.value.intValue);
		buf_uint32_encode(&w, result.value.intValue);
		break;
	case NPVariantType_Double:
		logger_debug("double: %f", result.value.doubleValue);
		buf_double_encode(&w, result.value.doubleValue);
		break;
	case NPVariantType_String:
		stringValue = &result.value.stringValue;
		logger_debug("string: %s", stringValue->UTF8Characters);
		buf_uint32_encode(&w, stringValue->UTF8Length + 1);
		buf_bytes_encode(&w, stringValue->UTF8Characters,
				stringValue->UTF8Length + 1);
		break;
	case NPVariantType_Object:
		logger_debug("object: %p", result.value.objectValue);
		buf_uint64_encode(&w, (uintptr_t)result.value.objectValue);
		break;
	default:
		break;
	}

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NPN_InvalidateRect_handle(Wrapper *wrapper, RPCMsg *msg)
{
	RPCSess *sess = &wrapper->rsess;
	NPP instance;
	BufReader r;
	NPRect invalidRect;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);
	buf_uint64_decode(&r, (uint64_t *)&instance);
	logger_debug("instance: %p");
	buf_uint16_decode(&r, &invalidRect.top);
	buf_uint16_decode(&r, &invalidRect.left);
	buf_uint16_decode(&r, &invalidRect.bottom);
	buf_uint16_decode(&r, &invalidRect.right);
	logger_debug("invalidRect {");
	logger_debug("  top: %"PRIu16, invalidRect.top);
	logger_debug("  left: %"PRIu16, invalidRect.left);
	logger_debug("  bottom: %"PRIu16, invalidRect.bottom);
	logger_debug("  right: %"PRIu16, invalidRect.right);
	logger_debug("}");

	wrapper->bfuncs->invalidaterect(instance, &invalidRect);

	rpc_return(sess, msg);

	logger_debug("end");
}

static void
NPN_GetURLNotify_handle(Wrapper *wrapper, RPCMsg *msg)
{
	RPCSess *sess = &wrapper->rsess;
	NPP instance;
	BufReader r;
	BufWriter w;
	NPError error;
	uint32_t len;
	const char *relativeURL;
	const char *target;
	void *notifyData;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);
	buf_uint64_decode(&r, (uint64_t *)&instance);
	logger_debug("instance: %p");
	buf_uint32_decode(&r, &len);
	if (len) {
		relativeURL = buf_bytes_decode(&r, len);
	} else {
		relativeURL = NULL;
	}
	logger_debug("relativeURL: %s", relativeURL);

	buf_uint32_decode(&r, &len);
	if (len) {
		target = buf_bytes_decode(&r, len);
	} else {
		target = NULL;
	}
	logger_debug("target: %s", target);

	buf_uint64_decode(&r, (uint64_t *)&notifyData);
	logger_debug("notifyData: %p", notifyData);

	error = wrapper->bfuncs->geturlnotify(instance, relativeURL, target,
			notifyData);

	logger_debug("error: %s(%d)", np_errorstr(error), error);

	buf_writer_open(&w, 0);
	buf_uint16_encode(&w, error);

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NPN_PushPopupsEnabledState_handle(Wrapper *wrapper, RPCMsg *msg)
{
	RPCSess *sess = &wrapper->rsess;
	NPP instance;
	BufReader r;
	NPBool enabled;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);
	buf_uint64_decode(&r, (uint64_t *)&instance);
	logger_debug("instance: %p", instance);

	buf_uint8_decode(&r, &enabled);
	logger_debug("enabled: %u", enabled);

	wrapper->bfuncs->pushpopupsenabledstate(instance, enabled);

	rpc_return(sess, msg);

	logger_debug("end");
}

static void
NPN_PopPopupsEnabledState_handle(Wrapper *wrapper, RPCMsg *msg)
{
	RPCSess *sess = &wrapper->rsess;
	NPP instance;
	BufReader r;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);
	buf_uint64_decode(&r, (uint64_t *)&instance);
	logger_debug("instance: %p", instance);

	wrapper->bfuncs->poppopupsenabledstate(instance);

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
	case RPC_NPN_GetStringIdentifier:
		NPN_GetStringIdentifier_handle(wrapper, msg);
		break;
	case RPC_NPN_GetProperty:
		NPN_GetProperty_handle(wrapper, msg);
		break;
	case RPC_NPN_Evaluate:
		NPN_Evaluate_handle(wrapper, msg);
		break;
	case RPC_NPN_InvalidateRect:
		NPN_InvalidateRect_handle(wrapper, msg);
		break;
	case RPC_NPN_GetURLNotify:
		NPN_GetURLNotify_handle(wrapper, msg);
		break;
	case RPC_NPN_PushPopupsEnabledState:
		NPN_PushPopupsEnabledState_handle(wrapper, msg);
		break;
	case RPC_NPN_PopPopupsEnabledState:
		NPN_PopPopupsEnabledState_handle(wrapper, msg);
		break;
	default:
		break;
	}
}

static gboolean
io_handler(GIOChannel *source, GIOCondition condition, gpointer data)
{
	RPCSess *sess = data;

	logger_debug("start");

	if (rpc_handle(sess) != 0) {
		logger_debug("rpc_handle failed.");
                return false;
        }

        logger_debug("end");
	return true;
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

	rpcsess_init(&wrapper->rsess, wrapper->engine.rfd, NULL,
			npn_dispatch, wrapper);
	rpcsess_init(&wrapper->sess, wrapper->engine.fd, &wrapper->rsess,
			NULL, NULL);
	{
		GIOChannel *channel;

		channel = g_io_channel_unix_new(wrapper->engine.fd);
		if (channel == NULL) {
			return -1;
		}
		g_io_channel_set_encoding(channel, NULL, NULL);
		g_io_add_watch(channel, G_IO_IN | G_IO_HUP, io_handler,
				&wrapper->rsess);
	}

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

	free(msg.ret);

	if (error != NPERR_NO_ERROR)
		return error;

	instance->pdata = malloc(sizeof(Plugin));

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
	const void *data;

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

	logger_debug("error: %s(%d)", np_errorstr(error), error);

	if (error != NPERR_NO_ERROR)
		return error;

	if (buf_uint32_decode(&r, &len) == NULL) {
		logger_debug("buf_uint16_decode failed.");
		return NPERR_GENERIC_ERROR;
	}

	if (len) {
		data = buf_bytes_decode(&r, len);
		if (save) {
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
	}

	free(instance->pdata);

	logger_debug("end");
	return NPERR_NO_ERROR;
}

static NPError
pNPP_SetWindow(NPP instance, NPWindow *window)
{
	Wrapper *wrapper;
	RPCSess *sess;
	RPCMsg msg = {
		.method = RPC_NPP_SetWindow,
	};
	BufReader r;
	BufWriter w;
	NPSetWindowCallbackStruct *ws_info;
	VisualID visualid;
	NPError error;

	logger_debug("start");

	wrapper = wrapper_get();
	if (wrapper == NULL)
		return NPERR_GENERIC_ERROR;

	sess = &wrapper->sess;

	buf_writer_open(&w, 0);

	if (instance)
		logger_debug("instance: {%p, %p}",
				instance->pdata, instance->ndata);
	buf_uint64_encode(&w, (uintptr_t)instance);

	if (window) {
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
		ws_info = window->ws_info;
		if (ws_info) {
			logger_debug("    type: %"PRId32, ws_info->type);
			logger_debug("    display: %p", ws_info->display);
			logger_debug("    visual: %p", ws_info->visual);
			logger_debug("    colormap: %"PRIx64, ws_info->colormap);
			logger_debug("    depth: %u", ws_info->depth);
		}
		logger_debug("  type: %d", window->type);
		logger_debug("}");
		buf_uint32_encode(&w, sizeof(*window));
		buf_uint64_encode(&w, (uintptr_t)window->window);
		buf_uint32_encode(&w, window->x);
		buf_uint32_encode(&w, window->y);
		buf_uint32_encode(&w, window->width);
		buf_uint32_encode(&w, window->height);
		buf_uint16_encode(&w, window->clipRect.top);
		buf_uint16_encode(&w, window->clipRect.left);
		buf_uint16_encode(&w, window->clipRect.bottom);
		buf_uint16_encode(&w, window->clipRect.right);
		if (ws_info) {
			buf_uint32_encode(&w, sizeof(*ws_info));
			buf_uint32_encode(&w, ws_info->type);
			buf_uint64_encode(&w, (uintptr_t)ws_info->display);
			buf_uint64_encode(&w, (uintptr_t)ws_info->visual);
			/* Visual -> Visual ID */
			if (ws_info->visual) {
				visualid = XVisualIDFromVisual(ws_info->visual);
			} else {
				visualid = 0;
			}
			logger_debug("    visualID: %lu", visualid);
			buf_uint64_encode(&w, visualid);
			buf_uint64_encode(&w, ws_info->colormap);
			buf_uint32_encode(&w, ws_info->depth);
		} else {
			buf_uint32_encode(&w, 0);
		}
		buf_uint32_encode(&w, window->type);
	} else {
		buf_uint32_encode(&w, 0);
	}

	msg.param = w.data;
	msg.param_size = w.len;

	rpc_invoke(&wrapper->sess, &msg);

	buf_writer_close(&w);

	buf_reader_init(&r, msg.ret, msg.ret_size);
	buf_uint16_decode(&r, &error);

	logger_debug("error: %s(%d)", np_errorstr(error), error);

	free(msg.ret);

	logger_debug("end");
	return error;
}

static NPError
pNPP_NewStream(NPP instance, NPMIMEType type, NPStream *stream,
		NPBool seekable, uint16_t *stype)
{
	Wrapper *wrapper;
	RPCMsg msg = {
		.method = RPC_NPP_NewStream,
	};
	BufWriter w;
	BufReader r;
	NPError error;
	uint32_t len;

	logger_debug("start");

	wrapper = wrapper_get();
	if (wrapper == NULL)
		return NPERR_GENERIC_ERROR;

	buf_writer_open(&w, 0);

	if (instance)
		logger_debug("instance: {%p, %p}",
				instance->pdata, instance->ndata);
	buf_uint64_encode(&w, (uintptr_t)instance);

	logger_debug("type: '%s'", type);
	if (type) {
		len = strlen(type) + 1;
		buf_uint32_encode(&w, len);
		buf_bytes_encode(&w, type, len);
	} else {
		buf_uint32_encode(&w, 0);
	}

	logger_debug("stream {");
	logger_debug("  pdata: %p", stream->pdata);
	buf_uint64_encode(&w, (uintptr_t)stream->pdata);
	logger_debug("  ndata: %p", stream->ndata);
	buf_uint64_encode(&w, (uintptr_t)stream->ndata);
	logger_debug("  url: '%s'", stream->url);
	if (stream->url) {
		len = strlen(stream->url) + 1;
		buf_uint32_encode(&w, len);
		buf_bytes_encode(&w, stream->url, len);
	} else {
		buf_uint32_encode(&w, 0);
	}
	logger_debug("  end: %"PRIu32, stream->end);
	buf_uint32_encode(&w, stream->end);
	logger_debug("  lastmodified: %"PRIu32, stream->lastmodified);
	buf_uint32_encode(&w, stream->lastmodified);
	logger_debug("  notifyData: %p", stream->notifyData);
	buf_uint64_encode(&w, (uintptr_t)stream->notifyData);
	logger_debug("  headers: '%s'", stream->headers);
	if (stream->headers) {
		len = strlen(stream->headers) + 1;
		buf_uint32_encode(&w, len);
		buf_bytes_encode(&w, stream->headers, len);
	} else {
		buf_uint32_encode(&w, 0);
	}
	logger_debug("}");

	logger_debug("seekable: %u", seekable);
	buf_uint8_encode(&w, seekable);

	msg.param = w.data;
	msg.param_size = w.len;

	rpc_invoke(&wrapper->sess, &msg);

	buf_writer_close(&w);

	buf_reader_init(&r, msg.ret, msg.ret_size);
	buf_uint16_decode(&r, &error);
	logger_debug("error: %s(%d)", np_errorstr(error), error);
	buf_uint64_decode(&r, (uint64_t *)&stream->pdata);
	logger_debug("pdata: %p", stream->pdata);
	buf_uint64_decode(&r, (uint64_t *)&stream->notifyData);
	logger_debug("notifyData: %p", stream->notifyData);
	buf_uint16_decode(&r, stype);
	logger_debug("stype: %u", *stype);

	free(msg.ret);

	logger_debug("end");
	return error;
}

static NPError
pNPP_DestroyStream(NPP instance, NPStream *stream, NPReason reason)
{
	Wrapper *wrapper;
	RPCMsg msg = {
		.method = RPC_NPP_DestroyStream,
	};
	BufWriter w;
	BufReader r;
	NPError error;

	logger_debug("start");

	wrapper = wrapper_get();
	if (wrapper == NULL)
		return NPERR_GENERIC_ERROR;

	buf_writer_open(&w, 0);

	if (instance)
		logger_debug("instance: {%p, %p}",
				instance->pdata, instance->ndata);
	buf_uint64_encode(&w, (uintptr_t)instance);

	logger_debug("stream {");
	logger_debug("  pdata: %p", stream->pdata);
	buf_uint64_encode(&w, (uintptr_t)stream->pdata);
	logger_debug("  ndata: %p", stream->ndata);
	buf_uint64_encode(&w, (uintptr_t)stream->ndata);
	logger_debug("  url: '%s'", stream->url);
	logger_debug("  end: %"PRIu32, stream->end);
	logger_debug("  lastmodified: %"PRIu32, stream->lastmodified);
	logger_debug("  notifyData: %p", stream->notifyData);
	logger_debug("  headers: '%s'", stream->headers);
	logger_debug("}");

	logger_debug("reason: %u", reason);
	buf_uint16_encode(&w, reason);

	msg.param = w.data;
	msg.param_size = w.len;

	rpc_invoke(&wrapper->sess, &msg);

	buf_writer_close(&w);

	buf_reader_init(&r, msg.ret, msg.ret_size);
	buf_uint16_decode(&r, &error);
	logger_debug("error: %s(%d)", np_errorstr(error), error);

	free(msg.ret);

	logger_debug("end");
	return error;
}

static void
pNPP_StreamAsFile(NPP instance, NPStream *stream, const char *fname)
{
	logger_debug("start");
	if (instance)
		logger_debug("instance: {%p, %p}",
				instance->pdata, instance->ndata);
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
	Wrapper *wrapper;
	RPCMsg msg = {
		.method = RPC_NPP_WriteReady,
	};
	BufWriter w;
	BufReader r;
	int32_t ret;

	logger_debug("start");

	wrapper = wrapper_get();
	if (wrapper == NULL)
		return NPERR_GENERIC_ERROR;

	buf_writer_open(&w, 0);

	if (instance)
		logger_debug("instance: {%p, %p}",
				instance->pdata, instance->ndata);
	buf_uint64_encode(&w, (uintptr_t)instance);

	logger_debug("stream {");
	logger_debug("  pdata: %p", stream->pdata);
	buf_uint64_encode(&w, (uintptr_t)stream->pdata);
	logger_debug("  ndata: %p", stream->ndata);
	buf_uint64_encode(&w, (uintptr_t)stream->ndata);
	logger_debug("  url: '%s'", stream->url);
	logger_debug("  end: %"PRIu32, stream->end);
	logger_debug("  lastmodified: %"PRIu32, stream->lastmodified);
	logger_debug("  notifyData: %p", stream->notifyData);
	logger_debug("  headers: '%s'", stream->headers);
	logger_debug("}");

	msg.param = w.data;
	msg.param_size = w.len;

	rpc_invoke(&wrapper->sess, &msg);

	buf_writer_close(&w);

	buf_reader_init(&r, msg.ret, msg.ret_size);
	buf_uint32_decode(&r, &ret);
	logger_debug("ret: %"PRId32, ret);

	free(msg.ret);

	logger_debug("end");

	return ret;
}

static int32_t
pNPP_Write(NPP instance, NPStream *stream, int32_t offset,
		int32_t len, void *buffer)
{
	Wrapper *wrapper;
	RPCMsg msg = {
		.method = RPC_NPP_Write,
	};
	BufWriter w;
	BufReader r;
	int32_t ret;

	logger_debug("start");

	wrapper = wrapper_get();
	if (wrapper == NULL)
		return NPERR_GENERIC_ERROR;

	buf_writer_open(&w, 0);

	if (instance)
		logger_debug("instance: {%p, %p}",
				instance->pdata, instance->ndata);
	buf_uint64_encode(&w, (uintptr_t)instance);

	logger_debug("stream {");
	logger_debug("  pdata: %p", stream->pdata);
	buf_uint64_encode(&w, (uintptr_t)stream->pdata);
	logger_debug("  ndata: %p", stream->ndata);
	buf_uint64_encode(&w, (uintptr_t)stream->ndata);
	logger_debug("  url: '%s'", stream->url);
	logger_debug("  end: %"PRIu32, stream->end);
	logger_debug("  lastmodified: %"PRIu32, stream->lastmodified);
	logger_debug("  notifyData: %p", stream->notifyData);
	logger_debug("  headers: '%s'", stream->headers);
	logger_debug("}");

	logger_debug("offset: %"PRId32, offset);
	buf_uint32_encode(&w, offset);
	logger_debug("len: %"PRId32, len);
	buf_uint32_encode(&w, len);
	//logger_dump("buffer", buffer, len);
	buf_bytes_encode(&w, buffer, len);

	msg.param = w.data;
	msg.param_size = w.len;

	rpc_invoke(&wrapper->sess, &msg);

	buf_writer_close(&w);

	buf_reader_init(&r, msg.ret, msg.ret_size);
	buf_uint32_decode(&r, &ret);
	logger_debug("ret: %"PRId32, ret);

	free(msg.ret);

	logger_debug("end");

	return ret;
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
	Wrapper *wrapper;
	RPCSess *sess;
	RPCMsg msg = {
		.method = RPC_NPP_HandleEvent,
	};
	BufReader r;
	BufWriter w;
	NPEvent *npevent = event;
	int16_t ret;

	logger_debug("start");

	wrapper = wrapper_get();
	if (wrapper == NULL)
		return NPERR_GENERIC_ERROR;

	sess = &wrapper->sess;

	buf_writer_open(&w, 0);

	if (instance)
		logger_debug("instance: {%p, %p}",
				instance->pdata, instance->ndata);
	buf_uint64_encode(&w, (uintptr_t)instance);

	logger_debug("event: %p", event);
	logger_debug("  type: %d", npevent->type);
	buf_uint32_encode(&w, npevent->type);

	logger_debug("  serial: %d", npevent->xany.serial);
	buf_uint64_encode(&w, npevent->xany.serial);
	logger_debug("  send_event: %d", npevent->xany.send_event);
	buf_uint32_encode(&w, npevent->xany.send_event);
	logger_debug("  window: %lu", npevent->xany.window);
	buf_uint64_encode(&w, npevent->xany.window);

	switch (npevent->type) {
	case GraphicsExpose:
		wrapper_flush(instance, wrapper);
		logger_debug("expose");
		logger_debug("  x: %d", npevent->xgraphicsexpose.x);
		buf_uint32_encode(&w, npevent->xgraphicsexpose.x);
		logger_debug("  y: %d", npevent->xgraphicsexpose.y);
		buf_uint32_encode(&w, npevent->xgraphicsexpose.y);
		logger_debug("  width: %d", npevent->xgraphicsexpose.width);
		buf_uint32_encode(&w, npevent->xgraphicsexpose.width);
		logger_debug("  height: %d", npevent->xgraphicsexpose.height);
		buf_uint32_encode(&w, npevent->xgraphicsexpose.height);
		logger_debug("  count: %d", npevent->xgraphicsexpose.count);
		buf_uint32_encode(&w, npevent->xgraphicsexpose.count);
		logger_debug("  major_code: %d",
				npevent->xgraphicsexpose.major_code);
		buf_uint32_encode(&w, npevent->xgraphicsexpose.major_code);
		logger_debug("  minor_code: %d",
				npevent->xgraphicsexpose.minor_code);
		buf_uint32_encode(&w, npevent->xgraphicsexpose.minor_code);
		break;
	case FocusIn:
	case FocusOut:
		logger_debug("focus");
		logger_debug("  mode: %d", npevent->xfocus.mode);
		buf_uint32_encode(&w, npevent->xfocus.mode);
		logger_debug("  detail: %d", npevent->xfocus.detail);
		buf_uint32_encode(&w, npevent->xfocus.detail);
		break;
	case EnterNotify:
	case LeaveNotify:
		logger_debug("crossing");
		logger_debug("  root: %lu", npevent->xcrossing.root);
		buf_uint64_encode(&w, npevent->xcrossing.root);
		logger_debug("  subwindow: %lu", npevent->xcrossing.subwindow);
		buf_uint64_encode(&w, npevent->xcrossing.subwindow);
		logger_debug("  time: %lu", npevent->xcrossing.time);
		buf_uint64_encode(&w, npevent->xcrossing.time);
		logger_debug("  x: %d", npevent->xcrossing.x);
		buf_uint32_encode(&w, npevent->xcrossing.x);
		logger_debug("  y: %d", npevent->xcrossing.y);
		buf_uint32_encode(&w, npevent->xcrossing.y);
		logger_debug("  x_root: %d", npevent->xcrossing.x_root);
		buf_uint32_encode(&w, npevent->xcrossing.x_root);
		logger_debug("  y_root: %d", npevent->xcrossing.y_root);
		buf_uint32_encode(&w, npevent->xcrossing.y_root);
		logger_debug("  mode: %d", npevent->xcrossing.mode);
		buf_uint32_encode(&w, npevent->xcrossing.mode);
		logger_debug("  detail: %d", npevent->xcrossing.detail);
		buf_uint32_encode(&w, npevent->xcrossing.detail);
		logger_debug("  same_screen: %d", npevent->xcrossing.same_screen);
		buf_uint32_encode(&w, npevent->xcrossing.same_screen);
		logger_debug("  focus: %d", npevent->xcrossing.focus);
		buf_uint32_encode(&w, npevent->xcrossing.focus);
		logger_debug("  state: %u", npevent->xcrossing.state);
		buf_uint32_encode(&w, npevent->xcrossing.state);
		break;
	case MotionNotify:
		logger_debug("motion");
		logger_debug("  root: %lu", npevent->xmotion.root);
		buf_uint64_encode(&w, npevent->xmotion.root);
		logger_debug("  subwindow: %lu", npevent->xmotion.subwindow);
		buf_uint64_encode(&w, npevent->xmotion.subwindow);
		logger_debug("  time: %lu", npevent->xmotion.time);
		buf_uint64_encode(&w, npevent->xmotion.time);
		logger_debug("  x: %d", npevent->xmotion.x);
		buf_uint32_encode(&w, npevent->xmotion.x);
		logger_debug("  y: %d", npevent->xmotion.y);
		buf_uint32_encode(&w, npevent->xmotion.y);
		logger_debug("  x_root: %d", npevent->xmotion.x_root);
		buf_uint32_encode(&w, npevent->xmotion.x_root);
		logger_debug("  y_root: %d", npevent->xmotion.y_root);
		buf_uint32_encode(&w, npevent->xmotion.y_root);
		logger_debug("  state: %u", npevent->xmotion.state);
		buf_uint32_encode(&w, npevent->xmotion.state);
		logger_debug("  is_hint: %u", npevent->xmotion.is_hint);
		buf_uint8_encode(&w, npevent->xmotion.is_hint);
		logger_debug("  same_screen: %d", npevent->xmotion.same_screen);
		buf_uint32_encode(&w, npevent->xmotion.same_screen);
		break;
	case ButtonPress:
		wrapper_flush(instance, wrapper);
		wrapper_ungrab(instance, wrapper, npevent->xbutton.time);
	case ButtonRelease:
		logger_debug("button");
		logger_debug("  root: %lu", npevent->xbutton.root);
		buf_uint64_encode(&w, npevent->xbutton.root);
		logger_debug("  subwindow: %lu", npevent->xbutton.subwindow);
		buf_uint64_encode(&w, npevent->xbutton.subwindow);
		logger_debug("  time: %lu", npevent->xbutton.time);
		buf_uint64_encode(&w, npevent->xbutton.time);
		logger_debug("  x: %d", npevent->xbutton.x);
		buf_uint32_encode(&w, npevent->xbutton.x);
		logger_debug("  y: %d", npevent->xbutton.y);
		buf_uint32_encode(&w, npevent->xbutton.y);
		logger_debug("  x_root: %d", npevent->xbutton.x_root);
		buf_uint32_encode(&w, npevent->xbutton.x_root);
		logger_debug("  y_root: %d", npevent->xbutton.y_root);
		buf_uint32_encode(&w, npevent->xbutton.y_root);
		logger_debug("  state: %u", npevent->xbutton.state);
		buf_uint32_encode(&w, npevent->xbutton.state);
		logger_debug("  button: %u", npevent->xbutton.button);
		buf_uint32_encode(&w, npevent->xbutton.button);
		logger_debug("  same_screen: %d", npevent->xbutton.same_screen);
		buf_uint32_encode(&w, npevent->xbutton.same_screen);
		break;
	case KeyPress:
	case KeyRelease:
		logger_debug("key");
		logger_debug("  root: %lu", npevent->xkey.root);
		buf_uint64_encode(&w, npevent->xkey.root);
		logger_debug("  subwindow: %lu", npevent->xkey.subwindow);
		buf_uint64_encode(&w, npevent->xkey.subwindow);
		logger_debug("  time: %lu", npevent->xkey.time);
		buf_uint64_encode(&w, npevent->xkey.time);
		logger_debug("  x: %d", npevent->xkey.x);
		buf_uint32_encode(&w, npevent->xkey.x);
		logger_debug("  y: %d", npevent->xkey.y);
		buf_uint32_encode(&w, npevent->xkey.y);
		logger_debug("  x_root: %d", npevent->xkey.x_root);
		buf_uint32_encode(&w, npevent->xkey.x_root);
		logger_debug("  y_root: %d", npevent->xkey.y_root);
		buf_uint32_encode(&w, npevent->xkey.y_root);
		logger_debug("  state: %u", npevent->xkey.state);
		buf_uint32_encode(&w, npevent->xkey.state);
		logger_debug("  keycode: %u", npevent->xkey.keycode);
		buf_uint32_encode(&w, npevent->xkey.keycode);
		logger_debug("  same_screen: %d", npevent->xkey.same_screen);
		buf_uint32_encode(&w, npevent->xkey.same_screen);
		break;
	default:
		logger_debug("unknown");
		break;
	}

	msg.param = w.data;
	msg.param_size = w.len;

	rpc_invoke(&wrapper->sess, &msg);

	buf_writer_close(&w);

	buf_reader_init(&r, msg.ret, msg.ret_size);
	buf_uint16_decode(&r, &ret);

	logger_debug("ret: %"PRId16, ret);

	free(msg.ret);

	logger_debug("end");
	return ret;
}

static void
pNPP_URLNotify(NPP instance, const char *url, NPReason reason, void *notifyData)
{
	Wrapper *wrapper;
	RPCSess *sess;
	RPCMsg msg = {
		.method = RPC_NPP_URLNotify,
	};
	BufWriter w;
	uint32_t len;

	logger_debug("start");

	wrapper = wrapper_get();
	if (wrapper == NULL)
		return;

	sess = &wrapper->sess;

	buf_writer_open(&w, 0);

	if (instance)
		logger_debug("instance: {%p, %p}",
				instance->pdata, instance->ndata);
	buf_uint64_encode(&w, (uintptr_t)instance);

	logger_debug("url: '%s'", url);
	if (url) {
		len = strlen(url) + 1;
		buf_uint32_encode(&w, len);
		buf_bytes_encode(&w, url, len);
	} else {
		buf_uint32_encode(&w, 0);
	}

	logger_debug("reason: %u", reason);
	buf_uint16_encode(&w, reason);

	logger_debug("notifyData: %p", notifyData);
	buf_uint64_encode(&w, (uintptr_t)notifyData);

	msg.param = w.data;
	msg.param_size = w.len;

	rpc_invoke(&wrapper->sess, &msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static NPError
pNPP_GetValue(NPP instance, NPPVariable variable, void *ret_value)
{
	Wrapper *wrapper;
	RPCSess *sess;
	RPCMsg msg = {
		.method = RPC_NPP_GetValue,
	};
	BufReader r;
	BufWriter w;
	NPError error;
	uintptr_t ident;
	uint32_t x32;
	NPObject *npobj;

	logger_debug("start");

	wrapper = wrapper_get();
	if (wrapper == NULL)
		return NPERR_GENERIC_ERROR;

	sess = &wrapper->sess;

	if (instance)
		logger_debug("instance: {%p, %p}",
				instance->pdata, instance->ndata);
	logger_debug("variable: %s(%u)", npp_varstr(variable), variable);
	logger_debug("ret_value: %p", ret_value);

	switch (variable) {
	case NPPVpluginNameString:
	case NPPVpluginDescriptionString:
		return NP_GetValue(NULL, variable, ret_value);
	default:
		break;
	}

	buf_writer_open(&w, 0);
	buf_uint64_encode(&w, (uintptr_t)instance);
	buf_uint32_encode(&w, variable);

	msg.param = w.data;
	msg.param_size = w.len;

	rpc_invoke(&wrapper->sess, &msg);

	buf_writer_close(&w);

	buf_reader_init(&r, msg.ret, msg.ret_size);
	buf_uint16_decode(&r, &error);

	logger_debug("error: %s(%d)", np_errorstr(error), error);
	if (error != NPERR_NO_ERROR) {
		free(msg.ret);
		return error;
	}

	switch (variable) {
	case NPPVpluginScriptableNPObject:
		buf_uint64_decode(&r, (uint64_t *)&ident);
		logger_debug("ident: %p", ident);
		npobj = npobject_find(ident);
		if (!npobj) {
			npobj = npobject_create(ident);
		}
		logger_debug("npobj: %p", npobj);
		*(NPObject **)ret_value = npobj;
		break;
	case NPPVpluginWantsAllNetworkStreams:
		buf_uint32_decode(&r, &x32);
		logger_debug("value: %"PRIu32, x32);
		*(uint32_t *)ret_value = x32;
		break;
	default:
		free(msg.ret);
		return NPERR_GENERIC_ERROR;
	}

	free(msg.ret);

	logger_debug("end");
	return error;
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
	logger_debug("error: %s(%d)", np_errorstr(error), error);

	free(msg.ret);

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
	logger_debug("error: %s(%d)", np_errorstr(error), error);

	free(msg.ret);

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
