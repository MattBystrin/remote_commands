BDIR = build

server: server.c thread_pool.c
	gcc -g $^ -o $(BDIR)/server -std=c99 -lpthread 
dir:
	mkdir -p build
clean: dir
	rm build/*
