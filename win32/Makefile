VERSION = 1.0.0

FAIL_CC = mingw32-gcc -s -O2 -Wall -o $@ -I..
FAIL_CC_DLL = $(FAIL_CC) -shared -Wl,--kill-at -Wl,-subsystem,windows
WINDRES = windres -o $@
PERL = perl
RM = rm -f
UNIX2DOS = unix2dos
ASCIIDOC = python C:/bin/asciidoc/asciidoc.py -o $@ -a doctime
ASCIIDOC_POSTPROCESS = $(PERL) -pi.bak -e "s/527bbd;/800080;/;END{unlink '$@.bak'}" $@
SEVENZIP = 7z a -mx=9

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

../README.html: ../README ../INSTALL
	$(ASCIIDOC) -a failsrc ../README
	$(ASCIIDOC_POSTPROCESS)

README_Windows.html: ../README USAGE
	$(ASCIIDOC) -a failwin ../README
	$(ASCIIDOC_POSTPROCESS)

srcdist: ../README.html
	$(RM) ../../fail-$(VERSION).tar.gz
	$(PERL) -x.. ../maketar.pl -d fail-$(VERSION) \
		-t fail.c fail.h \
		fail2png.c pngsave.c pngsave.h \
		win32/failwin/failwin.c win32/failwin/failwin.h win32/failwin/failwin.rc \
		win32/xnview/Xfail.c \
		Makefile win32/Makefile \
		COPYING README INSTALL README.html win32/USAGE \
		-s maketar.pl raw2c.pl \
		-b jakub.act win32/faileye.ico \
		| $(SEVENZIP) -tgzip -si ../../fail-$(VERSION).tar.gz

dist: srcdist all COPYING.txt README_Windows.html
	$(RM) ../../fail-$(VERSION)-win32.zip
	$(SEVENZIP) -tzip ../../fail-$(VERSION)-win32.zip \
		COPYING.txt README_Windows.html \
		fail2png.exe Xfail.dll failwin.exe

www: ../../www/index.html

../../www/index.html: ../README
	$(ASCIIDOC) -a failwww ../README
	$(ASCIIDOC_POSTPROCESS)

clean:
	$(RM) fail2png.exe Xfail.dll failwin.exe failwin/failwin-res.o ../palette.h COPYING.txt ../README.html README_Windows.html

.DELETE_ON_ERROR:
