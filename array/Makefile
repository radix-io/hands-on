# change these three according to your environment, or load apropriate
# modules(1)
PNETCDF=/usr
MPICC=cc

CC=cc
CPPFLAGS=-I${PNETCDF}/include
# for generating slides
CTORTF=../mpi2tutorial/maint/ctortf

PROGRAMS=1-description 2-posix 3-master\
	5-array-mpiio-write 6-array-mpiio-write-read \
	7-array-pnetcdf-write 8-array-pnetcdf-write-read\
	9-array-pnetcdf-op-combine

all: $(PROGRAMS)

examples: array.rtf mpi-util.rtf \
	1-description.rtf 2-posix.rtf 3-master.rtf \
	5-array-mpiio-write.rtf 6-array-mpiio-write-read.rtf \
	7-array-pnetcdf-write.rtf 8-array-pnetcdf-write-read.rtf \
	9-array-pnetcdf-op-combine.rtf


1-description: 1-description.o

2-posix: 2-posix.o util.o

3-master: 3-master.c util.o
	$(MPICC) $(CFLAGS) $(CPPFLAGS) $^ -o $@

5-array-mpiio-write: 5-array-mpiio-write.c util.o
	$(MPICC) $(CFLAGS) $(CPPFLAGS) $^ -o $@

6-array-mpiio-write-read: 6-array-mpiio-write-read.c util.o
	$(MPICC)  $(CFLAGS) $(CPPFLAGS) $^ -o $@

7-array-pnetcdf-write: 7-array-pnetcdf-write.c util.o
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -I${PNETCDF}/include $^ -o $@ -L${PNETCDF}/lib -lpnetcdf

8-array-pnetcdf-write-read: 8-array-pnetcdf-write-read.c util.o
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -I${PNETCDF}/include $^ -o $@ -L${PNETCDF}/lib -lpnetcdf

9-array-pnetcdf-op-combine: 9-array-pnetcdf-op-combine.c util.o
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -I${PNETCDF}/include $^ -o $@ -L${PNETCDF}/lib -lpnetcdf

clean:
	rm -f *.o *.rtf *.nc $(PROGRAMS)

# OK to ignore: used to generate syntax-highlighted code fragments for
# presentation

%.rtf: %.c
	${CTORTF}  $< $@

%.rtf: %.h
	${CTORTF}  $< $@
