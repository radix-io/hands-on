# ATPESC 2019 hands-on I/O exercises and reference material

# Table of Contents:
- [Reservation](#reservation)
- [Initial setup](#initial-setup)
- [Darshan](#darshan)
- [Globus](#globus)
- [Array](#array)
- [Variance](#variance)
- [Parallel-NetCDF](#parallel-netcdf)
- [Game of Life](#game-of-life)

Please feel free to use the hands-on time to improve your own applications or
ask the presenters for I/O advice!  We are also providing structured,
self-contained exercises for attendees who would like to gain experience 
working with the tools and technologies presented at ATPESC.

The attendees are required to have a laptop with a working web browser and
SSH client. For the purpose of this tutorial, all exercises can be performed
via an SSH terminal.

## Reservation

ATPESC 2019 attendees will have access to a 79 node reservation on Theta 
from 9am to 9pm to execute hands-on exercises as part of the I/O track.
Just submit your jobs to the `R.ATPESC2019` queue and the
`ATPESC2019` allocation (using `-A ATPESC2019` and `-q R.ATPESC2019` in your job
script or qsub command line) for priority execution.

## Initial setup

All examples in this hands-on collection are intended for execution on the
Theta system at the ALCF. See instructions below for how to set
 up your Theta environment.

* Confirm account access if you haven't already (see presenters for
details)
* Log on to Theta
* Download the tutorial materials to your home directory
  * `mkdir atpesc-io`
  * `cd atpesc-io`
  * `git clone https://xgitlab.cels.anl.gov/ATPESC-IO/hands-on.git`
  * `cd hands-on
* Set up your environment to have access to the utilities needed for the hands-on exercises
  * `source ./setup-env.sh`

Please use the `ATPESC2019` project and `R.ATPESC2019` queue for job submissions.  You can also use
the `/projects/ATPESC2019` directory for data storage if you modify the
exercise scripts or try your own examples.

Please ask your instructors if you have questions or need assistance.

## Darshan

### Running hands-on example programs

* Compile example programs and submit into the job queue (see below for
details on specific example programs)
  * `cc <exampleprogram>.c -o <exampleprogram>`
  * `qsub ./<exampleprogram>.qsub`
* Check the queue to see when your jobs complete
  * `qstat |grep <username>`
* And/or wait for a specific job to complete with `cqwait <jobid>`
* Look for log files in `/lus/theta-fs0/logs/darshan/theta/2019/8/2/<username>*` or `/lus/theta-fs0/logs/darshan/theta/2019/8/3/<username>*`
  * Copy log files to your home directory
* Use `darshan-job-summary.pl` or `darshan-parser` to investigate Darshan
characterization data
  * darshan-job-summary.pl will produce \*.pdf files with an analysis summary.
  * You can use scp to copy these to your laptop to view them, or run `evince *.pdf` on Theta to display them remotely over your ssh session it forwards X connections.

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

## Globus 

The Globus hands-on exercise demonstrates how to retrieve a file from a
Globus endpoint.  If you retrieve the file correctly, you can decode it to
receive a secret message!

### Create an Globus account

* Go to https://www.globus.org and click the "Login" button in the upper
  right hand corner
* Sign into Globus using either
    * An institutional login (you can select "Argonne LCF" to login using an
      ALCF cryptocard)
    * A Google account
    * An Orcid
* Once you log in, you will have a "Transfer files" screen
* Click on the "Endpoint" box on the left side of the transfer
    * In the search dialog, look for the "ESnet Read-Only Test DTN at Sunnyvale"
      endpoint and select it
* Click on the "Endpoint" box on the right side of the transfer
    * In the search dialog, look for the "alcf#dtn\_theta" endpoint and
      select it
* You are now ready to transfer a file from Sunnyvale to your home directory
  on Theta!
* Click on the "1M.dat" file on the left side (ESnet Sunnyvale)
* Click on the blue arrow to transfer it to the right side (ALCF Theta)
* Once the file has transfered, log into Theta and run
  `hands-on/globus/globus-decoder.sh 1M.dat` to decode a secret message, and
  let your instructors know if you find it!

Note that this is just a basic demonstration of how easy it is to transfer
files with Globus.  For small files like this one you could have just as
easily used scp.  For larger data sets, Globus online gives you a number of
additional features and far higher performance.  See the ATPESC 2019 "Data
Management Tools" presentation for details.

## Array

The presentation will walk you through several interfaces for writing an array
to a file.  We have provided you with some skeleton code which you can build
upon during the lecture.  If you get stuck you can find complete examples in
the `solutions` directory.

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

You can install Parallel-NetCDF on your laptop easily enough, or you can use
the installation on Mira/Cetus (at /soft/libraries/pnetcdf/current/cnk-gcc/current).  The
Parallel-NetCDF projet has a [Quick Tutorial](http://trac.mcs.anl.gov/projects/parallel-netcdf/wiki/QuickTutorial) outlining several different ways
one can do I/O with Parallel-NetCDF.  We'll also explore attributes.

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

We have provided the Game of Life code discussed today if you want to
experiment with I/O and do not already have a program handy.

### Building Notes
To build the "game of life" example on ALCF's IBM machines (Mira, Cetus, Vesta), configure with
`--with-mpi=/soft/compilers/wrappers/gcc/` and
`--with-pnetcdf=/soft/libraries/pnetcdf/current/cnk-gcc/current`

    configure --with-mpi=/soft/compilers/wrappers/gcc/ \
        --with-pnetcdf=/soft/libraries/pnetcdf/current/cnk-gcc/current


For Theta, the paths are a little different.  Configure might pick up the right
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

