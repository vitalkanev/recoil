all: fail2png

fail2png: fail2png.c pngsave.c fail.c pngsave.h fail.h palette.h
	gcc -s -O2 -Wall -o $@ fail2png.c pngsave.c fail.c -lpng -lz -lm

palette.h: raw2c.pl jakub.act
	perl raw2c.pl jakub.act >$@

clean:
	rm -f fail2png palette.h
