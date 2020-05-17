LIB:=libwrapper.so
SRCS:=wrapper.c
SRCS+=engine.c
SRCS+=npobject.c
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
CPPFLAGS+=-I/usr/local/include
CPPFLAGS+=-I../include
CPPFLAGS+=-I../common
CPPFLAGS+=-DXP_UNIX=1
CPPFLAGS+=-DMOZ_X11=1
CPPFLAGS+=$(shell pkg-config --cflags gtk+-2.0)
CPPFLAGS+=-fPIC
CPPFLAGS+=-pipe

all: $(LIB)

clean:
	rm -f $(OBJS) $(LIB) *~

install:
	cp $(LIB) /home/paleman/.mozilla/plugins/npwrapper.libflashplayer.so

$(LIB): $(OBJS)
	$(CC) -shared -o $@ $^ $(LDADD)

$(OBJS): %.o : %.c
	$(CC) -c $(CPPFLAGS) -o $@ $<

