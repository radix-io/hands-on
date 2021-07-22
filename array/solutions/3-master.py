from mpi4py import MPI
import sys
import array

YDIM = 1
XDIM = 5


def generate_data(seed=0, x=XDIM, y=YDIM):
    raw_data = []
    for x in range(x*y):
        raw_data.append(seed*10+x)

    return raw_data


def write_data(comm=MPI.COMM_WORLD, filename="dummy"):

    all_data = generate_data(comm.Get_rank(), XDIM, YDIM)

    all_data = comm.gather(all_data, root=0)

    # While it's not exactly natural to write out binary in Python,
    # binary data is easier to decompose across multiple processors:
    # fifty integers always take the same amount of space, no matter
    # if those integers are '1' or '19822523'

    if comm.Get_rank() == 0:
        # subsequent data is for all processes, one row per process
        science_metadata = [YDIM*comm.Get_size(), XDIM, 1]

        # we use lowercase-g gather, so we operated on python objects,
        # getting back a list of lists.  Smush them together
        science_data = []
        for x in all_data:
            science_data = science_data + x

        f = open(filename, "wb")
        f.write(array.array("i", science_metadata).tobytes())
        f.write(array.array("i", science_data).tobytes())
    else:
        assert all_data is None


write_data(MPI.COMM_WORLD, sys.argv[1])
