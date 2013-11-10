VERSION = 3.0.0

CITO = cito
RECOIL_CC = mingw32-gcc -s -O2 -Wall -o $@ -I..
RECOIL_CC_DLL = $(RECOIL_CC) -shared -Wl,--kill-at -Wl,-subsystem,windows
RECOIL_GPP = mingw32-g++ -static -s -O2 -Wall -o $@ -I..
WINDRES = windres -o $@
X64_RECOIL_CC = x86_64-w64-mingw32-gcc -s -O2 -Wall -o $@ -I..
X64_RECOIL_CC_DLL = $(X64_RECOIL_CC) -shared -Wl,--kill-at -Wl,-subsystem,windows
X64_RECOIL_GPP = x86_64-w64-mingw32-g++ -static -s -O2 -Wall -o $@ -I..
X64_WINDRES = x86_64-w64-mingw32-windres -o $@
CSC = C:/Windows/Microsoft.NET/Framework/v3.5/csc.exe -nologo -o+ -out:$@
RM = rm -f
UNIX2DOS = unix2dos
ASCIIDOC_START = asciidoc -o -
ASCIIDOC_END = | sed -e "s/527bbd;/800080;/" | xmllint --valid --nonet -o $@ -
GIT = git
TAR = tar
SEVENZIP = 7z a -mx=9
CANDLE = candle -ext WixUtilExtension -nologo -o $@
LIGHT = light -ext WixUtilExtension -nologo -o $@ -spdb
IMAGEMAGICK_X64_INST_DIR = c:\Program Files\ImageMagick-6.8.2-Q16
PAINT_NET_DIR = c:\Program Files\Paint.NET

WIN32_BIN = recoil2png.exe IM_MOD_RL_recoil_.dll Xrecoil1.usr Xrecoil2.usr Xrecoil3.usr Xrecoil4.usr Xrecoil5.usr Xrecoil6.usr Xrecoil7.usr Xrecoil8.usr Xrecoil9.usr Xrecoil10.usr Xrecoil11.usr Xrecoil12.usr Xrecoil13.usr Xrecoil14.usr Xrecoil15.usr Xrecoil16.usr Xrecoil17.usr Xrecoil18.usr recoilwin.exe RECOIL.plg RecoilPaintDotNet.dll

all: $(WIN32_BIN) thumbrecoil.dll

recoil2png.exe: ../recoil2png.c ../pngsave.c ../pngsave.h ../recoil.c ../recoil.h
	$(RECOIL_CC) ../recoil2png.c ../pngsave.c ../recoil.c -static -lpng -lz

IM_MOD_RL_recoil_.dll: ../recoilmagick.c ../recoil.c ../recoil.h
	$(RECOIL_CC_DLL) ../recoilmagick.c ../recoil.c -lCORE_RL_magick_
 
x64/IM_MOD_RL_recoil_.dll: ../recoilmagick.c ../recoil.c ../recoil.h
	test -d x64 || mkdir x64
	$(X64_RECOIL_CC_DLL) ../recoilmagick.c ../recoil.c -I"$(IMAGEMAGICK_X64_INST_DIR)"/include -lCORE_RL_magick_ -L"$(IMAGEMAGICK_X64_INST_DIR)"

Xrecoil1.usr Xrecoil2.usr Xrecoil3.usr Xrecoil4.usr Xrecoil5.usr Xrecoil6.usr Xrecoil7.usr Xrecoil8.usr Xrecoil9.usr Xrecoil10.usr Xrecoil11.usr Xrecoil12.usr Xrecoil13.usr Xrecoil14.usr Xrecoil15.usr Xrecoil16.usr Xrecoil17.usr Xrecoil18.usr: xnview/Xrecoil.c ../recoil.c ../recoil.h
	$(RECOIL_CC_DLL) "-DXNVIEW_RECOIL_EXTS=\"$(word $(@:Xrecoil%.usr=%),rip;gr8;mic;hip;tip;int;inp;apc;ap3;hr;hr2 pic;cpr;cin;cci;fnt;sxs;gr9;plm;ilc;mcp;ghg mch;ige;256;ap2;jgp;dgp;esc;pzm;ist;raw;rgb mgp;wnd;chr;shp;mbg;fwa;rm0;rm1;rm2;rm3;rm4 xlp;max;shc;all;app;sge;dlm;bkg;g09;bg9;apv spc;apl;gr7;g10;g11;art;drg;agp;pla;mis;4pl 4mi;4pm;pgf;pgc;pi1;pi2;pi3;pc1;pc2;pc3;neo doo;spu;tny;tn1;tn2;tn3;ca1;ca2;ca3;ing;pac sps;gfb;pi4;pi9;dgu;dg1;trp;tru;god;ftc;del dph;ipc;nlq;pmd;acs;pcs;hpm;mcs;a4r;dc1;dgc cpt;iff;bl1;bl2;bl3;bru;ip2;imn;icn;din;vzi irg;ir2;ice;img;ximg;mpp;mc;mg1;mg2;mg4;mg8 scr;atr;ifl;ch4;ch6;ch8;leo;acbm;zxp;hlr;lbm cdu;che;cwg;dd;dol;drz;fgs;fpt;gcd;gig;gih hbm;hed;hir;iph;ipt;ism;koa;mil;mon;ocp;p64 pi;pmg;rp;rpm;sar;vid;sif;drl;mci;ami;gg jj;64c;afl;bfli;bml;dlp;drp;eci;ffli;fli;fp2 fun;gun;hfc;ecp;dgi;lum;bgp;hpc;aas)\"" \
		xnview/Xrecoil.c ../recoil.c

recoilwin.exe: recoilwin/recoilwin.c recoilwin/recoilwin.h recoilwin/recoilwin-res.o ../pngsave.c ../pngsave.h ../formats.h ../recoil.c ../recoil.h
	$(RECOIL_CC) -Wl,-subsystem,windows recoilwin/recoilwin.c recoilwin/recoilwin-res.o ../pngsave.c ../recoil.c -lcomctl32 -lcomdlg32 -lgdi32 -static -lpng -lz

recoilwin/recoilwin-res.o: recoilwin/recoilwin.rc recoilwin/recoilwin.h faileye.ico ../recoil.h
	$(WINDRES) -I .. recoilwin/recoilwin.rc

thumbrecoil.dll: thumbrecoil/thumbrecoil.cpp ../formats.h ../recoil.c ../recoil.h
	$(RECOIL_GPP) -shared -Wl,-subsystem,windows -Wl,--kill-at thumbrecoil/thumbrecoil.cpp ../recoil.c -lgdi32 -lole32 -luuid

x64/thumbrecoil.dll: thumbrecoil/thumbrecoil.cpp ../formats.h ../recoil.c ../recoil.h
	test -d x64 || mkdir x64
	$(X64_RECOIL_GPP) -shared -Wl,-subsystem,windows -Wl,--kill-at thumbrecoil/thumbrecoil.cpp ../recoil.c -lgdi32 -lole32 -luuid

RECOIL.plg: imagine/recoilimagine.c ../formats.h ../recoil.c ../recoil.h
	$(RECOIL_CC_DLL) imagine/recoilimagine.c ../recoil.c

x64/RECOIL.plg64: imagine/recoilimagine.c ../formats.h ../recoil.c ../recoil.h
	test -d x64 || mkdir x64
	$(X64_RECOIL_CC_DLL) imagine/recoilimagine.c ../recoil.c

RecoilPaintDotNet.dll: paint.net/RecoilPaintDotNet.cs paint.net/RECOIL.cs
	$(CSC) -t:library paint.net\\RecoilPaintDotNet.cs paint.net\\RECOIL.cs -r:"$(PAINT_NET_DIR)\PaintDotNet.Base.dll" -r:"$(PAINT_NET_DIR)\PaintDotNet.Data.dll"

paint.net/RECOIL.cs: ../recoil.ci ../atari8.fnt ../jakub.act
	$(CITO) -o $@ -I .. -n Recoil $<

../formats.h: ../formats.h.xsl ../formats.xml
	xsltproc -o $@ ../formats.h.xsl ../formats.xml

# http://www.cmcrossroads.com/article/rules-multiple-outputs-gnu-make
%.c %.h: %.ci ../atari8.fnt ../jakub.act
	$(CITO) -o $*.c -I .. $<

COPYING.txt: ../COPYING
	$(UNIX2DOS) <$< >$@

../README.html: ../README ../INSTALL
	$(ASCIIDOC_START) -a recoilsrc ../README $(ASCIIDOC_END)

README_Windows.html: ../README USAGE
	$(ASCIIDOC_START) -a recoilwin ../README $(ASCIIDOC_END)

README_WindowsSetup.html: ../README USAGE
	$(ASCIIDOC_START) ../README $(ASCIIDOC_END)

setup: ../../recoil-$(VERSION)-win32.msi

../../recoil-$(VERSION)-win32.msi: setup/recoil.wixobj faileye.ico setup/license.rtf setup/Website.url README_WindowsSetup.html $(WIN32_BIN) thumbrecoil.dll
	$(LIGHT) -ext WixUIExtension -sice:ICE69 $<

setup/recoil.wixobj: setup/recoil.wxs
	$(CANDLE) -dVERSION=$(VERSION) $<

x64-setup: ../../recoil-$(VERSION)-win64.msi

../../recoil-$(VERSION)-win64.msi: x64/recoil.wixobj faileye.ico setup/license.rtf setup/Website.url README_WindowsSetup.html $(WIN32_BIN) thumbrecoil.dll x64/thumbrecoil.dll x64/IM_MOD_RL_recoil_.dll x64/RECOIL.plg64
	$(LIGHT) -ext WixUIExtension -sice:ICE69 -sice:ICE80 -b setup $<

x64/recoil.wixobj: setup/recoil.wxs
	test -d x64 || mkdir x64
	$(CANDLE) -arch x64 -dVERSION=$(VERSION) $<

../MANIFEST:
	test -e ../.git && ( \
		($(GIT) --git-dir=../.git ls-files | grep -vF .gitignore \
			&& echo MANIFEST && echo README.html && echo recoil.c && echo recoil.h) | sort >$@; \
	)

srcdist: ../MANIFEST ../README.html ../recoil.c ../recoil.h
	$(RM) ../../recoil-$(VERSION).tar.gz
	cd .. && $(TAR) -c --numeric-owner --owner=0 --group=0 --mode=644 -T MANIFEST --transform=s,,recoil-$(VERSION)/, | $(SEVENZIP) -tgzip -si ../recoil-$(VERSION).tar.gz

dist: srcdist ../../recoil-$(VERSION)-win32.zip setup x64-setup

../../recoil-$(VERSION)-win32.zip: COPYING.txt README_Windows.html $(WIN32_BIN)
	$(RM) ../../recoil-$(VERSION)-win32.zip
	$(SEVENZIP) -tzip ../../recoil-$(VERSION)-win32.zip COPYING.txt README_Windows.html $(WIN32_BIN)

www: ../../www/index.html

../../www/index.html: ../README
	$(ASCIIDOC_START) -a recoilwww ../README $(ASCIIDOC_END)

clean:
	$(RM) $(WIN32_BIN) thumbrecoil.dll recoilwin/recoilwin-res.o COPYING.txt \
		../README.html README_Windows.html README_WindowsSetup.html setup/recoil.wixobj x64/recoil.wixobj x64/IM_MOD_RL_recoil_.dll x64/thumbrecoil.dll x64/RECOIL.plg64 RecoilPaintDotNet.dll

.PHONY: all clean setup x64-setup ../MANIFEST srcdist dist www

.DELETE_ON_ERROR:
