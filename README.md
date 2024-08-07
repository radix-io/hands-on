# ATPESC 2024 hands-on I/O exercises and reference material

# Table of Contents:
- [Reservation](#reservation)
- [Initial setup](#initial-setup)
- [Darshan](#darshan)
- [Array](#array)
- [Variance](#variance)
- [Parallel-NetCDF](#parallel-netcdf)
- [Game of Life](#game-of-life)
- [Sparse Matrix](#sparse-matrix)

This is the documentation for the hands-on exercises in the ``Track 7: Data
Intensive Computing and I/O'' portion of ATPESC 2024.  Agenda information
can be found here:

[ATPESC 2024 Track 7 agenda](https://extremecomputingtraining.anl.gov/agenda-2024/#Track-7)

We will describe these exercises in greater detail during the ATPESC
lectures and provide hands-on support via the #io channel in Slack.  Feel
free to ask us questions about your own code as well.

The attendees are required to have a laptop with a working web browser and
SSH client. For the purpose of this tutorial, all exercises can be performed
via an SSH terminal.

## Reservations

ATPESC 2024 attendees will have access to a 300 node reservation on Polaris
(ALCF) from 9am to 9pm CT to execute hands-on exercises as part of the I/O track.

On Polaris, submit your jobs to our reservation using the the `ATPESC2024`
allocation and the `ATPESC` queue (`-A ATPESC2024` and `-q ATPESC` options,
respectively, in your job script or qsub command line).
You can use the /eagle/projects/ATPESC2024/usr/
directory for data storage; please create a subdirectory there based on your
username to avoid conflicts with other users.

## Initial setup (ALCF Polaris)

* Confirm account access if you haven't already (see presenters for
details)
* Log on to Polaris
* Download the tutorial materials to your home directory.
  * `mkdir atpesc-io`
  * `cd atpesc-io`
  * `git clone https://github.com/radix-io/hands-on.git`
  * `cd hands-on`
* Set up your environment to have access to the utilities needed for the hands-on exercises
  * `source ./polaris-setup-env.sh`

## Darshan

### Running hands-on example programs

All Darshan hands-on examples are set up for use on the Polaris (ALCF) system.
See the Polaris setup instructions above to configure your baseline
environment.

* Compile example programs and submit into the job queue (see below for
details on specific example programs)
  * `cc <exampleprogram>.c -o <exampleprogram>`
  * `qsub ./<exampleprogram>.qsub`
* Check the queue to see when your jobs complete
  * `qstat |grep <username>`
* Look for log files in `/lus/grand/logs/darshan/polaris/2024/8/8/<username>*` (or whatever the current day is in UTC)
  * Copy log files to your home directory
* Use the PyDarshan job summary tool or `darshan-parser` to investigate Darshan
characterization data
  * `python -m darshan summary <log_path>` command will produce \*.html files with an analysis summary
  * You can use scp to copy these to your laptop to view them in a browser

### Hands-on exercise: helloworld

The hands-on material includes an example application called `helloworld`.
Compile it, run it, and generate the Darshan job summary following the
instructions above.  How many files did the application open?  How much data
did it read, and how much data did it write?  What approximate I/O
performance did it achieve?

### Hands-on exercise: warpdrive

_NOTE: this exercise is best done
some time after the MPI-IO and/or performance tuning presentations. It
requires diagnosis of I/O performance problems to complete._

The hands-on material includes an example application called `warpdrive`.
There are two versions of this application: warpdriveA and warpdriveB.  Both
of them do the same amount of I/O from each process, but one of them performs
better than the other.  Which one has the fastest I/O?  Why?

### Hands-on exercise: fidgetspinner

_NOTE: this exercise is best done
some time after the MPI-IO and/or performance tuning presentations. It
requires diagnosis of I/O performance problems to complete._

The hands-on material includes an example application called
`fidgetspinner`.  There are two versions of this application:
fidgetspinnerA and fidgetspinnerB.  Both of them do the same amount of
I/O from each process, but one of them performs better than the other.
Which one has the fastest I/O?  Why?

## Array

The presentation will walk you through several interfaces for writing an array
to a file.  We have provided you with some skeleton code which you can build
upon during the lecture.  If you get stuck you can find complete examples in
the `solutions` directory.

## IOR

IOR has a lot of command line arguments.  I have included the job submission
scripts I used for the talk in the 'ior' directory

## Variance

The variance subdirectory contains a hands-on example to illustrate the kind of variance you can expect from each job run in terms of I/O performance.  To execute it:

* `cc variance.c -o variance`
* `qsub variance.qsub`

If you look in variance.qsub you will see that the job is a script job that executes the same program 5 times.  Each will display the elapsed time of the I/O routine.

* What was the slowest time?
* What was the fastest time?
* What was the average time?
* This is a small example program.  Do you think the variance will improve or get worse with a larger example?  What strategies might help improve performance in the example code?

## Parallel-NetCDF

You can install Parallel-NetCDF on your laptop easily enough if you already
have MPI installed.  On Theta or Ascent, the setup script will load the
necessary modules.  The Parallel-NetCDF projet has a
[Quick Tutorial](http://trac.mcs.anl.gov/projects/parallel-netcdf/wiki/QuickTutorial)
outlining several different ways one can do I/O with Parallel-NetCDF.  We'll
also explore attributes.

### Hands-on exercise: comparing I/O approaches
* The
 [QuickTutorial](http://trac.mcs.anl.gov/projects/parallel-netcdf/wiki/QuickTutorial)
 has links to code and some brief discussions about what the
 examples are trying to demonstrate.
* Following the "Real parallel I/O on
 shared files" example, build and run 'pnetcdf-write-standard' to create a
 (tiny) Parallel-NetCDF dataset.
* Look at a Darshan job summary.
* Next, follow the "Non-blocking interface" example to create another (tiny)
 Parallel-NetCDF dataset.
* Compare the Darshan job summary of this approach.  What's different between the two?

### Hands-on exercise: using attributes
* Write a simple parallel-netcdf program that puts your name as a global
  attribute on the data set.  You won't need to define any dimensions or
  variables. (If you need to cheat, look at the example C files above).
* What happens if you define different attributes on different processors?

## Game of Life

We have provided the a Game of Life program if you want to
experiment with I/O and do not already have a program handy.

### Building Notes

For Theta, cnfigure might pick up the right
MPI libraries automatically, but you can explicitly set MPICC and MPIF77 if it
does not.  Also, put configure into cross-compile mode with the `--host` flag so it does not try to run compute node code on the front end.

    $ module add cray-parallel-netcdf
    $ configure --with-pnetcdf=$PARALLEL_NETCDF_DIR \
         MPICC=cc MPIF77=ftn --host=x86_64-unknown-linux-gnu

For another example, I've built and installed MPICH and Parallel-NetCDF into my
home directory on my laptop.  The command might look something like this:

    configure --with-mpi=${HOME}/work/soft/mpich/bin/ \
        --with-pnetcdf=${HOME}/work/soft/pnetcdf


The "game of life" lives in the `examples/life` directory:

    $ cd examples/life
    $ make mlife-mpiio mlife-pnetcdf

### Execution

the mlife example takes a few arguments:

 * -x X, where x is number of columns
 * -y Y, where y is number of rows
 * -i I, where I is iterations to run
 * -r R, where R is iteration number to restart from
 * -p PATH, where PATH is the prefix mlife will put on the checkpoint files

To give you an idea of how big a problem size to use, here are some run times for problem sizes on a few machines I have at hand:
 * laptop: `mpiexec -np 4 ./mlife-mpiio -x 5000 -y 5000 -i 10` takes about 10 seconds.
 * Blue Gene /Q: `qsub -A radix-io -t 10 -n 128 --mode c16 ./mlife-pnetcdf -x 5000 -y 5000 -i 10 -p /projects/radix-io/robl/` takes about 13 seconds.
 * Theta: `./mlife-pnetcdf -x 5000 -y 5000 -i 10 -p /projects/radix-io/robl/mlife`  takes about 17 seconds -- don't forget to increase the default stripe size of your destination directory!



### Project Ideas
* Run the MPI-IO and Parallel-NetCDF versions, then use Darshan to observe
  any differences in behavior
* Rewrite the Parallel-NetCDF version of the Game of Life to dump out all
  checkpoints to a single dataset
* Write an MLIFEIO implementation that uses HDF5
* Experiment with Lustre stripe sizes on Theta.  When is a stripe width of 1 a
   good idea?

## Sparse Matrix I/O

A more sophisticated I/O example demonstrating non-contiguous I/O and
abstraction layers.  Writing a sparse matrix to a file as an N-dimensional
array can be wasteful for a very sparse matrix.  This example uses Compressed
Sparse Row (CSR) representation to write out the file using MPI-IO routines and
optimizations.

### Project Ideas

* The interface writes to MPI-IO.  Update it to use pnetcdf or hdf5
