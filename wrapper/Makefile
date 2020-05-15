LIB:=libwrapper.so
SRCS:=wrapper.c
SRCS+=engine.c
SRCS+=../common/rpc.c
SRCS+=../common/buf.c
SRCS+=../common/util.c
SRCS+=../common/logger.c
OBJS:=$(SRCS:.c=.o)

CC:=cc
CPPFLAGS:=-Wall
CPPFLAGS+=-W
CPPFLAGS+=-Wno-unused-parameter
CPPFLAGS+=-Wno-pointer-sign
CPPFLAGS+=-I../include
CPPFLAGS+=-I../common
CPPFLAGS+=-DXP_UNIX=1
CPPFLAGS+=-fPIC
CPPFLAGS+=-pipe

all: $(LIB)

clean:
	rm -f $(OBJS) $(LIB) *~

$(LIB): $(OBJS)
	$(CC) -shared -o $@ $^ $(LDADD)

$(OBJS): %.o : %.c
	$(CC) -c $(CPPFLAGS) -o $@ $<

