#ifndef BUF_H__
#define BUF_H__

#include <inttypes.h>
#include <stddef.h>

typedef struct BufWriter BufWriter;
typedef struct BufReader BufReader;

struct BufWriter {
	size_t len;
	size_t cap;
	void *data;
};

struct BufReader {
	size_t pos;
	size_t len;
	const void *data;
};


extern int buf_writer_open(BufWriter *, size_t);
extern void buf_writer_close(BufWriter *);
extern void buf_reader_init(BufReader *, const void *, size_t);

extern int buf_uint64_encode(BufWriter *, uint64_t);
extern int buf_uint32_encode(BufWriter *, uint32_t);
extern int buf_uint16_encode(BufWriter *, uint16_t);
extern int buf_uint8_encode(BufWriter *, uint8_t);
extern int buf_double_encode(BufWriter *, double);
extern int buf_bytes_encode(BufWriter *, const void *, size_t);
extern int buf_string_encode(BufWriter *, const char *);

extern const void *buf_uint64_decode(BufReader *, uint64_t *);
extern const void *buf_uint32_decode(BufReader *, uint32_t *);
extern const void *buf_uint16_decode(BufReader *, uint16_t *);
extern const void *buf_uint8_decode(BufReader *, uint8_t *);
extern const void *buf_double_decode(BufReader *, double *);
extern const void *buf_bytes_decode(BufReader *, size_t);

#endif
