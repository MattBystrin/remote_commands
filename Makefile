BDIR = build

.PHONY: all server client

all: server client

server: server/server.c server/thread_pool.c server/net.c
	gcc -g $^ -o $(BDIR)/server -std=c99 -lpthread 

client: client/client.c
	gcc -g $^ -o $(BDIR)/client -std=c99

dir:
	mkdir -p build
clean: dir
	rm build/*
