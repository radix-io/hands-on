# ATPESC 2020 hands-on I/O exercises and reference material

# Table of Contents:
- [Reservation](#reservation)
- [Initial setup](#initial-setup)
- [Darshan](#darshan)
- [Array](#array)
- [Variance](#variance)
- [Parallel-NetCDF](#parallel-netcdf)
- [Game of Life](#game-of-life)

This is the documentation for the hands-on exercises in the ``Track 3: Data
Intensive Computing and I/O'' portion of ATPESC 2020.  Agenda information
can be found here:

[ATPESC 2020 Track 3 agenda](https://extremecomputingtraining.anl.gov/agenda-2020/#Track-3)

We will describe these exercises in greater detail during the ATPESC
lectures and provide hands-on support via the #io channel in Slack.  Feel
free to ask us questions about your own code as well.

The attendees are required to have a laptop with a working web browser and
SSH client. For the purpose of this tutorial, all exercises can be performed
via an SSH terminal.

## Reservations

ATPESC 2020 attendees will have access to a 64 node reservation on Theta
(ALCF) and an 18 node reservation on Ascent (OLCF) from 9am to 6pm CT
to execute hands-on exercises as part of the I/O track.

On Theta, submit your jobs to the `ATPESC2020` queue and the `ATPESC2020`
allocation using the `-A ATPESC2020` and `-q ATPESC2020` options in your job
script or qsub command line.  You can use the /projects/ATPESC2020/
directory for data storage; please create a subdirectory there based on your
username to avoid conflicts with other users.

On Ascent, submit your jobs in the GEN139 project using the `-P GEN139`
options in your job script or bsub command line.

## Initial setup (Theta or Ascent)

* Confirm account access if you haven't already (see presenters for
details)
* Log on to Theta or Ascent
* Download the tutorial materials to your home directory.  Theta and Ascent are
  completely different machines managed by different groups so if you plan on
  trying out both, you'll have to repeat these steps on each machine.
  * `mkdir atpesc-io`
  * `cd atpesc-io`
  * `git clone https://xgitlab.cels.anl.gov/ATPESC-IO/hands-on.git`
    * ascent only: did you get `error: RPC failed; result=22, HTTP code = 404` ?
    * default git (`/usr/bin/git`) on ascent is too old.  Do a `module load git`
      to bring in version 2.20.1: that version is new enough to negotiate
      with our xgitlab.cels.anl.gov server
  * `cd hands-on`
* Set up your environment to have access to the utilities needed for the hands-on exercises
  * `source ./theta-setup-env.sh` or `ascent-setup-env.sh`

## Darshan

### Running hands-on example programs

All Darshan hands-on examples are set up for use on the Theta (ALCF) system.
See the Theta setup instructions above to configure your baseline
environment.

* Compile example programs and submit into the job queue (see below for
details on specific example programs)
  * `cc <exampleprogram>.c -o <exampleprogram>`
  * `qsub ./<exampleprogram>.qsub`
* Check the queue to see when your jobs complete
  * `qstat |grep <username>`
* And/or wait for a specific job to complete with `cqwait <jobid>`
* Look for log files in `/lus/theta-fs0/logs/darshan/theta/2020/7/31/<username>*` (or whatever the current day is in GMT)
  * Copy log files to your home directory
* Use `darshan-job-summary.pl` or `darshan-parser` to investigate Darshan
characterization data
  * darshan-job-summary.pl will produce \*.pdf files with an analysis summary.
  * You can use scp to copy these to your laptop to view them, or run `evince *.pdf` on Theta to display them remotely over your ssh session it forwards X connections.

### Darshan on Ascent

On Theta, one can generate a darshan log file and the report all on one system.
Ascent is missing a few packages for that. However, enterprising attendees can
find their log files on Ascent in the `/gpfs/wolf/darshan/ascent/2020/7/31`
directory.  Adventuresome attendees could try moving the log file to theta (or
a laptop with the `darshan-util` package and necessary latex components)
installed and generating the report there.

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

