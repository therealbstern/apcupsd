#
#
#  Makefile for docbook
#

first_rule: all

out=/tmp/db_tmp

# Source files
SRC= \
    appendix.hml \
    bible.hml \
    cables.hml \
    config_examples.hml \
    directives.hml \
    events.hml \
    faq.hml \
    install.hml \
    logging.hml \
    maintaining.hml \
    manual.hml \
    master_slave.hml \
    monitoring.hml \
    multiple_upses.hml \
    nis.hml \
    post_install.hml \
    release_notes.hml \
    rpm-faq.hml \
    serial.hml \
    shutdown.hml \
    snmp.hml \
    testing.hml \
    troubleshooting.hml \
    usage.hml \
    win32.hml

OBJ = \
    appendix.xml \
    bible.xml \
    cables.xml \
    config_examples.xml \
    directives.xml \
    events.xml \
    faq.xml \
    install.xml \
    logging.xml \
    maintaining.xml \
    manual.xml \
    master_slave.xml \
    monitoring.xml \
    multiple_upses.xml \
    nis.xml \
    post_install.xml \
    release_notes.xml \
    rpm-faq.xml \
    serial.xml \
    shutdown.xml \
    snmp.xml \
    testing.xml \
    troubleshooting.xml \
    usage.xml \
    win32.xml

.SUFFIXES:     .xml .hml
.PHONY:

.hml.xml:
	@echo "s%<b>%<emphasis role=\"bold\">%g" >${out}
	@echo "s%</b>%</emphasis>%g" >> ${out}
	@echo "s%<p>%<para>%g" >> ${out}
	@echo "s%</p>%</para>%g" >> ${out}
	@echo "s%<ol>%<orderedlist>%g" >> ${out}
	@echo "s%</ol>%</orderedlist>%g" >> ${out}
	@echo "s%<li>%<listitem><para>%g" >> ${out}
	@echo "s%</li>%</para></listitem>%g" >> ${out}
	@echo "s%<pre>%<programlisting>%g" >> ${out}
	@echo "s%</pre>%</programlisting>%g" >> ${out}
	sed -f ${out} $< >$@
	@rm -f ${out}


all:	manual.html web pdf

manual.html: manual.xsl $(OBJ)
	xmlto -m manual.xsl xhtml-nochunks manual.xml
	@echo "html manual built"

web:	manual.xsl $(OBJ)
	xmlto -m manual.xsl -o html html manual.xml
	@if [ $$? = 0 ] ; then \
	   echo "web html manual build"; \
	else \
	   echo "Error building web manual"; \
	fi
#	@rm -f *.xml
# Cleanup crappy html
	@echo "s%Â% %g" >${out}
	@echo "s% % %g" >>${out}
	@(cd html; for i in *.html; do \
	    cp -f $$i 1; \
	    sed -f ${out} 1 >$$i; \
	  done )
	@rm -f html/1
	@echo "html manual sanitized for web use"

	   

pdf:	manual.html
# Cleanup more crappy html
	@echo "s%<a id=\"%<a name=\"%g" >${out}
	@echo "s%href=\"#%href=\"manual.html#%g" >>${out}
	@echo "s%Â%\&nspb;%g" >>${out}
	@echo "s% %\&nspb;%g" >>${out}
	@cp -f manual.html broken_manual.html
	sed -f ${out} broken_manual.html >manual.html
	@rm -f broken_manual.html ${out}
	@echo "html manual sanitized for pdf production"
	  
xml:	manual.xsl $(OBJ)

main_configs:
	pic2graph -density 100 <main_configs.pic >main_configs.png

clean:
	rm -f 1 *.xml

distclean:  clean
