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
#include "npp.h"
#include "rpc.h"
#include "buf.h"
#include "plugin.h"
#include "browser.h"


static void
test_create(Plugin *p, Browser *b)
{
	NPError error;
	NPMIMEType pluginType = "application/x-shockwave-flash";
	NPP_t np;
	uint16_t mode = 1;
	uint16_t argc = 19;
	char *argn[] = {
		"type",
		"id",
		"data",
		"class",
		"width",
		"height",
		"SRC",
		"PARAM",
		"menu",
		"scale",
		"allowFullscreen",
		"allowScriptAccess",
		"bgcolor",
		"base",
		"flashvars",
		"allowscriptaccess",
		"allowfullscreen",
		"allownetworking",
		"wmode"
	};
	char *argv[] = {
		"application/x-shockwave-flash",
		"Main",
		"http://static-a.yahoo-mbga.jp/static/img/stdgame/300003/swf/main.swf?1343114848&ts=201503191400",
		"flashfirebug",
		"760",
		"680",
		"http://static-a.yahoo-mbga.jp/static/img/stdgame/300003/swf/main.swf?1343114848&ts=201503191400",
		NULL,
		"false",
		"noScale",
		"true",
		"always",
		"#FFFFFF",
		"http://static-a.yahoo-mbga.jp/static/img/stdgame/300003/swf/?1343114848",
		"ts=201503191400&httpApiHost=http://yahoo-mbga.jp",
		"always",
		"true",
		"all",
		"opaque"
	};

	np.ndata = b;
	np.pdata = NULL;

	logger_debug("start");

	error = p->funcs.newp(pluginType, &np, mode, argc, argn, argv, NULL);
	if (error != NPERR_NO_ERROR) {
		logger_debug("newp %s", np_errorstr(error));
		return;
	}

	error = p->funcs.destroy(&np, NULL);
	if (error != NPERR_NO_ERROR) {
		logger_debug("destroy %s", np_errorstr(error));
		return;
	}

	logger_debug("end");
}

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
		pluginType = buf_bytes_decode(&r, len);
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
			argn[i] = buf_bytes_decode(&r, len);
		} else {
			argn[i] = NULL;
		}
		logger_debug("argn[%u]: '%s'", i, argn[i]);
	}

	for (i = 0; i < argc; i++) {
		buf_uint32_decode(&r, &len);
		if (len) {
			argv[i] = buf_bytes_decode(&r, len);
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
		saved->buf = buf_bytes_decode(&r, len);
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
