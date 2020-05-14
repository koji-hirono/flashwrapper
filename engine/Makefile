PROG:=np_engine
SRCS:=engine_main.c
SRCS+=plugin.c
SRCS+=browser.c
SRCS+=util.c
SRCS+=logger.c
OBJS:=$(SRCS:.c=.o)

LDADD:=$(shell pkg-config --libs gtk+-2.0)
LDADD+=-lXt
LDADD+=-ldl

CC:=cc
CPPFLAGS:=-Wall
CPPFLAGS+=-W
CPPFLAGS+=-I../include
CPPFLAGS+=-DXP_UNIX=1
CPPFLAGS+=-DMOZ_X11=1
CPPFLAGS+=$(shell pkg-config --cflags gtk+-2.0)

all: $(PROG)

clean:
	rm -f $(OBJS) $(PROG) *~

$(PROG): $(OBJS)
	$(CC) $(CPPFLAGS) -o $@ $^ $(LDADD)

$(OBJS): %.o : %.c
	$(CC) -c $(CPPFLAGS) -o $@ $<

