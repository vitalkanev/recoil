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

.DELETE_ON_ERROR:
