# Makefile for simple echo client and server

CXX=		g++ $(CCFLAGS)
GOODBYE=		goodbye.o
HELLO=		hello.o
SLOW-GOODBYE=	slow-server.o
OBJS =		$(GOODBYE) $(HELLO) $(SLOW-GOODBYE)

LIBS=

CCFLAGS= -g

all:	goodbye hello slow-server

goodbye: $(GOODBYE)
	$(CXX) -o goodbye $(GOODBYE) $(LIBS)

hello: $(HELLO)
	$(CXX) -o hello $(HELLO) $(LIBS)

slow-server: $(SLOW-GOODBYE)
	$(CXX) -o slow-server $(SLOW-GOODBYE) $(LIBS)

clean:
	rm -f $(OBJS) $(OBJS:.o=.d)

realclean:
	rm -f $(OBJS) $(OBJS:.o=.d) goodbye hello slow-server


# These lines ensure that dependencies are handled automatically.
%.d:	%.cc
	$(SHELL) -ec '$(CC) -M $(CPPFLAGS) $< \
		| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

include	$(OBJS:.o=.d)
