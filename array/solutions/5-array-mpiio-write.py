from mpi4py import MPI
import sys
import numpy as np

YDIM = 1
XDIM = 5


def generate_data(seed=0, x=XDIM, y=YDIM):
    raw_data = np.empty(x*y, dtype='int32')
    for x in range(x*y):
        raw_data[x] = seed*10+x

    return raw_data


# looks the same as the "all from rank 0": takes a communicator and a file
# name.  This version however will use MPI-IO: rank 0 will write the header.
# All ranks will write out the array data collectively

def write_data(comm=MPI.COMM_WORLD, filename="dummy"):

    amode = MPI.MODE_CREATE | MPI.MODE_WRONLY

    my_data = generate_data(comm.Get_rank(), XDIM, YDIM)

    fh = MPI.File.Open(comm, filename, amode)

    # subsequent data is for all processes, one row per process
    science_metadata = np.array([YDIM*comm.Get_size(), XDIM, 1], dtype='int32')

    # While it's not exactly natural to write out binary in Python,
    # binary data is easier to decompose across multiple processors:
    # fifty integers always take the same amount of space, no matter
    # if those integers are '1' or '19822523'

    if comm.Get_rank() == 0:

        fh.Write(science_metadata.tobytes())

    sizeof_int = 4  # we will store C-style integers, not python int objects
    fh.Set_view(len(science_metadata)*sizeof_int,
                etype=MPI.INT, filetype=MPI.INT)
    fh.Write_at_all(comm.Get_rank()*XDIM*YDIM,
                    my_data.tobytes())
    fh.Close()


write_data(MPI.COMM_WORLD, sys.argv[1])
