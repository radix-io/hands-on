# change these three according to your environment, or load apropriate
# modules(1)
# setting PNECDF to /usr is a trick. It lets us rely on modules in the common
# case.  If you went and build your own pnetcdf and installed it into your,
# say, $HOME/soft directory, then you can modify this line.
PNETCDF=/usr
MPICC=mpicc

CC=gcc
CPPFLAGS=-I${PNETCDF}/include
# for generating slides
CTORTF=../../mpi2tutorial/maint/ctortf

PROGRAMS=posix master\
	array-mpiio-write array-mpiio-write-read \
	array-pnetcdf-write array-pnetcdf-write-read\
	array-pnetcdf-op-combine

all: $(PROGRAMS)

examples: array.rtf mpi-util.rtf \
	description.rtf posix.rtf master.rtf \
	array-mpiio-write.rtf array-mpiio-write-read.rtf \
	array-pnetcdf-write.rtf array-pnetcdf-write-read.rtf


description: description.o

posix: posix.o util.o

master: master.c util.o
	$(MPICC) $(CFLAGS) $(CPPFLAGS) $^ -o $@

array-mpiio-write: array-mpiio-write.c util.o
	$(MPICC) $(CFLAGS) $(CPPFLAGS) $^ -o $@

array-mpiio-write-read: array-mpiio-write-read.c util.o
	$(MPICC)  $(CFLAGS) $(CPPFLAGS) $^ -o $@

array-pnetcdf-write: array-pnetcdf-write.c util.o
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -I${PNETCDF}/include $^ -o $@ -L${PNETCDF}/lib -lpnetcdf

array-pnetcdf-write-read: array-pnetcdf-write-read.c util.o
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -I${PNETCDF}/include $^ -o $@ -L${PNETCDF}/lib -lpnetcdf

array-pnetcdf-op-combine: array-pnetcdf-op-combine.c util.o
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -I${PNETCDF}/include $^ -o $@ -L${PNETCDF}/lib -lpnetcdf

array-pnetcdf-op-combine-compare: array-pnetcdf-op-combine-compare.c util.o
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -I${PNETCDF}/include $^ -o 10-array-pnetcdf -L${PNETCDF}/lib -lpnetcdf
	$(MPICC) $(CFLAGS) $(CPPFLAGS) -DOP_COMBINE -I${PNETCDF}/include $^ -o array-pnetcdf-op-combine -L${PNETCDF}/lib -lpnetcdf

clean:
	rm -f *.o *.rtf *.nc $(PROGRAMS)

# OK to ignore: used to generate syntax-highlighted code fragments for
# presentation

%.rtf: %.c
	${CTORTF}  $< $@

%.rtf: %.h
	${CTORTF}  $< $@
