<?xml version="1.0"?>
<!--
    texi2latex.xsl - main XSLT stylesheet for converting Texinfo to LaTeX.
	$Id: texi2latex.xsl,v 1.1 2005-03-13 21:43:23 kerns Exp $	

    Copyright © 2004, 2005 Torsten Bronger <bronger@physik.rwth-aachen.de>.

    This file is part of texi2latex.

    texi2latex is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    texi2latex is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along
    with texi2latex; if not, write to the Free Software Foundation, Inc., 59
    Temple Place, Suite 330, Boston, MA 02111-1307 USA
-->

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<!-- The output encoding is UTF-8.  This is not because inputenc's UTF-8
     option is supporsed to be used, neither for the ucs package, but because
     the output LaTeX document contains a lot of control characters (see
     text-nodes.mod.xsl for details), and this is sent through tbrplent for
     substituting real LaTeX commands for all UTF-8 sequences.

     Please note that it may happen that a special package must be loaded for
     very rare characters.  If this is not done already you get an error
     message by LaTeX.  In this ase you have to load the package yoursef via
     the file texi2latex.cfg, that you have to provide in the current
     directory (or where LaTeX can find it).

     For efurther info see the documentation of texi2latex. -->

<xsl:output method="text" indent="no" encoding="utf-8"/>

<!-- Here all elements should be listed that have no direct #PCDATA in its
     contents models -->

<xsl:strip-space elements="multitable columnfraction thead tbody row float node"/>

<!-- Now we include all modules -->

<xsl:include href="text-nodes.mod.xsl"/>
<xsl:include href="inline.mod.xsl"/>
<xsl:include href="structuring.mod.xsl"/>
<xsl:include href="lists.mod.xsl"/>
<xsl:include href="tables.mod.xsl"/>
<xsl:include href="blocks.mod.xsl"/>
<xsl:include href="urls.mod.xsl"/>
<xsl:include href="common.mod.xsl"/>
<xsl:include href="images-floats.mod.xsl"/>
<xsl:include href="index.mod.xsl"/>
<xsl:include href="xrefs.mod.xsl"/>
<xsl:include href="i18n.mod.xsl"/>
<xsl:include href="titlepage.mod.xsl"/>
<xsl:include href="preamble.mod.xsl"/>

<!-- I don't know whether this is the default template anyway, but be that as
     it may: I make it explicit.  So if an element is not matched, simply all
     child nodes are matched. -->

<xsl:template match="*">
  <xsl:apply-templates select="node()|@*"/>
</xsl:template>


<!-- In itemizations it is bad to have the \par *before* the paragraph.  But
     normally it it okay this way, and I've made excellent experiences with
     this technique in tbook. -->

<xsl:template match="para">
  <xsl:text>\par </xsl:text>
  <xsl:if test="@role = 'continues'">
    <xsl:text>\noindent </xsl:text>
  </xsl:if>
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="sp">
  <xsl:value-of select="concat('\vspace{',@lines,'\baselineskip}')"/>
</xsl:template>

</xsl:stylesheet>
