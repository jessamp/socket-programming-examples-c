# Makefile for echo client and server

CXX=			g++ $(CCFLAGS)

MSGD=		msgd.o server.o message.o
MSG=		msg.o client.o message.o
OBJS =			$(MSGD) $(MSG)

LIBS=

CCFLAGS= -g -std=c++11

all:	msgd msg

msgd:$(MSGD)
	$(CXX) -o msgd $(MSGD) $(LIBS)

msg:$(MSG)
	$(CXX) -o msg $(MSG) $(LIBS)

clean:
	rm -f $(OBJS) $(OBJS:.o=.d)

realclean:
	rm -f $(OBJS) $(OBJS:.o=.d) msgd msg


# These lines ensure that dependencies are handled automatically.
%.d:	%.cc
	$(SHELL) -ec '$(CC) -M $(CPPFLAGS) $< \
		| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

include	$(OBJS:.o=.d)
