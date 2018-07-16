ALL: all-redirect
SHELL = /bin/sh

# This makefile has only a distribution and clean target
dist:
	if [ ! -d tmp ] ; then mkdir tmp ; else rm -rf tmp/* ; fi
	cd tmp && cvs -d `cat ../CVS/Root` export -D 'now' mpi2tutorial
	cd tmp && d=`date +%m-%d-%Y` && mv mpi2tutorial mpi2tutorial-$$d &&\
	    tar zcf ../mpi2tutorial-$$d.tar.gz mpi2tutorial-$$d
	rm -rf tmp

clean:
	cd examples && make clean
