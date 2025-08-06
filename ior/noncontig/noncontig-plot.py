#!/usr/bin/env python

import sys
import io
import matplotlib.pyplot as plt
import pandas as pd

from matplotlib.pyplot import figure

def ior_to_df(filename):

    # if you give IOR a config file it will run all the experiments then dump out a nice table at the end
    found_start=0
    input_buffer = io.StringIO()

    df = pd.DataFrame()

    with open(filename, 'r') as f:
        for line in f:
            if found_start:
                input_buffer.write(line)
            elif line.startswith("Summary of all tests:"):
                found_start=1
        # read_csv will s
        input_buffer.seek(0);
        #print(input_buffer)
        df = pd.read_csv(input_buffer, sep='\s+', skipfooter=1, engine='python')

    # cleanup:
    # - row 0 is a warmup
    df.drop(df.index[0], inplace=True)
    return df


figure(figsize=(10, 3.5), dpi=80)

plt.xlabel("Pieces")
plt.ylabel("Time (seconds)")
plt.title("IOR write time as degree of noncontig increases")

collective = ior_to_df(sys.argv[1]);
tuned_collective = ior_to_df(sys.argv[2]);
listio     = ior_to_df(sys.argv[3]);

plt.plot('segcnt', 'Mean(s)', data=collective, label="collective")
plt.plot('segcnt', 'Mean(s)', data=tuned_collective, label="tuned collective")
plt.plot('segcnt', 'Mean(s)', data=listio, label="list-io")
plt.xscale('log', base=10)
plt.legend()
plt.savefig("ior-noncontig.png")
