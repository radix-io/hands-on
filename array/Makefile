MPI=/usr
PNETCDF=/usr
CC=${MPI}/bin/mpicc
CPPFLAGS=-I${PNETCDF}/include
CTORTF=../mpi2tutorial/maint/ctortf
#LDFLAGS=-L${PNETCDF}/lib
#LDLIBS=-lpnetcdf

all: array array-pnetcdf

examples: 1-description.rtf 2-posix.rtf 3-master.rtf array.rtf mpi-util.rtf

array: array.o

array-pnetcdf: array-pnetcdf.o

1-description: 1-description.o

2-posix: 2-posix.o util.o

3-master: 3-master.o util.o

clean:
	rm -f *.o 1-description 1-description 2-posix 3-master *.rtf

# OK to ignore: used to generate syntax-highlighted code fragments for
# presentation

%.rtf: %.c
	${CTORTF}  $< $@

%.rtf: %.h
	${CTORTF}  $< $@
