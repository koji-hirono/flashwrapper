#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>

#include "logger.h"
#include "rpc.h"


static int
rpc_send(RPCSess *sess, const void *data, size_t size)
{
	const char *p = data;
	int r;

	do {
		r = write(sess->fd, p, size);
		if (r == -1) {
			logger_debug("write failed. %s", strerror(errno));
			return -1;
		}
		size -= r;
		p += r;
	} while (size > 0);

	return 0;
}

static int
rpc_recv(RPCSess *sess, void *data, size_t size)
{
	char *p = data;
	int r;

	do {
		r = read(sess->fd, p, size);
		if (r == -1) {
			logger_debug("read failed. %s", strerror(errno));
			return -1;
		}
		if (r == 0) {
			logger_debug("read EOF");
			return -1;
		}
		size -= r;
		p += r;
	} while (size > 0);

	return 0;
}


int
rpcmsghdr_encode(RPCMsgHdr *hdr, void *buf, size_t size)
{
	unsigned char *p = buf;
	uint32_t x;

	if (size < RPCMsgHdrSize)
		return -1;

	x = (hdr->type << 31) | hdr->method;
	*p++ = (x >> 24) & 0xff;
	*p++ = (x >> 16) & 0xff;
	*p++ = (x >> 8) & 0xff;
	*p++ = x & 0xff;

	x = hdr->size;
	*p++ = (x >> 24) & 0xff;
	*p++ = (x >> 16) & 0xff;
	*p++ = (x >> 8) & 0xff;
	*p++ = x & 0xff;

	return 0;
}

int
rpcmsghdr_decode(RPCMsgHdr *hdr, const void *buf, size_t size)
{
	const unsigned char *p = buf;
	uint32_t x;

	if (size < RPCMsgHdrSize)
		return -1;
	x = *p++;
	x <<= 8;
	x |= *p++;
	x <<= 8;
	x |= *p++;
	x <<= 8;
	x |= *p++;

	hdr->type = (x >> 31) & 0x1;
	hdr->method = x & 0x7fffffff;

	x = *p++;
	x <<= 8;
	x |= *p++;
	x <<= 8;
	x |= *p++;
	x <<= 8;
	x |= *p++;

	hdr->size = x;

	return 0;
}

void
rpcsess_init(RPCSess *sess, int fd, void (*dispatch)(RPCSess *, RPCMsg *, void *),
		void *ctxt)
{
	sess->fd = fd;
	sess->dispatch = dispatch;
	sess->ctxt = ctxt;
}

int
rpc_invoke(RPCSess *sess, RPCMsg *msg)
{
	char buf[RPCMsgHdrSize];
	RPCMsgHdr hdr;
	RPCMsg req;
	void *data;

	hdr.type = RPCTypeRequest;
	hdr.method = msg->method;
	hdr.size = msg->param_size;

	rpcmsghdr_encode(&hdr, buf, sizeof(buf));

	if (rpc_send(sess, buf, sizeof(buf)) != 0) {
		logger_debug("rpc_send header failed.");
		return -1;
	}
	if (msg->param_size) {
		if (rpc_send(sess, msg->param, msg->param_size) != 0) {
			logger_debug("rpc_send param failed.");
			return -1;
		}
	}

	for (;;) {
		if (rpc_recv(sess, buf, sizeof(buf)) != 0) {
			logger_debug("rpc_recv header failed.");
			return -1;
		}

		rpcmsghdr_decode(&hdr, buf, sizeof(buf));

		if (hdr.size) {
			data = malloc(hdr.size);
			if (data == NULL)
				return -1;
			if (rpc_recv(sess, data, hdr.size) != 0) {
				logger_debug("rpc_recv failed.");
				free(data);
				return -1;
			}
		} else {
			data = NULL;
		}

		if (hdr.type == RPCTypeResponse) {
			msg->ret = data;
			msg->ret_size = hdr.size;
			break;
		}

		req.method = hdr.method;
		req.param = data;
		req.param_size = hdr.size;
		req.ret = NULL;
		req.ret_size = 0;

		sess->dispatch(sess, &req, sess->ctxt);

		free(data);
	}

	return 0;
}

int
rpc_handle(RPCSess *sess)
{
	char buf[RPCMsgHdrSize];
	RPCMsgHdr hdr;
	RPCMsg req;
	void *data;

	if (rpc_recv(sess, buf, sizeof(buf)) != 0) {
		logger_debug("rpc_recv header failed.");
		return -1;
	}

	rpcmsghdr_decode(&hdr, buf, sizeof(buf));

	if (hdr.size) {
		data = malloc(hdr.size);
		if (data == NULL)
			return -1;
		if (rpc_recv(sess, data, hdr.size) != 0) {
			logger_debug("rpc_recv failed.");
			free(data);
			return -1;
		}
	} else {
		data = NULL;
	}

	if (hdr.type == RPCTypeResponse) {
		logger_debug("unexpected Response message");
		free(data);
		return -1;
	}

	req.method = hdr.method;
	req.param = data;
	req.param_size = hdr.size;
	req.ret = NULL;
	req.ret_size = 0;

	sess->dispatch(sess, &req, sess->ctxt);

	free(data);

	return 0;
}

int
rpc_return(RPCSess *sess, RPCMsg *msg)
{
	char buf[RPCMsgHdrSize];
	RPCMsgHdr hdr;
	
	hdr.type = RPCTypeResponse;
	hdr.method = msg->method;
	hdr.size = msg->ret_size;

	rpcmsghdr_encode(&hdr, buf, sizeof(buf));

	if (rpc_send(sess, buf, sizeof(buf)) != 0) {
		logger_debug("rpc_send header failed.");
		return -1;
	}
	if (msg->ret_size) {
		if (rpc_send(sess, msg->ret, msg->ret_size) != 0) {
			logger_debug(" rpc_send ret failed.");
			return -1;
		}
	}

	return 0;
}
