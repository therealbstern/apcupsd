#
#
#  Makefile for LaTeX  
#
# To build everything do
#    make tex
#    make links
#    make all
#    make web
#    make html
#    make pdf
#
# or simply
#
#    make
#

#
# Location of old wml files for conversion
#
WMLSRC=.

IMAGES=../images

#  bacula  -- special case below
#  running -- special case below
#  developers -- special case below

#
# Note, these are all parts of the manual not in any
#  particular order (mostly alphabetic). The order they
#  appear in the manual is defined in bacula.tex
#
# Note also that certain of these chapters are in the 
#  bacula.tex manual, and others in the developers.tex 
#  manual.
#
MANUAL = manual


first_rule: bacula developers

bacula: h2l hyphens tex web html dvipdf

.SUFFIXES:     .tex .html
.PHONY:
.DONTCARE:


tex:
	-latex -interaction=batchmode apcupsd.tex
#	makeindex manual.idx
#	@mv -f manual.ind manuali-general.linked.tex
#	makeindex manual.ddx
#	@mv -f manual.ind manuali-dir.linked.tex
#	makeindex manual.fdx
#	@mv -f manual.ind manuali-fd.linked.tex
#	makeindex manual.sdx
#	@mv -f manual.ind manuali-sd.linked.tex
#	makeindex manual.cdx
#	@mv -f manual.ind manuali-console.linked.tex
#	./check_hyphens.pl manuali-general.linked.tex manuali-dir.linked.tex \
#		manuali-fd.linked.tex manuali-sd.linked.tex manuali-console.linked.tex
#	-latex -interaction=batchmode manual.tex


pdf:
	@echo "Making pdfm"
	dvipdfm -p a4 apcupsd.dvi

html:
	@echo "Making html"
	latex2html -white -no_subdir -split 0 -toc_stars -white -notransparent \
		apcupsd >/dev/null

web:
	@echo "Making web"
	@mkdir -p apcupsd
	@rm -rf apcupsd/*
	latex2html -split 4 -local_icons -t "Apcupsd User's Guide" -long_titles 4 \
		-toc_stars -contents_in_nav -white -notransparent apcupsd >/dev/null

h2l:
	@echo "Making html2latex"
	@(for i in ${MANUAL}; do \
	   echo "$$i"; \
	   grep -v "#include *" ${WMLSRC}/$$i.html >/tmp/1; \
	   ./html2latex -images -home ${IMAGES} -noinitstring \
	    /tmp/1 $$i.tex; \
	 done)

links:
# Include link translation for files that have changed.
	@echo "Making links"
	./link_resolver.pl -f manual.tex

hyphens:
# Changes '--' to '\verb{--{ in the tex files.
	./check_hyphens.pl manual.tex

clean:
	@rm -f 1 2 3
	@rm -f *.aux *.cp *.fn *.ky *.log *.pg
	@rm -f *.backup *.ps *.dvi *.ilg *.lof *.lot
	@rm -f *.old WARNINGS *.out *.toc *.idx
	@rm -f images.pl labels.pl internals.pl


distclean:  clean
