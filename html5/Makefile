all: fail.js html5fail.html

fail.js: ../fail.ci ../atari8.fnt ../jakub.act
	cito -o $@ -I .. $<

html5fail.html: html5fail.txt
	asciidoc -o - $< | sed -e "s/527bbd;/800080;/" > $@

clean:
	$(RM) fail.js html5fail.html

.PHONY: all clean

.DELETE_ON_ERROR:
