VERSION = 1.0.1

FAIL_CC = mingw32-gcc -s -O2 -Wall -o $@ -I..
FAIL_CC_DLL = $(FAIL_CC) -shared -Wl,--kill-at -Wl,-subsystem,windows
WINDRES = windres -o $@
PERL = perl
RM = rm -f
UNIX2DOS = unix2dos
ASCIIDOC_START = asciidoc -o - -a localtime -a doctime
ASCIIDOC_END = | sed -e "s/527bbd;/800080;/" | xmllint --valid --nonet -o $@ -
SEVENZIP = 7z a -mx=9
CANDLE = candle -nologo -o $@
LIGHT = light -nologo -o $@

all: fail2png.exe IM_MOD_RL_fail_.dll Xfail1.usr Xfail2.usr failwin.exe

fail2png.exe: ../fail2png.c ../pngsave.c ../fail.c ../pngsave.h ../fail.h ../palette.h
	$(FAIL_CC) ../fail2png.c ../pngsave.c ../fail.c -static -lpng12 -lz

IM_MOD_RL_fail_.dll: ../fail.c ../fail.h ../palette.h ../failmagick.c
	$(FAIL_CC_DLL) ../fail.c ../failmagick.c -lCORE_RL_magick_
 
Xfail1.usr Xfail2.usr: xnview/Xfail.c ../fail.c ../fail.h ../palette.h
	$(FAIL_CC_DLL) "-DXNVIEW_FAIL_EXT=$(if $(findstring 1,$@),\"rip;gr8;mic;hip;tip;int;inp;apc;ap3;hr\",\"pic;cpr;cin;cci;fnt;sxs;gr9;plm;ilc;mcp;ghg\")" \
		../fail.c xnview/Xfail.c
	
failwin.exe: failwin/failwin.c failwin/failwin-res.o ../pngsave.c ../fail.c failwin/failwin.h ../pngsave.h ../fail.h ../palette.h
	$(FAIL_CC) -Wl,-subsystem,windows failwin/failwin.c failwin/failwin-res.o ../pngsave.c ../fail.c -lcomctl32 -lcomdlg32 -lgdi32 -static -lpng12 -lz

failwin/failwin-res.o: failwin/failwin.rc failwin/failwin.h faileye.ico ../fail.h
	$(WINDRES) -I .. failwin/failwin.rc

../palette.h: ../raw2c.pl ../jakub.act
	$(PERL) ../raw2c.pl ../jakub.act >$@

COPYING.txt: ../COPYING
	$(UNIX2DOS) <$< >$@

../README.html: ../README ../INSTALL
	$(ASCIIDOC_START) -a failsrc ../README $(ASCIIDOC_END)

README_Windows.html: ../README USAGE
	$(ASCIIDOC_START) -a failwin ../README $(ASCIIDOC_END)

README_WindowsSetup.html: ../README USAGE
	$(ASCIIDOC_START) ../README $(ASCIIDOC_END)

setup: ../../fail-$(VERSION)-win32.msi

../../fail-$(VERSION)-win32.msi: setup/fail.wixobj faileye.ico setup/license.rtf setup/Website.url all README_WindowsSetup.html
	$(LIGHT) -ext WixUIExtension -sice:ICE69 $<

setup/fail.wixobj: setup/fail.wxs
	$(CANDLE) -dVERSION=$(VERSION) $<

srcdist: ../README.html
	$(RM) ../../fail-$(VERSION).tar.gz
	$(PERL) -x.. ../maketar.pl -d fail-$(VERSION) \
		-t fail.c fail.h \
		fail2png.c pngsave.c pngsave.h \
		win32/failwin/failwin.c win32/failwin/failwin.h win32/failwin/failwin.rc \
		win32/xnview/Xfail.c \
		win32/setup/fail.wxs win32/setup/license.rtf win32/setup/Website.url \
		failmagick.c \
		Makefile win32/Makefile \
		COPYING README INSTALL README.html win32/USAGE \
		-s maketar.pl raw2c.pl \
		-b jakub.act win32/faileye.ico \
		| $(SEVENZIP) -tgzip -si ../../fail-$(VERSION).tar.gz

dist: srcdist all COPYING.txt README_Windows.html setup
	$(RM) ../../fail-$(VERSION)-win32.zip
	$(SEVENZIP) -tzip ../../fail-$(VERSION)-win32.zip \
		COPYING.txt README_Windows.html \
		fail2png.exe Xfail1.usr Xfail2.usr failwin.exe \
		IM_MOD_RL_fail_.dll

www: ../../www/index.html

../../www/index.html: ../README
	$(ASCIIDOC_START) -a failwww ../README $(ASCIIDOC_END)

clean:
	$(RM) fail2png.exe Xfail1.usr Xfail2.usr IM_MOD_RL_fail_.dll failwin.exe failwin/failwin-res.o ../palette.h COPYING.txt ../README.html README_Windows.html README_WindowsSetup.html setup/fail.wixobj

.DELETE_ON_ERROR:
