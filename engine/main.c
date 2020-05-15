#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "npapi.h"
#include "npfunctions.h"

#include "logger.h"
#include "util.h"
#include "rpc.h"
#include "buf.h"
#include "npp.h"
#include "npstream.h"
#include "plugin.h"
#include "browser.h"


static void
print_pluginfuncs(Plugin *p)
{
	logger_debug("size: %u/%zu", p->funcs.size, sizeof(p->funcs));
	logger_debug("version: %d", p->funcs.version);
	logger_debug("newp: %p", p->funcs.newp);
	logger_debug("destroy: %p", p->funcs.destroy);
	logger_debug("setwindow: %p", p->funcs.setwindow);
	logger_debug("newstream: %p", p->funcs.newstream);
	logger_debug("destroystream: %p", p->funcs.destroystream);
	logger_debug("asfile: %p", p->funcs.asfile);
	logger_debug("writeready: %p", p->funcs.writeready);
	logger_debug("write: %p", p->funcs.write);
	logger_debug("print: %p", p->funcs.print);
	logger_debug("event: %p", p->funcs.event);
	logger_debug("urlnotify: %p", p->funcs.urlnotify);
	logger_debug("javaClass: %p", p->funcs.javaClass);
	logger_debug("getvalue: %p", p->funcs.getvalue);
	logger_debug("setvalue: %p", p->funcs.setvalue);
	logger_debug("gotfocus: %p", p->funcs.gotfocus);
	logger_debug("lostfocus: %p", p->funcs.lostfocus);
	logger_debug("urlredirectnotify: %p", p->funcs.urlredirectnotify);
	logger_debug("clearsitedata: %p", p->funcs.clearsitedata);
	logger_debug("getsiteswithdata: %p", p->funcs.getsiteswithdata);
	logger_debug("didComposite: %p", p->funcs.didComposite);
}

static void
NP_Initialize_handle(Plugin *p, Browser *b, RPCSess *sess, RPCMsg *msg)
{
	BufWriter w;
	NPError error;

	logger_debug("start");

	error = p->np_init(&b->funcs, &p->funcs);

	logger_debug("NP_Initialize: %s(%d)", np_errorstr(error), error);

	print_pluginfuncs(p);

	buf_writer_open(&w, 0);
	buf_uint16_encode(&w, error);

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NP_Shutdown_handle(Plugin *p, Browser *b, RPCSess *sess, RPCMsg *msg)
{
	BufWriter w;
	NPError error;

	logger_debug("start");

	error = p->np_shutdown();

	logger_debug("NP_Shutdown: %s(%d)", np_errorstr(error), error);

	buf_writer_open(&w, 0);
	buf_uint16_encode(&w, error);

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NP_GetValue_handle(Plugin *p, Browser *b, RPCSess *sess, RPCMsg *msg)
{
	BufReader r;
	BufWriter w;
	NPPVariable variable;
	NPError error;
	uint32_t len;
	char *s;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);
	buf_uint32_decode(&r, &variable);

	logger_debug("variable: %u", variable);

	switch (variable) {
	case NPPVpluginNameString:
	case NPPVpluginDescriptionString:
		error = p->np_get_value(NULL, variable, (void *)&s);
		logger_debug("error: %d", error);
		logger_debug("ret: '%s'", s);
		len = strlen(s) + 1;
		break;
	default:
		error = NPERR_INVALID_PARAM;
		len = 0;
		s = NULL;
		logger_debug("error: %d", error);
		break;
	}

	buf_writer_open(&w, 0);
	buf_uint16_encode(&w, error);
	if (len > 0) {
		buf_uint32_encode(&w, len);
		buf_bytes_encode(&w, s, len);
	}

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NP_GetMIMEDescription_handle(Plugin *p, Browser *b, RPCSess *sess, RPCMsg *msg)
{
	BufWriter w;
	uint32_t len;

	logger_debug("start");

	logger_debug("ret: '%s'", p->mimedesc);

	if (p->mimedesc) {
		len = strlen(p->mimedesc) + 1;
	} else {
		len = 0;
	}

	buf_writer_open(&w, 0);
	buf_uint32_encode(&w, len);
	if (len)
		buf_bytes_encode(&w, p->mimedesc, len);

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NP_GetPluginVersion_handle(Plugin *p, Browser *b, RPCSess *sess, RPCMsg *msg)
{
	BufWriter w;
	uint32_t len;

	logger_debug("start");

	logger_debug("ret: '%s'", p->version);

	if (p->version) {
		len = strlen(p->version) + 1;
	} else {
		len = 0;
	}

	buf_writer_open(&w, 0);
	buf_uint32_encode(&w, len);
	if (len)
		buf_bytes_encode(&w, p->version, len);

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NPP_New_handle(Plugin *p, Browser *b, RPCSess *sess, RPCMsg *msg)
{
	BufReader r;
	BufWriter w;
	char *pluginType;
	uintptr_t ident;
	NPP npp;
	uint16_t mode;
	uint16_t argc;
	NPSavedData *saved;
	NPSavedData saved_data;
	NPError error;
	uint32_t len;
	uint16_t i;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);

	buf_uint32_decode(&r, &len);
	if (len) {
		pluginType = (char *)buf_bytes_decode(&r, len);
	} else {
		pluginType = NULL;
	}
	logger_debug("pluginType: %s", pluginType);

	buf_uint64_decode(&r, &ident);
	logger_debug("npp_ident: %p", ident);
	npp = npp_create(ident);
	if (npp == NULL) {
		logger_debug("npp_create failed.");
		return;
	}
	logger_debug("npp: %p", npp);

	buf_uint16_decode(&r, &mode);
	logger_debug("mode: %u", mode);

	buf_uint16_decode(&r, &argc);
	logger_debug("argc: %u", argc);

	char *argn[argc];
	char *argv[argc];

	for (i = 0; i < argc; i++) {
		buf_uint32_decode(&r, &len);
		if (len) {
			argn[i] = (char *)buf_bytes_decode(&r, len);
		} else {
			argn[i] = NULL;
		}
		logger_debug("argn[%u]: '%s'", i, argn[i]);
	}

	for (i = 0; i < argc; i++) {
		buf_uint32_decode(&r, &len);
		if (len) {
			argv[i] = (char *)buf_bytes_decode(&r, len);
		} else {
			argv[i] = NULL;
		}
		logger_debug("argv[%u]: '%s'", i, argv[i]);
	}

	if (buf_uint32_decode(&r, &len) == NULL) {
		saved = NULL;
	} else {
		saved = &saved_data;
		saved->len = len;
		saved->buf = (char *)buf_bytes_decode(&r, len);
		logger_dump("saved", saved->buf, saved->len);
	}

	error = p->funcs.newp(pluginType, npp, mode, argc, argn, argv, saved);

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
NPP_Destroy_handle(Plugin *p, Browser *b, RPCSess *sess, RPCMsg *msg)
{
	BufReader r;
	BufWriter w;
	uintptr_t ident;
	NPP npp;
	NPSavedData *saved;
	NPError error;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);

	buf_uint64_decode(&r, &ident);
	logger_debug("npp_ident: %p", ident);
	npp = npp_find(ident);
	if (npp == NULL) {
		logger_debug("npp_find failed.");
		return;
	}
	logger_debug("npp: %p", npp);

	saved = NULL;
	error = p->funcs.destroy(npp, &saved);

	logger_debug("error: %s(%d)", np_errorstr(error), error);

	if (error != NPERR_NO_ERROR)
		npp_destroy(npp);

	buf_writer_open(&w, 0);
	buf_uint16_encode(&w, error);
	if (saved) {
		buf_uint32_encode(&w, saved->len);
		buf_bytes_encode(&w, saved->buf, saved->len);
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
NPP_SetWindow_handle(Plugin *p, Browser *b, RPCSess *sess, RPCMsg *msg)
{
	BufReader r;
	BufWriter w;
	uintptr_t ident;
	NPP npp;
	NPWindow *window;
	NPWindow window_data;
	NPSetWindowCallbackStruct ws_info_data;
	NPError error;
	uint32_t len;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);

	buf_uint64_decode(&r, &ident);
	logger_debug("npp_ident: %p", ident);
	npp = npp_find(ident);
	if (npp == NULL) {
		logger_debug("npp_find failed.");
		return;
	}
	logger_debug("npp: %p", npp);

	buf_uint32_decode(&r, &len);
	if (len == 0) {
		window = NULL;
	} else {
		window = &window_data;
		logger_debug("window {");
		buf_uint64_decode(&r, (uint64_t *)&window->window);
		logger_debug("  window: %p", window->window);
		buf_uint32_decode(&r, &window->x);
		logger_debug("  x: %"PRId32, window->x);
		buf_uint32_decode(&r, &window->y);
		logger_debug("  y: %"PRId32, window->y);
		buf_uint32_decode(&r, &window->width);
		logger_debug("  width: %"PRIu32, window->width);
		buf_uint32_decode(&r, &window->height);
		logger_debug("  height: %"PRIu32, window->height);
		logger_debug("  clipRect {");
		buf_uint16_decode(&r, &window->clipRect.top);
		logger_debug("    top: %"PRIu16, window->clipRect.top);
		buf_uint16_decode(&r, &window->clipRect.left);
		logger_debug("    left: %"PRIu16, window->clipRect.left);
		buf_uint16_decode(&r, &window->clipRect.bottom);
		logger_debug("    bottom: %"PRIu16, window->clipRect.bottom);
		buf_uint16_decode(&r, &window->clipRect.right);
		logger_debug("    right: %"PRIu16, window->clipRect.right);
		logger_debug("  }");
		buf_uint32_decode(&r, &len);
		if (len == 0) {
			window->ws_info = NULL;
			logger_debug("  ws_info: %p", window->ws_info);
		} else {
			VisualID visualid;
			GdkScreen *gdk_screen;
			GdkDisplay *gdk_display;
			GdkVisual *gdk_visual;
			Display *display;

			window->ws_info = &ws_info_data;
			logger_debug("  ws_info: {");

			buf_uint32_decode(&r, &ws_info_data.type);
			logger_debug("    type: %"PRId32, ws_info_data.type);

			buf_uint64_decode(&r, (uint64_t *)&display);
			logger_debug("    display: %p", display);
			if (display == NULL) {
				ws_info_data.display = NULL;
			} else {
				gdk_screen = gdk_screen_get_default();
				gdk_display = gdk_screen_get_display(gdk_screen);
				ws_info_data.display = gdk_x11_display_get_xdisplay(
						gdk_display);

				logger_debug("    => display: %p", ws_info_data.display);
			}

			buf_uint64_decode(&r, &visualid);
			logger_debug("      visualid: %lu", visualid);
			if (display == NULL) {
				ws_info_data.visual = NULL;
			} else {
				if (visualid == 0) {
					gdk_visual = gdk_visual_get_system();
				} else {
					gdk_visual = gdk_x11_screen_lookup_visual(
							gdk_screen, visualid);
				}
				ws_info_data.visual = gdk_x11_visual_get_xvisual(gdk_visual);
			}
			logger_debug("    visual: %p", ws_info_data.visual);

			buf_uint64_decode(&r, &ws_info_data.colormap);
			logger_debug("    colormap: %"PRIx64, ws_info_data.colormap);

			buf_uint32_decode(&r, &ws_info_data.depth);
			logger_debug("    depth: %u", ws_info_data.depth);

			logger_debug("  }");
		}
		buf_uint32_decode(&r, &window->type);
		logger_debug("  type: %"PRIu32, window->type);
		logger_debug("}");
	}

	error = p->funcs.setwindow(npp, window);

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
NPP_GetValue_handle(Plugin *p, Browser *b, RPCSess *sess, RPCMsg *msg)
{
	BufReader r;
	BufWriter w;
	uintptr_t ident;
	NPP npp;
	NPPVariable variable;
	NPError error;
	NPObject *npobj;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);

	buf_uint64_decode(&r, &ident);
	logger_debug("npp_ident: %p", ident);
	npp = npp_find(ident);
	if (npp == NULL) {
		logger_debug("npp_find failed.");
		return;
	}
	logger_debug("npp: %p", npp);

	buf_uint32_decode(&r, &variable);
	logger_debug("variable: %s(%u)", npp_varstr(variable), variable);

	buf_writer_open(&w, 0);

	switch (variable) {
	case NPPVpluginScriptableNPObject:
		error = p->funcs.getvalue(npp, variable, (void *)&npobj);
		logger_debug("error: %s(%d)", np_errorstr(error), error);
		buf_uint16_encode(&w, error);
		if (error == NPERR_NO_ERROR) {
			logger_debug("npobj: %p", npobj);
			buf_uint64_encode(&w, (uintptr_t)npobj);
		}
		break;
	default:
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
NPP_NewStream_handle(Plugin *p, Browser *b, RPCSess *sess, RPCMsg *msg)
{
	BufReader r;
	BufWriter w;
	uintptr_t ident;
	NPP npp;
	char *type;
	uintptr_t pdata;
	uintptr_t ndata;
	NPStream *stream;
	NPBool seekable;
	uint16_t stype;
	NPError error;
	uint32_t len;
	const char *s;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);

	buf_uint64_decode(&r, &ident);
	logger_debug("npp_ident: %p", ident);
	npp = npp_find(ident);
	if (npp == NULL) {
		logger_debug("npp_find failed.");
		return;
	}
	logger_debug("npp: %p", npp);

	buf_uint32_decode(&r, &len);
	if (len == 0) {
		type = NULL;
	} else {
		type = (char *)buf_bytes_decode(&r, len);
	}
	logger_debug("type: '%s'", type);

	logger_debug("stream {");
	buf_uint64_decode(&r, (uint64_t *)&pdata);
	logger_debug("  pdata: %p", pdata);
	buf_uint64_decode(&r, (uint64_t *)&ndata);
	logger_debug("  ndata: %p", ndata);

	stream = npstream_create(ndata);
	if (stream == NULL) {
		logger_debug("npstream_create failed.");
		return;
	}
	logger_debug("stream: %p", stream);
	stream->ndata = (void *)ndata;

	buf_uint32_decode(&r, &len);
	if (len == 0) {
		stream->url = NULL;
	} else {
		s = buf_bytes_decode(&r, len);
		stream->url = strdup(s);
	}
	logger_debug("  url: '%s'", stream->url);

	buf_uint32_decode(&r, &stream->end);
	logger_debug("  end: %"PRIu32, stream->end);

	buf_uint32_decode(&r, &stream->lastmodified);
	logger_debug("  lastmodified: %"PRIu32, stream->lastmodified);

	buf_uint64_decode(&r, (uint64_t *)&stream->notifyData);
	logger_debug("  notifyData: %p", stream->notifyData);

	buf_uint32_decode(&r, &len);
	if (len == 0) {
		stream->headers = NULL;
	} else {
		s = buf_bytes_decode(&r, len);
		stream->headers = strdup(s);
	}
	logger_debug("  headers: '%s'", stream->headers);
	logger_debug("}");

	buf_uint8_decode(&r, &seekable);
	logger_debug("seekable: %u", seekable);

	error = p->funcs.newstream(npp, type, stream, seekable, &stype);

	logger_debug("error: %s(%d)", np_errorstr(error), error);
	logger_debug("pdata: %p", stream->pdata);
	logger_debug("notifyData: %p", stream->notifyData);
	logger_debug("stype: %u", stype);

	buf_writer_open(&w, 0);
	buf_uint16_encode(&w, error);
	buf_uint64_encode(&w, (uintptr_t)stream->pdata);
	buf_uint64_encode(&w, (uintptr_t)stream->notifyData);
	buf_uint16_encode(&w, stype);

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NPP_DestroyStream_handle(Plugin *p, Browser *b, RPCSess *sess, RPCMsg *msg)
{
	BufReader r;
	BufWriter w;
	uintptr_t ident;
	NPP npp;
	uintptr_t pdata;
	uintptr_t ndata;
	NPStream *stream;
	NPReason reason;
	NPError error;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);

	buf_uint64_decode(&r, &ident);
	logger_debug("npp_ident: %p", ident);
	npp = npp_find(ident);
	if (npp == NULL) {
		logger_debug("npp_find failed.");
		return;
	}
	logger_debug("npp: %p", npp);

	logger_debug("stream {");
	buf_uint64_decode(&r, (uint64_t *)&pdata);
	logger_debug("  pdata: %p", pdata);
	buf_uint64_decode(&r, (uint64_t *)&ndata);
	logger_debug("  ndata: %p", ndata);

	stream = npstream_find(ndata);
	if (stream == NULL) {
		logger_debug("npstream_find failed.");
		return;
	}
	logger_debug("stream: %p", stream);

	buf_uint16_decode(&r, &reason);
	logger_debug("reason: %u", reason);

	error = p->funcs.destroystream(npp, stream, reason);

	logger_debug("error: %s(%d)", np_errorstr(error), error);

	if (error != NPERR_NO_ERROR)
		npstream_destroy(stream);

	buf_writer_open(&w, 0);
	buf_uint16_encode(&w, error);

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NPP_WriteReady_handle(Plugin *p, Browser *b, RPCSess *sess, RPCMsg *msg)
{
	BufReader r;
	BufWriter w;
	uintptr_t ident;
	NPP npp;
	uintptr_t pdata;
	uintptr_t ndata;
	NPStream *stream;
	int32_t ret;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);

	buf_uint64_decode(&r, &ident);
	logger_debug("npp_ident: %p", ident);
	npp = npp_find(ident);
	if (npp == NULL) {
		logger_debug("npp_find failed.");
		return;
	}
	logger_debug("npp: %p", npp);

	logger_debug("stream {");
	buf_uint64_decode(&r, (uint64_t *)&pdata);
	logger_debug("  pdata: %p", pdata);
	buf_uint64_decode(&r, (uint64_t *)&ndata);
	logger_debug("  ndata: %p", ndata);

	stream = npstream_find(ndata);
	if (stream == NULL) {
		logger_debug("npstream_find failed.");
		return;
	}
	logger_debug("stream: %p", stream);

	ret = p->funcs.writeready(npp, stream);

	logger_debug("ret: %"PRId32, ret);

	buf_writer_open(&w, 0);
	buf_uint32_encode(&w, ret);

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}

static void
NPP_Write_handle(Plugin *p, Browser *b, RPCSess *sess, RPCMsg *msg)
{
	BufReader r;
	BufWriter w;
	uintptr_t ident;
	NPP npp;
	uintptr_t pdata;
	uintptr_t ndata;
	NPStream *stream;
	int32_t offset;
	void *buffer;
	int32_t len;
	int32_t ret;

	logger_debug("start");

	buf_reader_init(&r, msg->param, msg->param_size);

	buf_uint64_decode(&r, &ident);
	logger_debug("npp_ident: %p", ident);
	npp = npp_find(ident);
	if (npp == NULL) {
		logger_debug("npp_find failed.");
		return;
	}
	logger_debug("npp: %p", npp);

	logger_debug("stream {");
	buf_uint64_decode(&r, (uint64_t *)&pdata);
	logger_debug("  pdata: %p", pdata);
	buf_uint64_decode(&r, (uint64_t *)&ndata);
	logger_debug("  ndata: %p", ndata);

	stream = npstream_find(ndata);
	if (stream == NULL) {
		logger_debug("npstream_find failed.");
		return;
	}
	logger_debug("stream: %p", stream);

	buf_uint32_decode(&r, &offset);
	logger_debug("offset: %"PRId32, offset);

	buf_uint32_decode(&r, &len);
	logger_debug("len: %"PRId32, len);
	buffer = (void *)buf_bytes_decode(&r, len);
	//logger_dump("buffer", buffer, len);

	ret = p->funcs.write(npp, stream, offset, len, buffer);

	logger_debug("ret: %"PRId32, ret);

	buf_writer_open(&w, 0);
	buf_uint32_encode(&w, ret);

	msg->ret = w.data;
	msg->ret_size = w.len;

	rpc_return(sess, msg);

	buf_writer_close(&w);

	logger_debug("end");
}


static void
np_dispatch(RPCSess *sess, RPCMsg *msg, void *ctxt)
{
	NPP npp = ctxt;
	Plugin *p = npp->pdata;
	Browser *b = npp->ndata;

	switch (msg->method) {
	case RPC_NP_Initialize:
		NP_Initialize_handle(p, b, sess, msg);
		break;
	case RPC_NP_Shutdown:
		NP_Shutdown_handle(p, b, sess, msg);
		break;
	case RPC_NP_GetValue:
		NP_GetValue_handle(p, b, sess, msg);
		break;
	case RPC_NP_GetMIMEDescription:
		NP_GetMIMEDescription_handle(p, b, sess, msg);
		break;
	case RPC_NP_GetPluginVersion:
		NP_GetPluginVersion_handle(p, b, sess, msg);
		break;
	case RPC_NPP_New:
		NPP_New_handle(p, b, sess, msg);
		break;
	case RPC_NPP_Destroy:
		NPP_Destroy_handle(p, b, sess, msg);
		break;
	case RPC_NPP_SetWindow:
		NPP_SetWindow_handle(p, b, sess, msg);
		break;
	case RPC_NPP_GetValue:
		NPP_GetValue_handle(p, b, sess, msg);
		break;
	case RPC_NPP_NewStream:
		NPP_NewStream_handle(p, b, sess, msg);
		break;
	case RPC_NPP_DestroyStream:
		NPP_DestroyStream_handle(p, b, sess, msg);
		break;
	case RPC_NPP_WriteReady:
		NPP_WriteReady_handle(p, b, sess, msg);
		break;
	case RPC_NPP_Write:
		NPP_Write_handle(p, b, sess, msg);
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
		close(sess->fd);
		gtk_main_quit();
		return false;
	}

	g_io_add_watch(source, G_IO_IN | G_IO_HUP, io_handler, data);

	logger_debug("end");

	return true;
}

int
main(int argc, char **argv)
{
	const char *path;
	char *s;
	NPP_t np;
	Plugin p;
	Browser b;
	RPCSess sess;
	GIOChannel *channel;
	int opt;
	int fd;

	path = "/usr/local/lib/browser_plugins/linux-flashplayer/libflashplayer.so";
	fd = 0;
	logger_fp = stdout;

	while ((opt = getopt(argc, argv, "p:s:v")) != -1) {
		switch (opt) {
		case 'p':
			path = optarg;
			break;
		case 's':
			fd = strtol(optarg, NULL, 0);
			break;
		case 'v':
			logger_fp = NULL;
			break;
		default:
			return 1;
		}
	}

	if (plugin_open(&p, path) != 0)
		return 1;

	logger_debug("Plugin version: %s", p.version);
	logger_debug("MIME: %s", p.mimedesc);

	s = NULL;
	p.np_get_value(NULL, NPPVpluginNameString, (void *)&s);
	logger_debug("Plugin name: %s", s);

	s = NULL;
	p.np_get_value(NULL, NPPVpluginDescriptionString, (void *)&s);
	logger_debug("Plugin desc: %s", s);

	np.ndata = &b;
	np.pdata = &p;

	rpcsess_init(&sess, fd, np_dispatch, &np);

	if (browser_init(&b, &sess) != 0) {
		plugin_close(&p);
		return 1;
	}

	channel = g_io_channel_unix_new(fd);
	if (channel == NULL) {
		plugin_close(&p);
		return 1;
	}
	g_io_channel_set_encoding(channel, NULL, NULL);
	g_io_add_watch(channel, G_IO_IN | G_IO_HUP, io_handler, &sess);

	gtk_main();

	plugin_close(&p);

	return 0;
}
