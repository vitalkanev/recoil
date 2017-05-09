<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="text" />
	<xsl:template match="/formats">
		<xsl:text>[Thumbnailer Entry]
Exec=recoil2png -o %o %i
MimeType=</xsl:text>
		<xsl:for-each select="platform/format/ext[not(. = following::ext)]">
			<xsl:sort />
			<xsl:if test="position() != 1">;</xsl:if>
			<xsl:text>image/x-</xsl:text>
			<xsl:value-of select="translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')" />
		</xsl:for-each>
		<xsl:text>&#10;</xsl:text>
	</xsl:template>
</xsl:stylesheet>
