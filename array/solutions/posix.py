import array

YDIM = 1
XDIM = 5


def generate_data(seed=0, x=XDIM, y=YDIM):
    raw_data = []
    for x in range(x*y):
        raw_data.append(seed*10+x)

    return raw_data


def write_data(filename="dummy"):
    science_metadata = [YDIM, XDIM, 1]

    science_data = generate_data(0, XDIM, YDIM)

    # While it's not exactly natural to write out binary in Python,
    # binary data is easier to decompose across multiple processors:
    # fifty integers always take the same amount of space, no matter
    # if those integers are '1' or '19822523'

    f = open(filename, "wb")

    f.write(array.array("i", science_metadata).tobytes())
    f.write(array.array("i", science_data).tobytes())


write_data("testfile")
