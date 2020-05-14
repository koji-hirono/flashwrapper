#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"


static void *
buf_expand(BufWriter *w, size_t add)
{
	size_t cap;
	char *data;

	cap = w->cap + add;
	data = realloc(w->data, cap);
	if (data == NULL)
		return NULL;

	w->cap = cap;
	w->data = data;
	return w->data + w->len;
}

int
buf_writer_open(BufWriter *w, size_t cap)
{
	if (cap == 0) {
		w->data = NULL;
	} else {
		w->data = malloc(cap);
		if (w->data == NULL)
			return -1;
	}

	w->cap = cap;
	w->len = 0;

	return 0;
}

void
buf_writer_close(BufWriter *w)
{
	free(w->data);
}

void
buf_reader_init(BufReader *r, const void *data, size_t len)
{
	r->data = data;
	r->len = len;
	r->pos = 0;
}

int
buf_uint64_encode(BufWriter *w, uint64_t x)
{
	char *p;

	if (w->len + sizeof(x) < w->cap)
		p = w->data + w->len;
	else if ((p = buf_expand(w, sizeof(x))) == NULL)
		return -1;

	*p++ = (x >> 56) & 0xff;
	*p++ = (x >> 48) & 0xff;
	*p++ = (x >> 40) & 0xff;
	*p++ = (x >> 32) & 0xff;
	*p++ = (x >> 24) & 0xff;
	*p++ = (x >> 16) & 0xff;
	*p++ = (x >> 8) & 0xff;
	*p = x & 0xff;
	w->len += sizeof(x);

	return 0;
}

int
buf_uint32_encode(BufWriter *w, uint32_t x)
{
	char *p;

	if (w->len + sizeof(x) < w->cap)
		p = w->data + w->len;
	else if ((p = buf_expand(w, sizeof(x))) == NULL)
		return -1;

	*p++ = (x >> 24) & 0xff;
	*p++ = (x >> 16) & 0xff;
	*p++ = (x >> 8) & 0xff;
	*p = x & 0xff;
	w->len += sizeof(x);

	return 0;
}

int
buf_uint16_encode(BufWriter *w, uint16_t x)
{
	char *p;

	if (w->len + sizeof(x) < w->cap)
		p = w->data + w->len;
	else if ((p = buf_expand(w, sizeof(x))) == NULL)
		return -1;

	*p++ = (x >> 8) & 0xff;
	*p = x & 0xff;
	w->len += sizeof(x);

	return 0;
}

int
buf_uint8_encode(BufWriter *w, uint8_t x)
{
	char *p;

	if (w->len + sizeof(x) < w->cap)
		p = w->data + w->len;
	else if ((p = buf_expand(w, sizeof(x))) == NULL)
		return -1;

	*p = x & 0xff;
	w->len++;

	return 0;
}

int
buf_double_encode(BufWriter *w, double x)
{
	return buf_uint64_encode(w, *(uint64_t *)&x);
}

int
buf_bytes_encode(BufWriter *w, const void *data, size_t len)
{
	char *p;

	if (w->len + len < w->cap)
		p = w->data + w->len;
	else if ((p = buf_expand(w, len)) == NULL)
		return -1;

	memcpy(p, data, len);
	w->len += len;

	return 0;
}

int
buf_string_encode(BufWriter *w, const char *data)
{
	return buf_bytes_encode(w, data, strlen(data) + 1);
}

const void *
buf_uint64_decode(BufReader *r, uint64_t *x)
{
	const char *p;
	size_t i;

	if (r->pos >= r->len)
		return NULL;

	p = (const char *)r->data + r->pos;

	*x = p[0] & 0xff;
	for (i = 1; i < sizeof(*x); i++) {
		*x <<= 8;
		*x |= p[i] & 0xff;
	}

	r->pos += i;

	return p;
}

const void *
buf_uint32_decode(BufReader *r, uint32_t *x)
{
	const char *p;
	size_t i;

	if (r->pos >= r->len)
		return NULL;

	p = (const char *)r->data + r->pos;

	*x = p[0] & 0xff;
	for (i = 1; i < sizeof(*x); i++) {
		*x <<= 8;
		*x |= p[i] & 0xff;
	}

	r->pos += i;

	return p;
}

const void *
buf_uint16_decode(BufReader *r, uint16_t *x)
{
	const char *p;
	size_t i;

	if (r->pos >= r->len)
		return NULL;

	p = (const char *)r->data + r->pos;

	*x = p[0] & 0xff;
	for (i = 1; i < sizeof(*x); i++) {
		*x <<= 8;
		*x |= p[i] & 0xff;
	}

	r->pos += i;

	return p;
}

const void *
buf_uint8_decode(BufReader *r, uint8_t *x)
{
	const char *p;

	if (r->pos >= r->len)
		return NULL;

	p = (const char *)r->data + r->pos;

	*x = p[0] & 0xff;
	r->pos++;

	return p;

}

const void *
buf_double_decode(BufReader *r, double *x)
{
	return buf_uint64_decode(r, (uint64_t *)x);
}

const void *
buf_bytes_decode(BufReader *r, size_t len)
{
	const char *p;

	if (r->pos >= r->len)
		return NULL;

	p = (const char *)r->data + r->pos;

	r->pos += len;

	return p;
}
