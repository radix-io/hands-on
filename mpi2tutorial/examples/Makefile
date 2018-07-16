ALL: all-redirect

MAKE = make

CTOPS=../maint/ctops
CTOPDF=../maint/ctopdf

DIRS = life pneo spawn mutex

PSFILES=life/mlife.ps \
	life/mlife-pt2pt.ps \
	life/mlife-io-stdout.ps \
	life/mlife-io-mpiio.ps \
	life/mlife-io-pnetcdf.ps \
	life/mlife-fence.ps \
	life/mlife-pscw.ps \
	life/mlife2d.ps \
	life/mlife2d-pt2pt.ps \
	life/mlife2d-io-stdout.ps \
	life/mlife2d-fence.ps \
	pneo/pneo_fence.ps \
	pneo/pneo_pscw.ps

PDFFILES = $(PSFILES:.ps=.pdf)

#	sparsemat/csrio-pnetcdf.ps
#	life/mlife2d-io-mpiio.ps
#	life/mlife2d-io-pnetcdf.ps

all-redirect:
	for dir in life pneo spawn mutex ; do \
	    (cd $$dir && $(MAKE) ) ; \
	done

notes: $(PSFILES) $(PDFFILES)
# psmerge doesn't work for enscript output, for whatever reason.
# examples.ps: $(PSFILES)
#	psmerge $(PSFILES) > $@

$(PSFILES): %.ps: %.c
	-$(CTOPS) $? $@

$(PDFFILES): %.pdf: %.c
	-$(CTOPDF) $? $@
clean:
	for dir in $(DIRS) ; do (cd $$dir && $(MAKE) clean) ; done
