PREFIX = /usr/local

all: fail2png

fail2png: fail2png.c pngsave.c fail.c pngsave.h fail.h palette.h
	gcc -s -O2 -Wall -o $@ fail2png.c pngsave.c fail.c -lpng -lz -lm

palette.h: raw2c.pl jakub.act
	perl raw2c.pl jakub.act >$@

README.html: README
	asciidoc -o $@ -a doctime -a failsrc README
	perl -pi -e 's/527bbd;/800080;/' $@

clean:
	rm -f fail2png palette.h

install: fail2png
	mkdir -p $(PREFIX)/bin
	cp -f fail2png $(PREFIX)/bin
	chmod 755 $(PREFIX)/bin/fail2png

uninstall:
	rm -f $(PREFIX)/bin/fail2png

.DELETE_ON_ERROR:
