VERSION = 3.4.0

CITO = cito
XSLTPROC = xsltproc -o $@
CFLAGS = -Wall -O2 -s
LDFLAGS_DLL = -shared -Wl,--kill-at -Wl,-subsystem,windows
CC = $(@D)-w64-mingw32-gcc -o $@ $(CFLAGS)
CXX = $(@D)-w64-mingw32-g++ -o $@ $(CFLAGS)
WINDRES = $(@D)-w64-mingw32-windres -o $@
CSC = C:/Windows/Microsoft.NET/Framework/v3.5/csc.exe -nologo -o+ -out:$@
RM = rm -f
UNIX2DOS = unix2dos
GIT = git
TAR = tar
SEVENZIP = 7z a -mx=9 -bd
CANDLE = candle -ext WixUtilExtension -nologo -o $@
LIGHT = light -ext WixUtilExtension -nologo -o $@ -spdb
MAGICK = magick
IMAGEMAGICK_i686_DIR = c:\Program Files (x86)\ImageMagick-6.8.9-Q16
IMAGEMAGICK_x86_64_DIR = c:\Program Files\ImageMagick-6.8.9-Q16
PAINT_NET_DIR = c:\Program Files\Paint.NET

WIN32_BIN = i686/recoil2png.exe i686/IM_MOD_RL_recoil_.dll i686/Xrecoil.usr i686/recoilwin.exe i686/RECOIL.plg paint.net/RecoilPaintDotNet.dll

all: $(WIN32_BIN) i686/thumbrecoil.dll

i686/recoil2png.exe: ../recoil2png.c ../pngsave.c ../pngsave.h ../recoil-stdio.c ../recoil-stdio.h ../recoil.c ../recoil.h
	mkdir -p $(@D) && $(CC) -I.. ../recoil2png.c ../pngsave.c ../recoil-stdio.c ../recoil.c -static -lpng16 -lz

i686/IM_MOD_RL_recoil_.dll x86_64/IM_MOD_RL_recoil_.dll: ../imagemagick/recoilmagick.c ../formats.h ../recoil.c ../recoil.h
	mkdir -p $(@D) && $(CC) $(LDFLAGS_DLL) -I.. ../imagemagick/recoilmagick.c ../recoil.c -I"$(IMAGEMAGICK_$(@D)_DIR)"/include -lCORE_RL_magick_ -L"$(IMAGEMAGICK_$(@D)_DIR)"
 
i686/Xrecoil.usr: xnview/Xrecoil.c ../formats.h recoil-win32.c recoil-win32.h ../recoil.c ../recoil.h
	mkdir -p $(@D) && $(CC) $(LDFLAGS_DLL) -I. -I.. xnview/Xrecoil.c recoil-win32.c ../recoil.c

i686/recoilwin.exe: recoilwin/recoilwin.c recoilwin/recoilwin.h recoil-win32.c recoil-win32.h i686/recoilwin-res.o ../pngsave.c ../pngsave.h ../formats.h ../recoil.c ../recoil.h
	mkdir -p $(@D) && $(CC) -I. -I.. -Wl,-subsystem,windows recoilwin/recoilwin.c recoil-win32.c i686/recoilwin-res.o ../pngsave.c ../recoil.c -lcomctl32 -lcomdlg32 -lgdi32 -static -lpng16 -lz

i686/recoilwin-res.o: recoilwin/recoilwin.rc recoilwin/recoilwin.h recoil.ico ../recoil.h
	mkdir -p $(@D) && $(WINDRES) -I.. recoilwin/recoilwin.rc

i686/thumbrecoil.dll x86_64/thumbrecoil.dll: thumbrecoil/thumbrecoil.cpp ../formats.h recoil-win32.c recoil-win32.h ../recoil.c ../recoil.h
	mkdir -p $(@D) && $(CXX) $(LDFLAGS_DLL) -I. -I.. -shared -Wl,-subsystem,windows -Wl,--kill-at thumbrecoil/thumbrecoil.cpp recoil-win32.c ../recoil.c -static -lgdi32 -lole32 -luuid

i686/RECOIL.plg x86_64/RECOIL.plg64: imagine/recoilimagine.c ../formats.h ../recoil.c ../recoil.h
	mkdir -p $(@D) && $(CC) $(LDFLAGS_DLL) -I.. imagine/recoilimagine.c ../recoil.c

paint.net/RecoilPaintDotNet.dll: paint.net/RecoilPaintDotNet.cs paint.net/RecoilFileTypeFactory.cs paint.net/RECOIL.cs
	$(CSC) -t:library paint.net\\RecoilPaintDotNet.cs paint.net\\RecoilFileTypeFactory.cs paint.net\\RECOIL.cs -r:"$(PAINT_NET_DIR)\PaintDotNet.Base.dll" -r:"$(PAINT_NET_DIR)\PaintDotNet.Data.dll"

paint.net/RecoilFileTypeFactory.cs: paint.net/RecoilFileTypeFactory.cs.xsl ../formats.xml
	$(XSLTPROC) paint.net/RecoilFileTypeFactory.cs.xsl ../formats.xml

paint.net/RECOIL.cs: ../recoil.ci ../atari8.fnt ../altirrapal.pal ../c16.pal ../zx81.fnt
	$(CITO) -o $@ -I .. -n Recoil $<

../formats.h: ../formats.h.xsl ../formats.xml
	$(XSLTPROC) ../formats.h.xsl ../formats.xml

# http://www.cmcrossroads.com/article/rules-multiple-outputs-gnu-make
%.c %.h: %.ci ../atari8.fnt ../altirrapal.pal ../c16.pal ../zx81.fnt
	$(CITO) -o $*.c -I .. $<

recoil.ico: ../recoil-512x512.png
	$(MAGICK) $< -resize 48x48 $@

COPYING.txt: ../COPYING
	$(UNIX2DOS) <$< >$@

setup: ../../recoil-$(VERSION)-win32.msi

x64-setup: ../../recoil-$(VERSION)-win64.msi

../../recoil-$(VERSION)-win32.msi: i686/recoil.wixobj recoil.ico setup/dialog.jpg setup/banner.jpg setup/license.rtf $(WIN32_BIN) i686/thumbrecoil.dll
	$(LIGHT) -ext WixUIExtension -sice:ICE69 $<

../../recoil-$(VERSION)-win64.msi: x86_64/recoil.wixobj recoil.ico setup/dialog.jpg setup/banner.jpg setup/license.rtf $(WIN32_BIN) i686/thumbrecoil.dll x86_64/thumbrecoil.dll x86_64/IM_MOD_RL_recoil_.dll x86_64/RECOIL.plg64
	$(LIGHT) -ext WixUIExtension -sice:ICE69 -sice:ICE80 $<

i686/recoil.wixobj x86_64/recoil.wixobj: setup/recoil.wxs setup/formats.wxi
	mkdir -p $(@D) && $(CANDLE) $(and $(@D:i686=), -arch x64) -dVERSION=$(VERSION) $<

setup/formats.wxi: setup/formats.wxi.xsl ../formats.xml
	$(XSLTPROC) setup/formats.wxi.xsl ../formats.xml

setup/dialog.jpg: ../recoil-512x512.png
	$(MAGICK) $< -resize 148 -extent 493x312-16-82 -strip $@

setup/banner.jpg: ../recoil-512x512.png
	$(MAGICK) $< -resize 48 -extent 493x58-425-5 -strip $@

../MANIFEST:
	test -e ../.git && ( \
		($(GIT) --git-dir=../.git ls-files | grep -vF .gitignore \
			&& echo MANIFEST && echo recoil.c && echo recoil.h) | sort >$@; \
	)

srcdist: ../MANIFEST ../recoil.c ../recoil.h
	$(RM) ../../recoil-$(VERSION).tar.gz
	cd .. && $(TAR) -c --numeric-owner --owner=0 --group=0 --mode=644 -T MANIFEST --transform=s,,recoil-$(VERSION)/, | $(SEVENZIP) -tgzip -si ../recoil-$(VERSION).tar.gz

dist: srcdist ../../recoil-$(VERSION)-win32.zip setup x64-setup

../../recoil-$(VERSION)-win32.zip: COPYING.txt $(WIN32_BIN)
	$(RM) ../../recoil-$(VERSION)-win32.zip
	$(SEVENZIP) -tzip ../../recoil-$(VERSION)-win32.zip COPYING.txt $(WIN32_BIN:%=./%)

clean:
	$(RM) $(WIN32_BIN) i686/thumbrecoil.dll i686/recoilwin-res.o recoil.ico COPYING.txt \
		setup/formats.wxi setup/dialog.jpg setup/banner.jpg i686/recoil.wixobj x86_64/recoil.wixobj \
		x86_64/IM_MOD_RL_recoil_.dll x86_64/thumbrecoil.dll x86_64/RECOIL.plg64 paint.net/RecoilPaintDotNet.dll \
		paint.net/RecoilFileTypeFactory.cs paint.net/RECOIL.cs ../formats.h

.PHONY: all clean setup x64-setup ../MANIFEST srcdist dist

.DELETE_ON_ERROR:
