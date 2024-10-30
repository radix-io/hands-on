CC=mpicc

CFLAGS=-Wall -g -O2

all: hello-mpiio hello-mpiio-noncontig hello-mpiio-view

hello-mpiio: hello-mpiio.c
hello-mpiio-noncontig: hello-mpiio-noncontig.c
hello-mpiio-view: hello-mpiio-view.c


# Your parallel-netcdf library is probably not installed in /usr unless it was
# via distribution package
# instead, if you installed pnetcdf via spack you could do
#    make PNETCDF=$(spack location -i parallel-netcdf)  hello-pnetcdf
# or if you loaded the 'cray-pnetcdf' module:
#     make PNETCDF=${PNETCDF_DIR} hello-pnetcdf

PNETCDF=/usr
CPPFLAGS=-I${PNETCDF}/include


hello-pnetcdf: hello-pnetcdf.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -I${PNETCDF}/include $^ -o $@ -L${PNETCDF}/lib -lpnetcdf

HDF5=/usr
CPPFLAGS=-I${HDF5}/include
hello-hdf5: hello-hdf5.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -I${HDF5}/include $^ -o $@ -L${HDF5}/lib -lhdf5


clean:
	rm -f hello-mpiio hello-mpiio-noncontig hello-mpiio-view hello-pnetcdf
