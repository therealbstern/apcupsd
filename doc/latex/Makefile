# #
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


MANUAL = manual

first_rule: apcupsd

apcupsd: h2l texcheck tex web html dvipdf

.SUFFIXES:     .tex .html
.PHONY:
.DONTCARE:


tex:
	cp -fp ${IMAGES}/hires/*.eps .
	-latex -interaction=batchmode apcupsd.tex
	makeindex apcupsd.idx
	-latex -interaction=batchmode manual.tex
	rm -f *.eps


pdf:
	cp -fp ${IMAGES}/hires/*.eps .
	@echo "Making pdfm"
	dvipdfm -p a4 apcupsd.dvi
	rm -f *.eps

html:
	@echo "Making html"
	rm -f *.eps *.png
	cp ${IMAGES}/*.eps .
	@rm -f next.*  
	latex2html -white -no_subdir -split 0 -toc_stars -white -notransparent \
		apcupsd >/dev/null
	./translate_images.pl apcupsd.html
	rm -f *.eps

web:
	@echo "Making web"
	@mkdir -p apcupsd
	@rm -rf apcupsd/*
	cp ${IMAGES}/*.eps .
	cp ${IMAGES}/*.eps apcupsd/
	@rm -f next.*
	latex2html -split 5 -local_icons -t "Apcupsd User's Guide" -long_titles 4 \
		-toc_stars -contents_in_nav -white -notransparent apcupsd >/dev/null
	./translate_images.pl apcupsd/Apcupsd_Users_Guide.html
	cp apcupsd/Apcupsd_Users_Guide.html apcupsd/index.html
	rm -f apcupsd/*.old
	rm -f *.eps apcupsd/*.eps

dvipdf:
	@echo "Making dvi to pdf"
	@cp -fp ${IMAGES}/hires/*.eps .
	dvipdf apcupsd.dvi apcupsd.pdf
	@rm -f *.eps


h2l:
	@echo "Making html2latex"
	@(for i in ${MANUAL}; do \
	   echo "$$i"; \
	   grep -v "#include *" ${WMLSRC}/$$i.html >/tmp/1; \
	   ./html2latex -images -home ${IMAGES} -noinitstring \
	    /tmp/1 $$i.tex; \
	 done)

texcheck:
# Does checking for the tex file, and possibly fixes things.
	./check_tex.pl --change apcupsd.tex
  
clean:
	@rm -f 1 2 3
	@rm -f *.aux *.cp *.fn *.ky *.log *.pg
	@rm -f *.backup *.ps *.dvi *.ilg *.lof *.lot
	@rm -f *.old WARNINGS *.out *.toc *.idx
	@rm -f images.pl labels.pl internals.pl
	@rm -f *.png *.pdf *.epf


distclean:  clean



