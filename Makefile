BDIR = build
ARGS = -g -pthread -std=c99 -I.

.PHONY: all server client dir

all: dir server client

server: server/server.c server/thread_pool.c server/net.c server/queue.c
	gcc $(ARGS) $^ -o $(BDIR)/server 

client: client/client.c
	gcc $(ARGS) $^ -o $(BDIR)/client 

dir:
	mkdir -p build
clean: dir
	rm build/*
