<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="text" />
	<xsl:template match="/formats">
		<xsl:for-each select="platform/format/ext[not(. = following::ext)]">
			<xsl:sort />
			<xsl:value-of select="." />
			<xsl:text>&#10;</xsl:text>
		</xsl:for-each>
	</xsl:template>
</xsl:stylesheet>
