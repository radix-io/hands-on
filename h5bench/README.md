# Understanding h5bench I/O

This directory contains an initial `h5bench` example setup for use in NERSC's Perlmutter system. Use in other systems might required modifications to the code according to each site's documentation.

## Initial setup (NERSC Perlmutter)

* Confirm account access if you haven't already (see presenters for details)
* Log on to Perlmutter
* Download the tutorial materials to your home directory.
  * `mkdir atpesc-io`
  * `cd atpesc-io`
  * `git clone https://github.com/radix-io/hands-on.git`
  * `cd hands-on/h5bench`

## Hands-On

### Install h5bench

* h5bench: https://github.com/hpc-io/h5bench
* Documentation: https://h5bench.readthedocs.io

Make sure you clone `h5bench` and install the base kernels. Once setup, make all the necessary changes to the provided SLURM batch script to point to your local installation.

### Exercise

You're goal is to use Darshan logs and traces in the provided baseline setup `atpsec.json` to understand the application's I/O behavior. 

* What can you infer about the application I/O behavior from Darshan’s report?
* What is the I/O bandwidth and time? 
* How many files did the application use? 
* Do you see any opportunities to tune the I/O?

After this initial run and analysis, try to make changes to the exposed I/O related parameters to tune the I/O performance to the system. Rember to collect and compare the logs/metrics of your tuned executions. 

* What was the observed impact of the changes you proposed?
* Does it follow the system I/O recommendation?

For additional information, refer to `h5bench` documentation.
