#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <gtk/gtk.h>

#include "npapi.h"
#include "npfunctions.h"

#include "logger.h"
#include "util.h"
#include "npp.h"
#include "buf.h"
#include "rpc.h"
#include "browser.h"


static Browser *g_browser = NULL;
static NPClass npobj_class = {};


static NPError
b_geturl(NPP npp, const char *relativeURL, const char *target)
{
	logger_debug("start");
	return NPERR_NO_ERROR;
}

static NPError
b_posturl(NPP npp, const char *relativeURL, const char *target,
		uint32_t len, const char *buf, NPBool file)
{
	logger_debug("start");
	return NPERR_NO_ERROR;
}

static NPError
b_requestread(NPStream *pstream, NPByteRange *rangeList)
{
	logger_debug("start");
	return NPERR_NO_ERROR;
}

static NPError
b_newstream(NPP npp, NPMIMEType type, const char *target,
		NPStream **result)
{
	logger_debug("start");
	return NPERR_NO_ERROR;
}

static int32_t
b_write(NPP npp, NPStream *pstream, int32_t len, void *buffer)
{
	logger_debug("start");
	return 0;
}

static NPError
b_destroystream(NPP npp, NPStream *pstream, NPError reason)
{
	logger_debug("start");
	return NPERR_NO_ERROR;
}

static void
b_status(NPP npp, const char *message)
{
	logger_debug("start");
}

static const char *
b_uagent(NPP npp)
{
	Browser *b = g_browser;
	RPCMsg msg = {
		.method = RPC_NPN_UserAgent,
	};
	BufWriter w;
	BufReader r;
	uint32_t len;
	static char *s = NULL;

	logger_debug("start");
	logger_debug("npp: %p", npp);

	if (s)
		return s;

	buf_writer_open(&w, 0);
	buf_uint64_encode(&w, npp_ident(npp));

	msg.param = w.data;
	msg.param_size = w.len;

	rpc_invoke(b->sess, &msg);

	buf_writer_close(&w);

	buf_reader_init(&r, msg.ret, msg.ret_size);
	buf_uint32_decode(&r, &len);
	if (len) {
		s = strdup(buf_bytes_decode(&r, len));
	} else {
		s = NULL;
	}

	free(msg.ret);

	return s;
}

static void *
b_memalloc(uint32_t size)
{
	logger_debug("start(%"PRIu32")", size);
	return malloc(size);
}

static void
b_memfree(void *ptr)
{
	logger_debug("start(%p)", ptr);
	free(ptr);
}

static uint32_t
b_memflush(uint32_t size)
{
	logger_debug("start(%"PRIu32")", size);
	return 0;
}

static void
b_reloadplugins(NPBool reloadPages)
{
	logger_debug("start(%u)", reloadPages);
}

static void *
b_getJavaEnv(void)
{
	logger_debug("start");
	return NULL;
}

static void *
b_getJavaPeer(NPP npp)
{
	logger_debug("start");
	return NULL;
}

static NPError
b_geturlnotify(NPP npp, const char *relativeURL, const char *target,
		void *notifyData)
{
	logger_debug("start");
	return NPERR_NO_ERROR;
}

static NPError
b_posturlnotify(NPP npp, const char *relativeURL, const char *target,
		uint32_t len, const char *buf, NPBool file, void *notifyData)
{
	logger_debug("start");
	return NPERR_NO_ERROR;
}

static NPError
b_getvalue(NPP npp, NPNVariable variable, void *result)
{
	Browser *b = g_browser;
	RPCMsg msg = {
		.method = RPC_NPN_GetValue,
	};
	BufWriter w;
	BufReader r;
	NPError error;
	uint32_t len;
	const char *s;

	logger_debug("start");
	logger_debug("npp: %p", npp);
	logger_debug("variable: %s(%d)", npn_varstr(variable), variable);

	buf_writer_open(&w, 0);
	buf_uint64_encode(&w, npp_ident(npp));
	buf_uint32_encode(&w, variable);

	msg.param = w.data;
	msg.param_size = w.len;

	rpc_invoke(b->sess, &msg);

	buf_writer_close(&w);

	buf_reader_init(&r, msg.ret, msg.ret_size);
	buf_uint16_decode(&r, &error);

	logger_debug("error: %s(%d)", np_errorstr(error), error);

	if (error != NPERR_NO_ERROR) {
		free(msg.ret);
		return error;
	}

	switch (variable) {
	case NPNVxDisplay:
	case NPNVxtAppContext:
	case NPNVnetscapeWindow:
	case NPNVjavascriptEnabledBool:
	case NPNVasdEnabledBool:
	case NPNVisOfflineBool:
	case NPNVserviceManager:
	case NPNVDOMElement:
	case NPNVDOMWindow:
		break;
	case NPNVToolkit:
		//*((NPNToolkitType *)result) = NPNVGtk2;
		buf_uint32_decode(&r, result);
		logger_debug("result: %u", *(NPNToolkitType *)result);
		break;
	case NPNVSupportsXEmbedBool:
		/* falseだと失敗する */
		//*(NPBool *)result = true;
		buf_uint8_decode(&r, result);
		logger_debug("result: %u", *(NPBool *)result);
		break;
	case NPNVWindowNPObject:
		/* Get the NPObject wrapper for the browser window. */
		/* TODO */
		//*(NPObject **)result = &npobj_window;
		buf_uint64_decode(&r, result);
		logger_debug("result: %p", *(NPObject **)result);
		break;
	case NPNVPluginElementNPObject:
		/* Get the NPObject wrapper for the plugins DOM element. */
		buf_uint64_decode(&r, result);
		logger_debug("result: %p", *(NPObject **)result);
		break;
	case NPNVSupportsWindowless:
		//*(NPBool *)result = true;
		buf_uint8_decode(&r, result);
		logger_debug("result: %u", *(NPBool *)result);
		break;
	case NPNVprivateModeBool:
		//*(NPBool *)result = false;
		buf_uint8_decode(&r, result);
		logger_debug("result: %u", *(NPBool *)result);
		break;
        case NPNVsupportsAdvancedKeyHandling:
		break;
        case NPNVdocumentOrigin:
		//*(char **)result = strdup("http://yahoo-mbga.jp");
		buf_uint32_decode(&r, &len);
		if (len) {
			s = buf_bytes_decode(&r, len);
			*(char **)result = strdup(s);
		} else {
			*(char **)result = NULL;
		}
		logger_debug("result: %p'%s'", *(char **)result, *(char **)result);
		break;
        case NPNVCSSZoomFactor:
        case NPNVpluginDrawingModel:
        case NPNVsupportsAsyncBitmapSurfaceBool:
                break;
        case NPNVmuteAudioBool:
                //*(NPBool *)result = false;
		buf_uint8_decode(&r, result);
		logger_debug("result: %u", *(NPBool *)result);
                break;
        default:
		logger_debug("unknown variable %u", variable);
		free(msg.ret);
		return NPERR_GENERIC_ERROR;
	}

	free(msg.ret);

	logger_debug("end");

	return NPERR_NO_ERROR;
}

static NPError
b_setvalue(NPP npp, NPPVariable variable, void *result)
{
	Browser *b = g_browser;
	RPCMsg msg = {
		.method = RPC_NPN_SetValue,
	};
	BufWriter w;
	BufReader r;
	NPError error;

	logger_debug("start");
	logger_debug("npp: %p", npp);
	logger_debug("variable: %s(%d)", npp_varstr(variable), variable);

	buf_writer_open(&w, 0);
	buf_uint64_encode(&w, npp_ident(npp));
	buf_uint32_encode(&w, variable);

	switch (variable) {
	case NPPVpluginWindowBool:
	case NPPVpluginTransparentBool:
		logger_debug("result: %u", result ? 1 : 0);
		buf_uint8_encode(&w, result ? 1 : 0);
		break;
	default:
		logger_debug("unknown variable: %d", variable);
		buf_writer_close(&w);
		return NPERR_GENERIC_ERROR;
	}

	msg.param = w.data;
	msg.param_size = w.len;

	rpc_invoke(b->sess, &msg);

	buf_writer_close(&w);

	buf_reader_init(&r, msg.ret, msg.ret_size);
	buf_uint16_decode(&r, &error);

	logger_debug("end(%s(%d))", np_errorstr(error), error);

	return error;
}

static void
b_invalidaterect(NPP npp, NPRect *invalidRect)
{
	logger_debug("start");
}

static void
b_invalidateregion(NPP npp, NPRegion invalidRegion)
{
	logger_debug("start");
}

static void
b_forceredraw(NPP npp)
{
	logger_debug("start");
}

static NPIdentifier
b_getstringidentifier(const NPUTF8 *name)
{
	NPIdentifier ident;

	logger_debug("start(%s)", name);

	if (strcmp(name, "location") == 0)
		ident = (NPIdentifier)0x2;
	else if (strcmp(name, "href") == 0)
		ident = (NPIdentifier)0x3;
	else if (strcmp(name, "top") == 0)
		ident = (NPIdentifier)0x4;
	else
		ident = NULL;

	logger_debug("end(%p)", ident);

	return ident;
}

static void
b_getstringidentifiers(const NPUTF8** names, int32_t nameCount,
		NPIdentifier *identifiers)
{
	logger_debug("start");
}

static NPIdentifier
b_getintidentifier(int32_t intid)
{
	logger_debug("start");
	return 0;
}

static bool
b_identifierisstring(NPIdentifier id)
{
	logger_debug("start");
	return false;
}

static NPUTF8 *
b_utf8fromidentifier(NPIdentifier id)
{
	logger_debug("start");
	return NULL;
}

static int32_t
b_intfromidentifier(NPIdentifier id)
{
	logger_debug("start");
	return 0;
}

static NPObject *
b_createobject(NPP npp, NPClass *aClass)
{
	NPObject *npobj;

	logger_debug("start");
	logger_debug("npp: %p", npp);
	logger_debug("aClass: %p", aClass);

	if (aClass->allocate) {
		npobj = aClass->allocate(npp, aClass);
	} else {
		npobj = malloc(sizeof(NPObject));
	}

	if (npobj) {
		npobj->_class = aClass;
		npobj->referenceCount = 1;
	}

	logger_debug("end(%p)", npobj);

	return npobj;
}

static NPObject *
b_retainobject(NPObject *npobj)
{
	logger_debug("start(%p)", npobj);

	if (!npobj)
		return npobj;

	npobj->referenceCount++;

	logger_debug("end(%p)", npobj);

	return npobj;
}

static void
b_releaseobject(NPObject *npobj)
{
	logger_debug("start(%p)", npobj);

	if (npobj)
		return;

	{
		Browser *b = g_browser;
		RPCMsg msg = {
			.method = RPC_NPN_ReleaseObject,
		};
		BufWriter w;

		buf_writer_open(&w, 0);
		buf_uint64_encode(&w, (uintptr_t)npobj);

		msg.param = w.data;
		msg.param_size = w.len;

		rpc_invoke(b->sess, &msg);

		buf_writer_close(&w);
		return;
	}

	npobj->referenceCount--;
	if (npobj->referenceCount > 0)
		return;

	if (npobj->_class && npobj->_class->deallocate) {
		npobj->_class->deallocate(npobj);
	} else {
		free(npobj);
	}

	logger_debug("end");
}

static bool
b_invoke(NPP npp, NPObject* npobj, NPIdentifier method,
		const NPVariant *args,
		uint32_t argCount, NPVariant *result)
{
	logger_debug("start(%p)", npobj);
	return false;
}

static bool
b_invokeDefault(NPP npp, NPObject *npobj, const NPVariant *args,
		uint32_t argCount, NPVariant *result)
{
	logger_debug("start(%p)", npobj);
	return false;
}

static bool
b_evaluate(NPP npp, NPObject *npobj, NPString *script,
		NPVariant *result)
{
	logger_debug("start(%p)", npobj);
	return false;
}

static NPObject npobj_location = {
	._class = &npobj_class,
	.referenceCount = 1
};

static NPObject npobj_top = {
	._class = &npobj_class,
	.referenceCount = 1
};


static bool
b_getproperty(NPP npp, NPObject *npobj, NPIdentifier property,
		NPVariant *result)
{
	logger_debug("start");
	logger_debug("npp: :%p", npp);
	logger_debug("npobj: :%p", npobj);
	logger_debug("property: :%p", property);

	switch ((uintptr_t)property) {
	case 0x2:
		result->type = NPVariantType_Object;
		/* TODO */
		result->value.objectValue = &npobj_location;
		logger_debug("object: :%p", result->value.objectValue);
		return true;
	case 0x3:
		result->type = NPVariantType_String;
		result->value.stringValue.UTF8Characters = "http://yahoo-mbga.jp/stdgame/300003?member_id=&invite_member=&appParams=&code=";
		result->value.stringValue.UTF8Length = strlen(
				result->value.stringValue.UTF8Characters);
		logger_debug("string: :%p", result->value.stringValue.UTF8Characters);
		return true;
	case 0x4:
		result->type = NPVariantType_Object;
		/* TODO */
		result->value.objectValue = &npobj_top;
		logger_debug("object: :%p", result->value.objectValue);
		return true;
	}

	return false;
}

static bool
b_setproperty(NPP npp, NPObject *npobj, NPIdentifier property,
		const NPVariant *value)
{
	logger_debug("start");
	return false;
}

static bool
b_removeproperty(NPP npp, NPObject *npobj, NPIdentifier property)
{
	logger_debug("start");
	return false;
}

static bool
b_hasproperty(NPP npp, NPObject *npobj, NPIdentifier propertyName)
{
	logger_debug("start");
	return false;
}

static bool
b_hasmethod(NPP npp, NPObject *npobj, NPIdentifier methodName)
{
	logger_debug("start");
	return false;
}

static void
b_releasevariantvalue(NPVariant *variant)
{
	logger_debug("start(%p)", variant);
}

static void
b_setexception(NPObject *npobj, const NPUTF8 *message)
{
	logger_debug("start");
}

static void
b_pushpopupsenabledstate(NPP npp, NPBool enabled)
{
	logger_debug("start");
}

static void
b_poppopupsenabledstate(NPP npp)
{
	logger_debug("start");
}

static bool
b_enumerate(NPP npp, NPObject *npobj, NPIdentifier **identifier,
		uint32_t *count)
{
	logger_debug("start");
	return false;
}

static void
b_pluginthreadasynccall(NPP instance, void (*func)(void *),
		void *userData)
{
	logger_debug("start");
}

static bool
b_construct(NPP npp, NPObject* npobj, const NPVariant *args,
		uint32_t argCount, NPVariant *result)
{
	logger_debug("start");
	return false;
}

static NPError
b_getvalueforurl(NPP instance, NPNURLVariable variable,
		const char *url, char **value, uint32_t *len)
{
	logger_debug("start");
	return NPERR_NO_ERROR;
}

static NPError
b_setvalueforurl(NPP instance, NPNURLVariable variable,
		const char *url, const char *value, uint32_t len)
{
	logger_debug("start");
	return NPERR_NO_ERROR;
}

static NPError
b_getauthenticationinfo(NPP instance, const char *protocol, const char *host,
		int32_t port, const char *scheme, const char *realm,
		char **username, uint32_t *ulen, char **password,
		uint32_t *plen)
{
	logger_debug("start");
	return NPERR_NO_ERROR;
}

static uint32_t
b_scheduletimer(NPP instance, uint32_t interval,
		NPBool repeat, void (*timerFunc)(NPP, uint32_t))
{
	logger_debug("start");
	return 0;
}

static void
b_unscheduletimer(NPP instance, uint32_t timerID)
{
	logger_debug("start");
}

static NPError
b_popupcontextmenu(NPP instance, NPMenu* menu)
{
	logger_debug("start");
	return NPERR_NO_ERROR;
}

static NPBool
b_convertpoint(NPP instance, double sourceX, double sourceY,
		NPCoordinateSpace sourceSpace,
		double *destX, double *destY,
		NPCoordinateSpace destSpace)
{
	logger_debug("start");
	return false;
}

/* handleevent */
/* unfocusinstance */

static void
b_urlredirectresponse(NPP instance, void* notifyData, NPBool allow)
{
	logger_debug("start");
}

static NPError
b_initasyncsurface(NPP instance, NPSize *size, NPImageFormat format,
		void *initData, NPAsyncSurface *surface)
{
	logger_debug("start");
	return NPERR_NO_ERROR;
}

static NPError
b_finalizeasyncsurface(NPP instance, NPAsyncSurface *surface)
{
	logger_debug("start");
	return NPERR_NO_ERROR;
}

static void
b_setcurrentasyncsurface(NPP instance, NPAsyncSurface *surface,
		NPRect *changed)
{
	logger_debug("start");
}

#if 0
#include <X11/Intrinsic.h>

static Display *x_display;
static XtAppContext x_app_context;
#endif

int
browser_init(Browser *b, RPCSess *sess)
{
#if 0
	int argc = 1;
	char *argv[] = {
		"np_engine",
		NULL
	};
	XtToolkitInitialize();
	x_app_context = XtCreateApplicationContext();
	x_display = XtOpenDisplay(x_app_context, NULL,
		       	"np_engine", "np_engine", NULL, 0, &argc, argv);
#endif
	gtk_init(NULL, NULL);
#if 0
	b->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	//b->window = NULL;
	gtk_widget_set_size_request(b->window, 760, 680);
	//gtk_widget_show(b->window);
#endif
	g_browser = b;
	logger_debug("browser: %p", b);

	b->sess = sess;

	memset(&b->funcs, 0, sizeof(b->funcs));
	b->funcs.version = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
	b->funcs.size = sizeof(b->funcs);

	b->funcs.geturl = b_geturl;
	b->funcs.posturl = b_posturl;
	b->funcs.requestread = b_requestread;
	//b->funcs.newstream = b_newstream;
	//b->funcs.write = b_write;
	//b->funcs.destroystream = b_destroystream;
	b->funcs.status = b_status;
	b->funcs.uagent = b_uagent;
	b->funcs.memalloc = b_memalloc;
	b->funcs.memfree = b_memfree;
	b->funcs.memflush = b_memflush;
	b->funcs.reloadplugins = b_reloadplugins;
	b->funcs.getJavaEnv = b_getJavaEnv;
	b->funcs.getJavaPeer = b_getJavaPeer;
	b->funcs.geturlnotify = b_geturlnotify;
	b->funcs.posturlnotify = b_posturlnotify;
	b->funcs.getvalue = b_getvalue;
	b->funcs.setvalue = b_setvalue;
	b->funcs.invalidaterect = b_invalidaterect;
	b->funcs.invalidateregion = b_invalidateregion;
	b->funcs.forceredraw = b_forceredraw;
	b->funcs.getstringidentifier = b_getstringidentifier;
	b->funcs.getstringidentifiers = b_getstringidentifiers;
	b->funcs.getintidentifier = b_getintidentifier;
	b->funcs.identifierisstring = b_identifierisstring;
	b->funcs.utf8fromidentifier = b_utf8fromidentifier;
	b->funcs.intfromidentifier = b_intfromidentifier;
	b->funcs.createobject = b_createobject;
	b->funcs.retainobject = b_retainobject;
	b->funcs.releaseobject = b_releaseobject;
	b->funcs.invoke = b_invoke;
	b->funcs.invokeDefault = b_invokeDefault;
	b->funcs.evaluate = b_evaluate;
	b->funcs.getproperty = b_getproperty;
	b->funcs.setproperty = b_setproperty;
	b->funcs.removeproperty = b_removeproperty;
	b->funcs.hasproperty = b_hasproperty;
	b->funcs.hasmethod = b_hasmethod;
	b->funcs.releasevariantvalue = b_releasevariantvalue;
	b->funcs.setexception = b_setexception;
	b->funcs.pushpopupsenabledstate = b_pushpopupsenabledstate;
	b->funcs.poppopupsenabledstate = b_poppopupsenabledstate;
	b->funcs.enumerate = b_enumerate;
	b->funcs.pluginthreadasynccall = b_pluginthreadasynccall;
	b->funcs.construct = b_construct;
	b->funcs.getvalueforurl = b_getvalueforurl;
	b->funcs.setvalueforurl = b_setvalueforurl;
	//b->funcs.getauthenticationinfo = b_getauthenticationinfo;
	b->funcs.scheduletimer = b_scheduletimer;
	b->funcs.unscheduletimer = b_unscheduletimer;
	//b->funcs.popupcontextmenu = b_popupcontextmenu;
	//b->funcs.convertpoint = b_convertpoint;
	//b->funcs.handleevent = NULL;
	//b->funcs.unfocusinstance = NULL;
	//b->funcs.urlredirectresponse = b_urlredirectresponse;
	//b->funcs.initasyncsurface = b_initasyncsurface;
	//b->funcs.finalizeasyncsurface = b_finalizeasyncsurface;
	//b->funcs.setcurrentasyncsurface = b_setcurrentasyncsurface;

	return 0;
}
