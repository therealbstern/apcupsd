<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<xsl:param name="make.valid.html" select="1"/>
<xsl:param name="html.cleanup" select="1"/>
<xsl:param name="chunk.first.sections" select="1"/>
<xsl:param name="use.id.as.filename" select="1"/>
<xsl:param name="emphasis.propagates.style" select="1"/>
<xsl:param name="para.propagates.style" select="1"/>
<xsl:param name="phrase.propagates.style" select="1"/>
<xsl:param name="html.stylesheet">manual.css</xsl:param>
<xsl:param name="css.decoration">1</xsl:param>

<!-- Italicize the contents of epigraphs -->
<xsl:template match="epigraph">
  <div class="{name(.)}">
    <i><xsl:apply-templates select="para"/></i>
    <span>--<i><xsl:apply-templates select="attribution"/></i></span>
  </div>
</xsl:template>

<!-- Unsuppress bibliography abstracts -->
<xsl:template match="abstract" mode="bibliography.mode">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="application">
  <xsl:call-template name="inline.italicseq"/>
</xsl:template>

</xsl:stylesheet>
