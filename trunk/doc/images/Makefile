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

IMAGES=.

IMGLST=  \
	apcupsd-logo \
	apcupsd \
	back \
	charging \
	home \
	img1 \
	img10 \
	img11 \
	img12 \
	img13 \
	img14 \
	img2 \
	img3 \
	img4 \
	img5 \
	img6 \
	img7 \
	img8 \
	img9 \
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
	wininstall7 \

first_rule: images

images: converteps

# Copied image conversion stuff from Bacula manual Makefile
# Convert any png images to eps (for LaTeX)
converteps:
	(for i in ${IMGLST} ; do \
	   pngtopnm $${i}.png | pnmtops -scale=0.65 --noturn -nosetpage >$${i}.eps; \
	   ./image_convert.pl --scalefile=image_scales --defaultscale=0.65 --resolution=140 --maxx=5 --maxy=8 $${i}.png hires/$${i}.eps; \
	 done)
   
clean:
	@rm -f *.eps hires/*.eps


distclean:  clean






