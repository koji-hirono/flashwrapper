PROG:=np_engine
SRCS:=main.c
SRCS+=plugin.c
SRCS+=browser.c
SRCS+=npp.c
SRCS+=npstream.c
SRCS+=../common/rpc.c
SRCS+=../common/buf.c
SRCS+=../common/util.c
SRCS+=../common/logger.c
OBJS:=$(SRCS:.c=.o)

LDADD:=$(shell pkg-config --libs gtk+-2.0)
LDADD+=$(shell pkg-config --libs gthread-2.0)
LDADD+=-lXt
LDADD+=-ldl

CC:=cc
CPPFLAGS:=-Wall
CPPFLAGS+=-W
CPPFLAGS+=-Wno-unused-function
CPPFLAGS+=-Wno-unused-parameter
CPPFLAGS+=-Wno-pointer-sign
CPPFLAGS+=-I../include
CPPFLAGS+=-I../common
CPPFLAGS+=-DXP_UNIX=1
CPPFLAGS+=-DMOZ_X11=1
CPPFLAGS+=$(shell pkg-config --cflags gtk+-2.0)
CPPFLAGS+=$(shell pkg-config --cflags gthread-2.0)

all: $(PROG)

clean:
	rm -f $(OBJS) $(PROG) *~

$(PROG): $(OBJS)
	$(CC) $(CPPFLAGS) -o $@ $^ $(LDADD)

$(OBJS): %.o : %.c
	$(CC) -c $(CPPFLAGS) -o $@ $<

