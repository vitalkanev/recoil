FAIL_CC = mingw32-gcc -s -O2 -Wall -o $@ -I..
FAIL_CC_DLL = $(FAIL_CC) -shared -Wl,--kill-at -Wl,-subsystem,windows
WINDRES = windres -o $@
PERL = perl
UNIX2DOS = unix2dos
ASCIIDOC = python C:/bin/asciidoc/asciidoc.py -o $@ -a doctime
ASCIIDOC_POSTPROCESS = $(PERL) -pi.bak -e "s/527bbd;/800080;/;END{unlink '$@.bak'}" $@

all: fail2png.exe Xfail.dll failwin.exe

fail2png.exe: ../fail2png.c ../pngsave.c ../fail.c ../pngsave.h ../fail.h ../palette.h
	$(FAIL_CC) ../fail2png.c ../pngsave.c ../fail.c -lpng -lz

Xfail.dll: xnview/Xfail.c ../fail.c ../fail.h ../palette.h
	$(FAIL_CC_DLL) ../fail.c xnview/Xfail.c

failwin.exe: failwin/failwin.c failwin/failwin-res.o ../pngsave.c ../fail.c failwin/failwin.h ../pngsave.h ../fail.h ../palette.h
	$(FAIL_CC) -Wl,-subsystem,windows failwin/failwin.c failwin/failwin-res.o ../pngsave.c ../fail.c -lcomctl32 -lcomdlg32 -lgdi32 -lpng -lz

failwin/failwin-res.o: failwin/failwin.rc failwin/failwin.h faileye.ico
	$(WINDRES) failwin/failwin.rc

../palette.h: ../raw2c.pl ../jakub.act
	$(PERL) ../raw2c.pl ../jakub.act >$@

COPYING.txt: ../COPYING
	$(UNIX2DOS) <$< >$@

../README.html: ../README
	$(ASCIIDOC) ../README
	$(ASCIIDOC_POSTPROCESS)

clean:
	rm fail2png.exe Xfail.dll failwin.exe failwin/failwin-res.o ../palette.h COPYING.txt ../README.html

.DELETE_ON_ERROR:
