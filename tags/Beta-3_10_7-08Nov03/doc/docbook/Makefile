#
#
#  Makefile for docbook
#

out="/tmp/db_tmp"

all:	html web pdf

html:	manual.xsl manual.xml
	xmlto -m manual.xsl xhtml-nochunks manual.xml

web:	manual.xsl manual.xml
	xmlto -m manual.xsl -o html html manual.xml

pdf:
	echo "s%<a id=\"%<a name=\"%g" >${out}
	echo "s%href=\"#%href=\"manual.html#%g" >>${out}
	echo "s%Â%\&nspb;%g" >>${out}
	rm -f 1
	cp manual.html 1
	sed -f ${out} 1 >manual.html
	@rm -f 1 ${out}
	  


main_configs:
	pic2graph -density 100 <main_configs.pic >main_configs.png

clean:
	rm -f 1

distclean:  clean
