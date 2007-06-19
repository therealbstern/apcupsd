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

L2H_COMMON_OPTS=-t "Apcupsd User's Guide" -notransparent -white -toc_stars

MANUAL = manual

first_rule: apcupsd

apcupsd: texcheck tex web html dvipdf

.SUFFIXES:     .tex .html
.PHONY:
.DONTCARE:


tex:
	cp -fp ${IMAGES}/hires/*.eps .
	latex -interaction=batchmode apcupsd.tex	# initial run
	latex -interaction=batchmode apcupsd.tex	# build the TOC
	makeindex apcupsd.idx				# build the index
	latex -interaction=batchmode apcupsd.tex	# incorporate the index
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
	latex2html ${L2H_COMMON_OPTS} -noauto_link -no_subdir -split 0 \
		-nonavigation apcupsd >/dev/null
	./translate_images.pl apcupsd.html
	sed -e "s/HREF=\"apcupsd.html/HREF=\"/" apcupsd.html > manual.html
	cp manual.html apcupsd.html
	ln -sf apcupsd.html index.html
	rm -f *.eps

web:
	@echo "Making web"
	@mkdir -p apcupsd
	@rm -rf apcupsd/*
	cp ${IMAGES}/*.eps .
	cp ${IMAGES}/*.eps apcupsd/
	@rm -f next.*
	latex2html ${L2H_COMMON_OPTS} -split 4 -local_icons -long_titles 4 \
		-contents_in_nav apcupsd >/dev/null
	./translate_images.pl apcupsd/Apcupsd_Users_Guide.html
	cp apcupsd/Apcupsd_Users_Guide.html apcupsd/index.html
	rm -f apcupsd/*.old
	rm -f *.eps apcupsd/*.eps

dvipdf:
	@echo "Making dvi to pdf"
	@cp -fp ${IMAGES}/hires/*.eps .
	dvipdf apcupsd.dvi apcupsd.pdf
	@rm -f *.eps


# Does checking for the tex file
texcheck:
	./check_tex.pl apcupsd.tex

# Fixes problems with tex files
texfix:
	./fix_tex.pl apcupsd.tex
  
clean:
	@rm -f 1 2 3
	@rm -f *.aux *.cp *.fn *.ky *.log *.pg
	@rm -f *.backup *.ps *.dvi *.ilg *.lof *.lot
	@rm -f *.old WARNINGS *.out *.toc *.idx
	@rm -f images.pl labels.pl internals.pl
	@rm -f *.png *.pdf *.epf


distclean:  clean



