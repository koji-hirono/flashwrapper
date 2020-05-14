LIB:=libwrapper.so
SRCS:=wrapper.c
SRCS+=engine.c
SRCS+=rpc.c
SRCS+=buf.c
SRCS+=logger.c
OBJS:=$(SRCS:.c=.o)

CC:=cc
CPPFLAGS:=-Wall
CPPFLAGS+=-W
CPPFLAGS+=-I../include
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

