<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="text" />
	<xsl:template match="/formats">
		<xsl:text>// Generated automatically from formats.xml and FilePicker.cs.xsl. Do not edit.

using System.Collections.Generic;
using Windows.Foundation;
using Windows.Storage;
using Windows.Storage.Pickers;

namespace RECOIL
{
	static class FilePicker
	{
		public static IAsyncOperation&lt;IReadOnlyList&lt;StorageFile&gt;&gt; PickFiles()
		{
			return new FileOpenPicker {
				FileTypeFilter = { </xsl:text>
				<xsl:for-each select="platform/format/ext[not(. = following::ext)]">
					<xsl:sort />
					<xsl:if test="position() != 1">, </xsl:if>
					<xsl:text>".</xsl:text>
					<xsl:value-of select="translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')" />
					<xsl:text>"</xsl:text>
				</xsl:for-each>
			<xsl:text> }
				}.PickMultipleFilesAsync();
		}
	}
}
</xsl:text>
	</xsl:template>
</xsl:stylesheet>
