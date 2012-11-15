VERSION = 1.4.0

FAIL_CC = mingw32-gcc -s -O2 -Wall -o $@ -I..
FAIL_CC_DLL = $(FAIL_CC) -shared -Wl,--kill-at -Wl,-subsystem,windows
FAIL_GPP = mingw32-g++ -static -s -O2 -Wall -o $@ -I..
WINDRES = windres -o $@
X64_FAIL_CC = x86_64-w64-mingw32-gcc -s -O2 -Wall -o $@ -I..
X64_FAIL_CC_DLL = $(X64_FAIL_CC) -shared -Wl,--kill-at -Wl,-subsystem,windows
X64_FAIL_GPP = x86_64-w64-mingw32-g++ -static -s -O2 -Wall -o $@ -I..
X64_WINDRES = x86_64-w64-mingw32-windres -o $@
PERL = perl
RM = rm -f
UNIX2DOS = unix2dos
ASCIIDOC_START = asciidoc -o -
ASCIIDOC_END = | sed -e "s/527bbd;/800080;/" | xmllint --valid --nonet -o $@ -
GIT = git
TAR = tar
SEVENZIP = 7z a -mx=9
CANDLE = candle -ext WixUtilExtension -nologo -o $@
LIGHT = light -ext WixUtilExtension -nologo -o $@
IMAGEMAGICK_X64_INST_DIR = c:\Program Files\ImageMagick-6.6.9-Q16

WIN32_BIN = fail2png.exe IM_MOD_RL_fail_.dll Xfail1.usr Xfail2.usr Xfail3.usr Xfail4.usr Xfail5.usr Xfail6.usr Xfail7.usr Xfail8.usr Xfail9.usr failwin.exe thumbfail.dll FAIL.plg

all: $(WIN32_BIN)

fail2png.exe: ../fail2png.c ../pngsave.c ../fail.c ../pngsave.h ../fail.h ../palette.h
	$(FAIL_CC) ../fail2png.c ../pngsave.c ../fail.c -static -lpng -lz

IM_MOD_RL_fail_.dll: ../fail.c ../fail.h ../palette.h ../failmagick.c
	$(FAIL_CC_DLL) ../fail.c ../failmagick.c -lCORE_RL_magick_
 
x64/IM_MOD_RL_fail_.dll: ../fail.c ../fail.h ../palette.h ../failmagick.c
	test -d x64 || mkdir x64
	$(X64_FAIL_CC_DLL) ../fail.c ../failmagick.c -I"$(IMAGEMAGICK_X64_INST_DIR)"/include -lCORE_RL_magick_ -L"$(IMAGEMAGICK_X64_INST_DIR)"

Xfail1.usr Xfail2.usr Xfail3.usr Xfail4.usr Xfail5.usr Xfail6.usr Xfail7.usr Xfail8.usr Xfail9.usr: xnview/Xfail.c ../fail.c ../fail.h ../palette.h
	$(FAIL_CC_DLL) "-DXNVIEW_FAIL_EXT=\"$(word $(@:Xfail%.usr=%),rip;gr8;mic;hip;tip;int;inp;apc;ap3;hr;hr2 pic;cpr;cin;cci;fnt;sxs;gr9;plm;ilc;mcp;ghg mch;ige;256;ap2;jgp;dgp;esc;pzm;ist;raw;rgb mgp;wnd;chr;shp;mbg;fwa;rm0;rm1;rm2;rm3;rm4 xlp;max;shc;all;app;sge;dlm;bkg;g09;bg9;apv spc;apl;gr7;g10;g11;art;drg;agp;pla;mis;4pl 4mi;4pm;pgf;pgc;pi1;pi2;pi3;pc1;pc2;pc3;neo doo;spu;tny;tn1;tn2;tn3;ca1;ca2;ca3;ing;pac sps;gfb;pi4;pi9)\"" \
		../fail.c xnview/Xfail.c
	
failwin.exe: failwin/failwin.c failwin/failwin-res.o ../pngsave.c ../fail.c failwin/failwin.h ../pngsave.h ../fail.h ../palette.h
	$(FAIL_CC) -Wl,-subsystem,windows failwin/failwin.c failwin/failwin-res.o ../pngsave.c ../fail.c -lcomctl32 -lcomdlg32 -lgdi32 -static -lpng -lz

failwin/failwin-res.o: failwin/failwin.rc failwin/failwin.h faileye.ico ../fail.h
	$(WINDRES) -I .. failwin/failwin.rc

thumbfail.dll: thumbfail/thumbfail.cpp ../fail.c ../fail.h ../palette.h
	$(FAIL_GPP) -shared -Wl,-subsystem,windows -Wl,--kill-at thumbfail/thumbfail.cpp ../fail.c -lgdi32 -lole32 -luuid

x64/thumbfail.dll: thumbfail/thumbfail.cpp ../fail.c ../fail.h ../palette.h
	test -d x64 || mkdir x64
	$(X64_FAIL_GPP) -shared -Wl,-subsystem,windows -Wl,--kill-at thumbfail/thumbfail.cpp ../fail.c -lgdi32 -lole32 -luuid

FAIL.plg: imagine/failimagine.c ../fail.c ../fail.h ../palette.h
	$(FAIL_CC_DLL) imagine/failimagine.c ../fail.c

x64/FAIL.plg64: imagine/failimagine.c ../fail.c ../fail.h ../palette.h
	$(X64_FAIL_CC_DLL) imagine/failimagine.c ../fail.c

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

../../fail-$(VERSION)-win32.msi: setup/fail.wixobj faileye.ico setup/license.rtf setup/Website.url $(WIN32_BIN) README_WindowsSetup.html
	$(LIGHT) -ext WixUIExtension -sice:ICE69 $<

setup/fail.wixobj: setup/fail.wxs
	$(CANDLE) -dVERSION=$(VERSION) $<

x64-setup: ../../fail-$(VERSION)-win64.msi

../../fail-$(VERSION)-win64.msi: x64/fail.wixobj faileye.ico setup/license.rtf x64/thumbfail.dll x64/IM_MOD_RL_fail_.dll x64/FAIL.plg64
	$(LIGHT) -ext WixUIExtension -sice:ICE69 -b setup $<

x64/fail.wixobj: setup/fail.wxs
	test -d x64 || mkdir x64
	$(CANDLE) -arch x64 -dVERSION=$(VERSION) $<

../MANIFEST:
	test -e ../.git && ( \
		($(GIT) ls-tree -r --name-only --full-tree master | grep -vF .gitignore \
			&& echo MANIFEST && echo README.html) | sort >$@; \
	)

srcdist: ../MANIFEST ../README.html
	$(RM) ../../fail-$(VERSION).tar.gz
	cd .. && $(TAR) -c --numeric-owner --owner=0 --group=0 --mode=644 -T MANIFEST --transform=s,,fail-$(VERSION)/, | $(SEVENZIP) -tgzip -si ../fail-$(VERSION).tar.gz

dist: srcdist ../../fail-$(VERSION)-win32.zip setup x64-setup

../../fail-$(VERSION)-win32.zip: COPYING.txt README_Windows.html $(WIN32_BIN)
	$(RM) ../../fail-$(VERSION)-win32.zip
	$(SEVENZIP) -tzip ../../fail-$(VERSION)-win32.zip \
		COPYING.txt README_Windows.html \
		fail2png.exe Xfail1.usr Xfail2.usr Xfail3.usr Xfail4.usr Xfail5.usr Xfail6.usr Xfail7.usr Xfail8.usr Xfail9.usr failwin.exe \
		IM_MOD_RL_fail_.dll

www: ../../www/index.html

../../www/index.html: ../README
	$(ASCIIDOC_START) -a failwww ../README $(ASCIIDOC_END)

clean:
	$(RM) $(WIN32_BIN) failwin/failwin-res.o ../palette.h COPYING.txt \
		../README.html README_Windows.html README_WindowsSetup.html setup/fail.wixobj x64/fail.wixobj x64/IM_MOD_RL_fail_.dll x64/thumbfail.dll x64/FAIL.plg64

.PHONY: all clean setup x64-setup ../MANIFEST srcdist dist www

.DELETE_ON_ERROR:
