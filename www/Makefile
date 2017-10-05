all: index.html formats.html html5recoil.html news.html contact.html privacy.html recoil.js

index.html: www.xsl index.xml
	xsltproc --stringparam title Home $^ | xmllint --valid --nonet -o $@ -

formats.html: www.xsl ../formats.xml
	xsltproc --stringparam title Formats $^ | xmllint --valid --nonet -o $@ -

news.html: www.xsl news.xml
	xsltproc --stringparam title News $^ | xmllint --valid --nonet -o $@ -

contact.html: www.xsl contact.xml
	xsltproc --stringparam title Contact $^ | xmllint --valid --nonet -o $@ -

privacy.html: www.xsl privacy.xml
	xsltproc --stringparam title Privacy $^ | xmllint --valid --nonet -o $@ -

html5recoil.html: www.xsl html5recoil.xml ../formats.xml
	xsltproc --stringparam title HTML5 www.xsl html5recoil.xml | xmllint --dropdtd --schema /etc/xml/xhtml5.xsd --nonet -o $@ -

recoil.js: ../recoil.ci ../atari8.fnt ../altirrapal.pal ../c16.pal ../c64.fnt ../zx81.fnt
	cito -o $@ -l js-ta -I .. $<

clean:
	$(RM) index.html formats.html html5recoil.html news.html contact.html recoil.js

.PHONY: all clean

.DELETE_ON_ERROR:
