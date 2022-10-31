BDIR = build

.PHONY: all server client tests

all: server client tests

server: server/server.c server/thread_pool.c server/net.c server/queue.c
	gcc -g $^ -o $(BDIR)/server -std=c99 -pthread 

client: client/client.c
	gcc -g $^ -o $(BDIR)/client -std=c99

tests: tests.c server/thread_pool.c server/queue.c
	gcc -g $^ -o $(BDIR)/tests

dir:
	mkdir -p build
clean: dir
	rm build/*
