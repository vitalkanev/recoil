<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns="http://www.w3.org/1999/xhtml" version="1.0">
	<xsl:output method="xml" omit-xml-declaration="yes" />
	<xsl:variable name="formats" select="document('../formats.xml')/formats" />
	<xsl:variable name="version" select="$formats/@version" />

	<xsl:template match="/page">
		<html xml:lang="en">
			<head>
				<meta http-equiv="content-type" content="application/xhtml+xml; charset=UTF-8" />
				<title>
					<xsl:text>RECOIL</xsl:text>
					<xsl:if test="@title != 'Home'">
						<xsl:text> - </xsl:text>
						<xsl:value-of select="@title" />
					</xsl:if>
				</title>
				<style>
					html { background-color: #eee; color: #000; font-family: Georgia,"Times New Roman",Times,serif; }
					h1, h2 { color: #808; }
					a { color: #00c; }
					.tabs { border-bottom: solid #888 1px; margin: 0px; padding: 10px 0px; }
					.tabs li { background-color: #ddd; border: solid #888 1px; display: inline; margin-right: -1px; padding: 10px; }
					.tabs li.tab_selected { background-color: #fff; border-bottom-color: #fff; padding: 10px 20px; }
					.tabs li a { padding: 10px; text-decoration: none; }
					.content { background-color: #fff; border: solid #888 1px; border-top-style: none; padding: 10px; }
					table { border-collapse: collapse; }
					th, td { border: solid #888 1px; padding-left: 1ex; padding-right: 1ex; }
					th { background-color: #ddd; }
				</style>
				<xsl:apply-templates select="script" />
			</head>
			<body>
				<h1>RECOIL - Retro Computer Image Library</h1>
				<ul class="tabs">
					<xsl:call-template name="menu"><xsl:with-param name="page">Home</xsl:with-param></xsl:call-template>
					<xsl:call-template name="menu"><xsl:with-param name="page">Formats</xsl:with-param></xsl:call-template>
					<xsl:call-template name="menu"><xsl:with-param name="page">Download</xsl:with-param></xsl:call-template>
					<xsl:call-template name="menu"><xsl:with-param name="page">HTML5</xsl:with-param></xsl:call-template>
					<xsl:call-template name="menu"><xsl:with-param name="page">News</xsl:with-param></xsl:call-template>
					<xsl:call-template name="menu"><xsl:with-param name="page">Contact</xsl:with-param></xsl:call-template>
				</ul>
				<div class="content">
					<xsl:apply-templates select="*[not(self::script)]" />
				</div>
				<p><a href="https://sourceforge.net/p/recoil/" rel="nofollow"><img alt="Download RECOIL" src="https://sourceforge.net/sflogo.php?type=13&amp;group_id=258474" /></a></p>
			</body>
		</html>
	</xsl:template>

	<xsl:template name="menu">
		<xsl:param name="page" />
		<li>
			<xsl:choose>
				<xsl:when test="$page = /page/@title">
					<xsl:attribute name="class">tab_selected</xsl:attribute>
					<xsl:value-of select="$page" />
				</xsl:when>
				<xsl:when test="$page = 'Home'">
					<a href="/">Home</a>
				</xsl:when>
				<xsl:when test="$page = 'Download'">
					<a href="http://sourceforge.net/projects/recoil/files/recoil/{$version}/">Download</a>
				</xsl:when>
				<xsl:when test="$page = 'HTML5'">
					<a href="html5recoil.html">HTML5</a>
				</xsl:when>
				<xsl:otherwise>
					<a href="{translate($page, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')}.html">
						<xsl:value-of select="$page" />
					</a>
				</xsl:otherwise>
			</xsl:choose>
		</li>
	</xsl:template>

	<xsl:template match="version">
		<xsl:value-of select="$version" />
	</xsl:template>

	<xsl:template match="release">
		<h2>
			<xsl:value-of select="@project" />
			<xsl:text>&#160;</xsl:text>
			<xsl:value-of select="@version" />
			<xsl:text>&#160;(</xsl:text>
			<xsl:value-of select="@date" />
			<xsl:text>)</xsl:text>
		</h2>
		<xsl:apply-templates />
	</xsl:template>

	<xsl:template match="formats-count">
		<xsl:value-of select="count($formats/platform/format)" />
	</xsl:template>

	<xsl:template match="formats-index">
		<ul>
			<xsl:for-each select="$formats/platform">
				<li>
					<a href="#{translate(@name, ' +/', '-p-')}">
						<xsl:value-of select="count(format)" />
						<xsl:text> format</xsl:text>
						<xsl:if test="count(format) != 1">s</xsl:if>
					</a>
					<xsl:text> of </xsl:text>
					<a href="{@href}"><xsl:value-of select="@name" /></a>
				</li>
			</xsl:for-each>
		</ul>
	</xsl:template>

	<xsl:template match="formats-table">
		<table>
			<tr>
				<th>Platform</th>
				<th>Extension</th>
				<th>Description</th>
				<th>Resolution</th>
				<th>Depth</th>
				<th>Frames</th>
			</tr>
			<xsl:for-each select="$formats/platform/format">
				<tr>
					<xsl:if test="not(preceding-sibling::format)">
						<xsl:attribute name="id">
							<xsl:value-of select="translate(../@name, ' +/', '-p-')" />
						</xsl:attribute>
					</xsl:if>
					<td>
						<xsl:value-of select="../@name" />
						<xsl:if test="@hardware">
							<xsl:text> </xsl:text>
							<xsl:value-of select="@hardware" />
						</xsl:if>
					</td>
					<td>
						<xsl:for-each select="ext">
							<xsl:if test="position() != 1">, </xsl:if>
							<xsl:value-of select="." />
							<xsl:if test="@fake"> (arbitrary)</xsl:if>
						</xsl:for-each>
						<xsl:for-each select="companionExt">
							<xsl:text>+</xsl:text>
							<xsl:value-of select="." />
						</xsl:for-each>
					</td>
					<td>
						<xsl:value-of select="@name" />
					</td>
					<td>
						<xsl:choose>
							<xsl:when test="@maxWidth or @maxHeight">
								<xsl:text>up to </xsl:text>
								<xsl:value-of select="@width" />
								<xsl:value-of select="@maxWidth" />
								<xsl:text>x</xsl:text>
								<xsl:value-of select="@height" />
								<xsl:value-of select="@maxHeight" />
							</xsl:when>
							<xsl:when test="@width and @height">
								<xsl:value-of select="@width" />
								<xsl:text>x</xsl:text>
								<xsl:value-of select="@height" />
							</xsl:when>
						</xsl:choose>
						<xsl:if test="@unit">
							<xsl:text> </xsl:text>
							<xsl:value-of select="@unit" />
						</xsl:if>
					</td>
					<td>
						<xsl:choose>
							<xsl:when test="@colors = '2'">
								<xsl:text>mono</xsl:text>
							</xsl:when>
							<xsl:when test="@colors">
								<xsl:value-of select="@colors" />
								<xsl:text> colors</xsl:text>
							</xsl:when>
							<xsl:when test="@grayscale">
								<xsl:value-of select="@grayscale" />
								<xsl:text>-level grayscale</xsl:text>
							</xsl:when>
						</xsl:choose>
					</td>
					<td>
						<xsl:choose>
							<xsl:when test="@maxFrames = '2'">
								<xsl:text>1 or 2</xsl:text>
							</xsl:when>
							<xsl:when test="@frames">
								<xsl:value-of select="@frames" />
							</xsl:when>
							<xsl:otherwise>
								<xsl:text>1</xsl:text>
							</xsl:otherwise>
						</xsl:choose>
					</td>
				</tr>
			</xsl:for-each>
		</table>
	</xsl:template>

	<xsl:template match="authors">
		<h2>Authors</h2>
		<dl>
			<xsl:for-each select="author">
				<dt>
					<span>
						<xsl:if test="@rip">
							<xsl:attribute name="style">border: solid #000 1px;</xsl:attribute>
						</xsl:if>
						<xsl:value-of select="@name" />
					</span>
				</dt>
				<dd><xsl:apply-templates /></dd>
			</xsl:for-each>
		</dl>
	</xsl:template>

	<xsl:template match="ul[@title]">
		<p><xsl:value-of select="@title" />:</p>
		<ul>
			<xsl:apply-templates />
		</ul>
	</xsl:template>

	<xsl:template match="input[@type='file']">
		<input>
			<xsl:copy-of select="@*" />
			<xsl:attribute name="accept">
				<xsl:for-each select="document('../formats.xml')/formats/platform/format/*[(self::ext or self::companionExt) and not(. = following::ext) and not(. = following::companionExt)]">
					<xsl:sort />
					<xsl:if test="position() != 1">,</xsl:if>
					<xsl:text>.</xsl:text>
					<xsl:value-of select="translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')" />
				</xsl:for-each>
			</xsl:attribute>
		</input>
	</xsl:template>

	<xsl:template match="a[@href]|br|canvas|code|div|h2|img|input|li|ol|p|script|select|ul">
		<xsl:element name="{name()}">
			<xsl:copy-of select="@*" />
			<xsl:apply-templates />
		</xsl:element>
	</xsl:template>

	<xsl:template match="*" />
</xsl:stylesheet>
