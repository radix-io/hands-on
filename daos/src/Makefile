CC=mpicc

BINS=posix-write posix-read mpiio-write mpiio-read dfs-write dfs-read daos-hello-world

all:: $(BINS)

posix-write.o: array.h

posix-read.o: array.h

mpiio-write.o: array.h

mpiio-read.o: array.h

dfs-write.o: array.h

dfs-read.o: array.h

posix-write: posix-write.o util.o
	$(CC) -o $@ $^

posix-read: posix-read.o util.o
	$(CC) -o $@ $^

mpiio-write: mpiio-write.o util.o
	$(CC) -o $@ $^

mpiio-read: mpiio-read.o util.o
	$(CC) -o $@ $^

dfs-write: dfs-write.o util.o
	$(CC) -o $@ $^ -ldfs -ldaos

dfs-read: dfs-read.o util.o
	$(CC) -o $@ $^ -ldfs -ldaos

daos-hello-world: daos-hello-world.o
	$(CC) -o $@ $^ -ldfs -ldaos

clean::
	rm -f *.o $(BINS) testfile

