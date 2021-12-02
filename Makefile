CC	= gcc
CFLAGS	= 
LDFLAGS	= -pthread

.PHONY: all
all: p2p

p2p: p2p.c utils.c udp_service.c tcp_service.c
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^

clean:
	rm p2p
