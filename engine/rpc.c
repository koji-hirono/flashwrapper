#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>

#include "logger.h"
#include "rpc.h"


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
	int r;

	hdr.type = RPCTypeRequest;
	hdr.method = msg->method;
	hdr.size = msg->param_size;

	rpcmsghdr_encode(&hdr, buf, sizeof(buf));

	r = write(sess->fd, buf, sizeof(buf));
	if (r == -1) {
		logger_debug("write(header) failed. %s", strerror(errno));
		return -1;
	}
	if (msg->param_size) {
		r = write(sess->fd, msg->param, msg->param_size);
		if (r == -1) {
			logger_debug("write(param) failed. %s", strerror(errno));
			return -1;
		}
	}

	for (;;) {
		r = read(sess->fd, buf, sizeof(buf));
		if (r == -1) {
			logger_debug("read(header) failed. %s", strerror(errno));
			return -1;
		}
		if (r == 0) {
			logger_debug("read(header) EOF");
			return -1;
		}

		rpcmsghdr_decode(&hdr, buf, sizeof(buf));

		if (hdr.size) {
			data = malloc(hdr.size);
			if (data == NULL)
				return -1;
			r = read(sess->fd, data, hdr.size);
			if (r == -1) {
				logger_debug("read failed. %s", strerror(errno));
				free(data);
				return -1;
			}
			if (r == 0) {
				logger_debug("read FIN");
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
	int r;

	r = read(sess->fd, buf, sizeof(buf));
	if (r == -1) {
		logger_debug("read(header) failed. %s", strerror(errno));
		return -1;
	}
	if (r == 0) {
		logger_debug("read(header) EOF");
		return -1;
	}

	rpcmsghdr_decode(&hdr, buf, sizeof(buf));

	if (hdr.size) {
		data = malloc(hdr.size);
		if (data == NULL)
			return -1;
		r = read(sess->fd, data, hdr.size);
		if (r == -1) {
			logger_debug("read failed. %s", strerror(errno));
			free(data);
			return -1;
		}
		if (r == 0) {
			logger_debug("read FIN");
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
	int r;
	
	hdr.type = RPCTypeResponse;
	hdr.method = msg->method;
	hdr.size = msg->ret_size;

	rpcmsghdr_encode(&hdr, buf, sizeof(buf));

	r = write(sess->fd, buf, sizeof(buf));
	if (r == -1) {
		logger_debug("write(header) failed. %s", strerror(errno));
		return -1;
	}
	if (msg->ret_size) {
		r = write(sess->fd, msg->ret, msg->ret_size);
		if (r == -1) {
			logger_debug("write(ret) failed. %s", strerror(errno));
			return -1;
		}
	}

	return 0;
}
