ALL: all-redirect

MAKE = make

all-redirect:
	cd examples && $(MAKE)

notes:
	cd examples && $(MAKE) notes

clean:
	cd examples && $(MAKE) clean

dist:
	if [ ! -d tmpdir ] ; then mkdir tmpdir ; fi
	cd tmpdir && cvs -d `cat ../CVS/Root` export -D now mpi2tutorial
	cd tmpdir/mpi2tutorial && autoconf
	cd tmpdir && \
		tar czf ../mpi2tutorial-`date '+%m-%d-%Y'`.tar.gz mpi2tutorial
	rm -rf tmpdir
