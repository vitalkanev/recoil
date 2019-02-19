<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html" indent="no" />
	<xsl:variable name="formats" select="document('../formats.xml')/formats" />
	<xsl:variable name="version" select="$formats/@version" />

	<xsl:template match="/page">
		<xsl:text disable-output-escaping="yes">&lt;!DOCTYPE html&gt;
</xsl:text>
		<html lang="en">
			<head>
				<title>
					<xsl:text>RECOIL</xsl:text>
					<xsl:if test="@title != 'Home'">
						<xsl:text> - </xsl:text>
						<xsl:value-of select="@title" />
					</xsl:if>
				</title>
				<style>
					html { background-color: #eee; color: #000; font-family: Segoe UI,Helvetica,Arial,sans-serif; padding: 0em 3em; }
					h1, h2 { color: #006374; }
					h1 { font-family: Georgia,"Times New Roman",Times,serif; }
					a { color: #00c; }
					nav ul { border-bottom: solid #ccc 1px; margin: 0px; padding: 0.5em 0em; }
					nav li { display: inline; }
					nav li.tab_selected { background-color: #fff; border: solid #ccc 1px; border-bottom-color: #fff; padding: 0.5em; }
					nav li a { padding: 0.5em; text-decoration: none; }
					main { background-color: #fff; border: solid #ccc 1px; border-top-style: none; padding: 1em 3em 3em 3em; }
					table.formats { border-collapse: collapse; }
					table.formats th, table.formats td { border: solid #aaa 1px; padding-left: 1em; padding-right: 1em; }
					th { background-color: #ddd; }
					.author { color: #006374; padding-right: 1em; text-align: right; }
					.rip { border: solid #000 1px; padding-left: 2px; padding-right: 2px; color: #000; }
				</style>
				<xsl:apply-templates select="script" />
			</head>
			<body>
				<header>
					<h1>RECOIL - Retro Computer Image Library</h1>
				</header>
				<nav>
					<ul>
						<xsl:call-template name="menu"><xsl:with-param name="page">Home</xsl:with-param></xsl:call-template>
						<xsl:call-template name="menu"><xsl:with-param name="page">Formats</xsl:with-param></xsl:call-template>
						<xsl:call-template name="menu"><xsl:with-param name="page">Android</xsl:with-param></xsl:call-template>
						<xsl:call-template name="menu"><xsl:with-param name="page">Windows</xsl:with-param></xsl:call-template>
						<xsl:call-template name="menu"><xsl:with-param name="page">macOS</xsl:with-param></xsl:call-template>
						<xsl:call-template name="menu"><xsl:with-param name="page">Linux</xsl:with-param></xsl:call-template>
						<xsl:call-template name="menu"><xsl:with-param name="page">Web</xsl:with-param></xsl:call-template>
						<xsl:call-template name="menu"><xsl:with-param name="page">News</xsl:with-param></xsl:call-template>
						<xsl:call-template name="menu"><xsl:with-param name="page">Contact</xsl:with-param></xsl:call-template>
					</ul>
				</nav>
				<main>
					<xsl:apply-templates select="*[not(self::script)]" />
				</main>
				<footer>
					<p>
						<a href="https://sourceforge.net/p/recoil/">
							<img alt="Download RECOIL" src="https://sourceforge.net/sflogo.php?type=13&amp;group_id=258474" />
						</a>
					</p>
				</footer>
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
		<table class="formats">
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

	<xsl:template match="download">
		<xsl:variable name="file" select="concat(@prefix, $version, @suffix)" />
		<a href="https://sourceforge.net/projects/recoil/files/recoil/{$version}/{$file}/download"><xsl:value-of select="$file" /></a>
	</xsl:template>

	<xsl:template match="recoil2png">
		<p>When you run <code>recoil2png</code> with just the input filenames, it writes the corresponding PNGs
		with the same names and locations and the extensions changed to <code>png</code>.</p>
		<p>To see what options are available, run the program without arguments.</p>
	</xsl:template>

	<xsl:template match="authors">
		<h2>Authors</h2>
		<table>
			<xsl:for-each select="author">
				<tr>
					<td class="author">
						<xsl:choose>
							<xsl:when test="@rip">
								<span class="rip"><xsl:value-of select="@name" /></span>
							</xsl:when>
							<xsl:when test="@href">
								<a href="{@href}"><xsl:value-of select="@name" /></a>
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="@name" />
							</xsl:otherwise>
						</xsl:choose>
					</td>
					<td>
						<xsl:apply-templates />
					</td>
				</tr>
			</xsl:for-each>
		</table>
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

	<xsl:template match="a[@href]|br|canvas|code|div|h2|img|input|li|ol|p|script|select|span|ul">
		<xsl:element name="{name()}">
			<xsl:copy-of select="@*" />
			<xsl:apply-templates />
		</xsl:element>
	</xsl:template>

	<xsl:template match="*" />
</xsl:stylesheet>
