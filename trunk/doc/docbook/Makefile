#
#
#  Makefile for docbook
#

first_rule: all

out="/tmp/db_tmp"

.SUFFIXES:     .xml .kml
.PHONY:

install.xml: install.kml
	@echo "s%<b>%<emphasis role=\"bold\">%g" >${out}
	@echo "s%</b>%</emphasis>%g" >> ${out}
	@echo "s%<p>%<para>%g" >> ${out}
	@echo "s%</p>%</para>%g" >> ${out}
	@echo "s%<ol>%<orderedlist>%g" >> ${out}
	@echo "s%</ol>%</orderedlist>%g" >> ${out}
	@echo "s%<li>%<listitem>%g" >> ${out}
	@echo "s%</li>%</listitem>%g" >> ${out}
	sed -f ${out} $< >$@
	@rm -f ${out}

all:	manual.html web pdf

manual.html: install.xml manual.xsl manual.xml
	xmlto -m manual.xsl xhtml-nochunks manual.xml
	@rm -f install.xml
	@echo "html manual built"

web:	install.xml manual.xsl manual.xml
	xmlto -m manual.xsl -o html html manual.xml >/dev/null 2>&1
	@if [ $$? = 0 ] ; then \
	   echo "web html manual build"; \
	else \
	   echo "Error building web manual"; \
	fi
	@rm -f install.xml

pdf:	manual.html
	@echo "s%<a id=\"%<a name=\"%g" >${out}
	@echo "s%href=\"#%href=\"manual.html#%g" >>${out}
	@echo "s%Â%\&nspb;%g" >>${out}
	@cp -f manual.html broken_manual.html
	sed -f ${out} broken_manual.html >manual.html
	@rm -f broken_manual.html ${out}
	@echo "html manual sanitized for pdf production"
	  


main_configs:
	pic2graph -density 100 <main_configs.pic >main_configs.png

clean:
	rm -f 1

distclean:  clean
