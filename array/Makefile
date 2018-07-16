MPI=/usr
PNETCDF=/usr
CC=${MPI}/bin/mpicc
CPPFLAGS=-I${PNETCDF}/include
LDFLAGS=-L${PNETCDF}/lib
LDLIBS=-lpnetcdf

all: array array-pnetcdf

array: array.o

array-pnetcdf: array-pnetcdf.o
