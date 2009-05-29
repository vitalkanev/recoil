FAIL_CC = mingw32-gcc -s -O2 -Wall -o $@ -I..
PERL = perl

all: fail2png.exe

fail2png.exe: ../fail2png.c ../pngsave.c ../fail.c ../pngsave.h ../fail.h ../palette.h
	$(FAIL_CC) ../fail2png.c ../pngsave.c ../fail.c -lpng -lz

../palette.h: ../raw2c.pl ../jakub.act
	$(PERL) ../raw2c.pl ../jakub.act >$@

clean:
	rm fail2png.exe ../palette.h
