TARGETS = index.html formats.html android.html windows.html macos.html linux.html web.html news.html contact.html privacy.html recoil.js recoil-180x180.png

all: $(TARGETS)

%.html: %.xml www.xsl ../formats.xml
	xsltproc -o $@ www.xsl $< && java -jar C:/bin/vnu.jar $@

recoil.js: ../recoil.ci ../atari8.fnt ../altirrapal.pal ../c16.pal ../c64.fnt ../zx81.fnt
	cito -o $@ -I .. $<

recoil-180x180.png: ../recoil-512x512.png
	magick $< -resize 180x180 -quality 95 -strip $@

clean:
	$(RM) $(TARGETS)

.PHONY: all clean

.DELETE_ON_ERROR:
