# AI Checkpointing

This directory contains some examples that illustrate the I/O behavior of AI checkpointing in PyTorch.

While originally written for use on NERSC's Perlmutter system, we have tried to
demonstrate how one might select different accelerator devices.  Most of the
code is device independent.

## Initial setup

* Confirm account access if you haven't already (see presenters for details)
* Log on to your destination machine (e.g. NERSC Perlmutter (nvidia) or ALCF Aurora (intel xpu)
* Download the tutorial materials to your home directory.
  * `mkdir atpesc-io`
  * `cd atpesc-io`
  * `git clone https://github.com/radix-io/hands-on.git`
  * `cd hands-on/ai-checkpoint`

## Hands-On

### Running hands-on example programs
Make sure you modify the `CHECKPOINT` variable in each `.py` to point to someplace inside `$SCRATCH` (Lustre) file system.
You also need to enable Darshan's instrumentation.

### Running at NERSC:

* Submit into the job queue
  * `sbatch <program>-perlmutter.sh`
* Check the queue to see when your jobs complete
  * `squeue --me`

### Running at ALCF

* Submit into the job queue
  * `qsub -q debug -v SCRIPT=<python-script> submit.pbs`
* Check the queue to see when your jobs complete
  * `qstat -u $USER`

### Darshan (any platform)

* Look for Darshan log files in `$DARSHAN_LOGS/2025/8/8/<username>*` (or whatever the current day is in UTC)
  * Copy log files to your home directory
* Use the PyDarshan job summary tool or `darshan-parser` to investigate Darshan characterization data
  * `python -m darshan summary <log_path>` command will produce \*.html files with an analysis summary
  * You can use `scp` to copy these to your laptop to view them in a browser

### Hands-on exercise: PyTorch Checkpoint

The hands-on material includes an example application called `checkpoint-vision-vit`. What is the application's observed I/O behavior? How many files did the application open? How much data did it write? What approximate I/O performance did it achieve?

### Hands-on exercise: PyTorch Distributed Checkpoint (DCP)

The hands-on material includes an example application called `checkpoint-vision-vit`. What is the application's observed I/O behavior? How many files did the application open? How much data did it write? What approximate I/O performance did it achieve?

### Hands-on exercise: PyTorch Asynchronous Distributed Checkpoint (DCP)

The hands-on material includes an example application called `checkpoint-vision-vit`. What is the application's observed I/O behavior? How many files did the application open? How much data did it write? What approximate I/O performance did it achieve? DO you observe any different behavior in Darshan?
