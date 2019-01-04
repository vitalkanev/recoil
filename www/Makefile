TARGETS = index.html formats.html android.html windows.html macos.html linux.html web.html news.html contact.html privacy.html recoil.js

all: $(TARGETS)

%.html: %.xml www.xsl ../formats.xml
	xsltproc -o $@ www.xsl $< && java -jar C:/bin/vnu.jar $@

recoil.js: ../recoil.ci ../atari8.fnt ../altirrapal.pal ../c16.pal ../c64.fnt ../zx81.fnt
	cito -o $@ -l js-ta -I .. $<

clean:
	$(RM) $(TARGETS)

.PHONY: all clean

.DELETE_ON_ERROR:
