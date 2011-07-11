<?xml version="1.0" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
	 xmlns="http://www.freedesktop.org/standards/shared-mime-info">
	<xsl:output method="xml" />
	<xsl:template match="/fail-types">
		<mime-info>
			<xsl:apply-templates select="type" />
		</mime-info>
	</xsl:template>
	<xsl:template match="type">
		<xsl:variable name="EXT" select="translate(@ext, 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ')" />
		<mime-type type="image/x-{@ext}">
			<comment><xsl:value-of select="$EXT" /> image</comment>
			<!-- based on image/png from freedesktop.org.xml -->
			<comment xml:lang="ara">صورة <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="az"><xsl:value-of select="$EXT" /> rəsmi</comment>
			<comment xml:lang="be">Vyjava <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="be@latin">Vyjava <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="bg">Изображение — <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="bs"><xsl:value-of select="$EXT" /> slika</comment>
			<comment xml:lang="ca">imatge <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="cs">Obrázek <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="cy">Delwedd <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="da"><xsl:value-of select="$EXT" />-billede</comment>
			<comment xml:lang="de"><xsl:value-of select="$EXT" />-Bild</comment>
			<comment xml:lang="el">εικόνα <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="en_GB"><xsl:value-of select="$EXT" /> image</comment>
			<comment xml:lang="eo"><xsl:value-of select="$EXT" />-bildo</comment>
			<comment xml:lang="es">Imagen <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="eu"><xsl:value-of select="$EXT" /> irudia</comment>
			<comment xml:lang="fi"><xsl:value-of select="$EXT" />-kuva</comment>
			<comment xml:lang="fr">image <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="ga">íomhá <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="hu"><xsl:value-of select="$EXT" />-kép</comment>
			<comment xml:lang="id">Citra <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="it">Immagine <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="ja"><xsl:value-of select="$EXT" /> 画像</comment>
			<comment xml:lang="kk"><xsl:value-of select="$EXT" /> суреті</comment>
			<comment xml:lang="ko"><xsl:value-of select="$EXT" /> 그림</comment>
			<comment xml:lang="lt"><xsl:value-of select="$EXT" /> paveikslėlis</comment>
			<comment xml:lang="lv"><xsl:value-of select="$EXT" /> attēls</comment>
			<comment xml:lang="ms">Imej <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="nb"><xsl:value-of select="$EXT" />-bilde</comment>
			<comment xml:lang="nl"><xsl:value-of select="$EXT" />-afbeelding</comment>
			<comment xml:lang="nn"><xsl:value-of select="$EXT" />-bilete</comment>
			<comment xml:lang="no"><xsl:value-of select="$EXT" />-bilde</comment>
			<comment xml:lang="oc">Imatge <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="pl">Obraz <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="pt">imagem <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="pt_BR">Imagem <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="ro">Imagine <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="ru">изображение <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="sk">Obrázok <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="sq">Figurë <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="sr"><xsl:value-of select="$EXT" /> слика</comment>
			<comment xml:lang="sv"><xsl:value-of select="$EXT" />-bild</comment>
			<comment xml:lang="tr"><xsl:value-of select="$EXT" /> görüntüsü</comment>
			<comment xml:lang="uk">зображення <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="vi">Ảnh <xsl:value-of select="$EXT" /></comment>
			<comment xml:lang="zh_CN"><xsl:value-of select="$EXT" /> 图像</comment>
			<comment xml:lang="zh_TW"><xsl:value-of select="$EXT" /> 圖片</comment>
			<glob pattern="*.{@ext}" />
			<xsl:if test="@acronym">
				<acronym><xsl:value-of select="translate(@acronym, &quot; ',0123456789abcdefghijklmnopqrstuvwxyz&quot;, '')" /></acronym>
				<expanded-acronym><xsl:value-of select="@acronym" /></expanded-acronym>
			</xsl:if>
		</mime-type>
	</xsl:template>
</xsl:stylesheet>
