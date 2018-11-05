# change these three according to your environment, or load apropriate
# modules(1)
HDF5=/usr
MPICC=cc

CC=cc
CPPFLAGS=-I${HDF5}/include
LDFLAGS=-L${HDF5}/lib

# for generating slides
CTORTF=../mpi2tutorial/maint/ctortf

PROGRAMS= h5par_ex1a h5par_ex1b h5par_ex1c \
	  h5par_ex2a h5par_ex2b

all: $(PROGRAMS)

examples: array.rtf mpi-util.rtf \
	1-description.rtf 2-posix.rtf 3-master.rtf \
	5-array-mpiio-write.rtf 6-array-mpiio-write-read.rtf \
	7-array-pnetcdf-write.rtf 8-array-pnetcdf-write-read.rtf \
	9-array-pnetcdf-op-combine.rtf


h5par_ex1a: h5par_ex1a.c
	$(MPICC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS) -lhdf5

h5par_ex1b: h5par_ex1b.c
	$(MPICC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS) -lhdf5

h5par_ex1c: h5par_ex1c.c
	$(MPICC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS) -lhdf5

h5par_ex2a: h5par_ex2a.c
	$(MPICC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS) -lhdf5

h5par_ex2b:  h5par_ex2b.c
	$(MPICC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS) -lhdf5

h5par-comparison:  h5par-comparison.c
	$(MPICC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS) -lhdf5
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -DUSE_COLL_DATA $^ -o h5par-comparison-collio $(LDFLAGS) -lhdf5
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -DUSE_COLL_DATA -DUSE_COLL_MD $^ -o h5par-comparison-collmd $(LDFLAGS) -lhdf5

clean:
	rm -f *.o *.rtf *.nc $(PROGRAMS)

# OK to ignore: used to generate syntax-highlighted code fragments for
# presentation

%.rtf: %.c
	${CTORTF}  $< $@

%.rtf: %.h
	${CTORTF}  $< $@
