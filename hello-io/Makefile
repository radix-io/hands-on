CC=mpicc
CFLAGS=-Wall -g -O2 -fsanitize=undefined -fsanitize=address

all: hello-mpiio hello-mpiio-noncontig hello-mpiio-view

hello-mpiio: hello-mpiio.c
hello-mpiio-noncontig: hello-mpiio-noncontig.c
hello-mpiio-view: hello-mpiio-view.c


clean:
	rm -f hello-mpiio hello-mpiio-noncontig hello-mpiio-view
