#
#
#  Makefile for texi
#

IMAGES = apcupsd \
	 back \
	 charging \
	 home \
	 main_configs \
	 multimon \
	 next \
	 onbatt \
	 online \
	 status \
	 thanks \
	 wininstall1 \
	 wininstall2 \
	 wininstall3 \
	 wininstall4 \
	 wininstall5 \
	 wininstall6 \
	 wininstall7  


first_rule: all

.SUFFIXES:     .texi .html .pdf
.PHONY:

# pdf does not yet work because there are no pdf image
#   files.

all:	html web pdf

html:	manual.texi
#	makeinfo --html --no-split --no-headers --number-sections  manual.texi >manual.html
	makeinfo --html --no-split --number-sections  manual.texi >manual.html
	@echo "html manual built"

web:	manual.texi
	rm -rf manual
	makeinfo --html --number-sections  manual.texi
	cp -p *.png manual/
	@echo "web manual built"

test:	test.texi
	makeinfo --html --no-split test.texi

#	   
# As is typical of tex, this part of generating the
#  manual produces a lot of debug and warning output.
#
pdf:	manual.texi
	texi2dvi --pdf manual.texi
	@rm -f manual.aux manual.cp manual.fn manual.ky manual.log manual.pg
	@rm -f manual.toc manual.tp manual.vr manual.cps
	@echo "pdf manual built"
	  
main_configs:
	pic2graph -density 100 <main_configs.pic >main_configs.png

convert:	  
	(for i in ${IMAGES} ; do \
	   pngtopnm $${i}.png | pnmtops -scale=0.85 --noturn -nosetpage | epstopdf --filter >$${i}.pdf ; \
	   done)

clean:
	@rm -f 1 2 3
	@rm -f manual.aux manual.cp manual.fn manual.ky manual.log manual.pg
	@rm -f manual.toc manual.tp manual.vr

distclean:  clean
