VERSION = 4.3.0

CITO = cito
XSLTPROC = xsltproc -o $@ $^
CFLAGS = -Wall -O2 -s
CSOURCES = $(patsubst %/,-I%,$(sort $(dir $(filter %.h,$^)))) $(^:%.h=)
LDFLAGS_DLL = -shared -Wl,--kill-at -Wl,-subsystem,windows
CC = $(@D)-w64-mingw32-gcc -o $@ $(CFLAGS) $(CSOURCES)
CXX = $(@D)-w64-mingw32-g++ -o $@ $(CFLAGS) $(CSOURCES)
WINDRES = $(@D)-w64-mingw32-windres -o $@
CSC = C:/Windows/Microsoft.NET/Framework/v4.0.30319/csc.exe -nologo -o+ -out:$@
RM = rm -f
UNIX2DOS = unix2dos <$< >$@
GIT = git
TAR = /usr/bin/tar
SEVENZIP = 7z a -mx=9 -bd -bso0
CANDLE = candle -ext WixUtilExtension -nologo -o $@
LIGHT = light -ext WixUtilExtension -nologo -o $@ -spdb
MAGICK = magick
IMAGEMAGICK_i686_DIR = C:\Program Files (x86)\ImageMagick-6.9.6-Q16
IMAGEMAGICK_x86_64_DIR = C:\Program Files\ImageMagick-6.9.6-Q16
PAINT_NET_DIR = C:\Program Files\paint.net
ifdef V
DO = mkdir -p $(@D) && 
else
DO = @echo $@ && mkdir -p $(@D) && 
endif

WIN32_BIN = i686/recoil2png.exe i686/IM_MOD_RL_recoil_.dll i686/Xrecoil.usr i686/recoilwin.exe i686/thumbrecoil.dll i686/RECOIL.plg paint.net/RecoilPaintDotNet.dll
WIN64_BIN = $(WIN32_BIN) x86_64/IM_MOD_RL_recoil_.dll x86_64/Xrecoil.usr x86_64/thumbrecoil.dll x86_64/RECOIL.plg64

all: $(WIN32_BIN)

i686/recoil2png.exe: ../recoil2png.c ../pngsave.c ../pngsave.h ../recoil-stdio.c ../recoil-stdio.h ../recoil.c ../recoil.h
	$(DO)$(CC) -static -lpng16 -lz

i686/IM_MOD_RL_recoil_.dll x86_64/IM_MOD_RL_recoil_.dll: ../imagemagick/recoilmagick.c ../formats.h ../recoil.c ../recoil.h
	$(DO)$(CC) $(LDFLAGS_DLL) -I"$(IMAGEMAGICK_$(@D)_DIR)"/include -lCORE_RL_magick_ -L"$(IMAGEMAGICK_$(@D)_DIR)"
 
i686/Xrecoil.usr x86_64/Xrecoil.usr: ../Xrecoil.c ../formats.h ../recoil-stdio.c ../recoil-stdio.h ../recoil.c ../recoil.h
	$(DO)$(CC) $(LDFLAGS_DLL)

%/recoilwin.exe: recoilwin/recoilwin.c recoilwin/recoilwin.h recoil-win32.c recoil-win32.h %/recoilwin-res.o ../pngsave.c ../pngsave.h ../formats.h ../recoil.c ../recoil.h
	$(DO)$(CC) -Wl,-subsystem,windows -lcomctl32 -lcomdlg32 -lgdi32 -static -lpng16 -lz

i686/recoilwin-res.o x86_64/recoilwin-res.o: recoilwin/recoilwin.rc recoilwin/recoilwin.h recoil.ico ../recoil.h
	$(DO)$(WINDRES) -I.. $<

i686/thumbrecoil.dll x86_64/thumbrecoil.dll: thumbrecoil/thumbrecoil.cpp ../formats.h recoil-win32.c recoil-win32.h ../recoil.c ../recoil.h
	$(DO)$(CXX) $(LDFLAGS_DLL) -static -lgdi32 -lole32 -luuid

i686/RECOIL.plg x86_64/RECOIL.plg64: imagine/recoilimagine.c ../formats.h ../recoil.c ../recoil.h
	$(DO)$(CC) $(LDFLAGS_DLL)

paint.net/RecoilPaintDotNet.dll: paint.net/RecoilPaintDotNet.cs paint.net/RecoilFileTypeFactory.cs paint.net/RECOIL.cs
	$(DO)$(CSC) -t:library $(subst /,\\,$^) -r:"$(PAINT_NET_DIR)\PaintDotNet.Base.dll" -r:"$(PAINT_NET_DIR)\PaintDotNet.Data.dll"

paint.net/RecoilFileTypeFactory.cs: paint.net/RecoilFileTypeFactory.cs.xsl ../formats.xml
	$(DO)$(XSLTPROC)

paint.net/RECOIL.cs: ../recoil.ci ../atari8.fnt ../altirrapal.pal ../c16.pal ../c64.fnt ../zx81.fnt
	$(DO)$(CITO) -o $@ -I .. -n Recoil $<

i686/by-platform.exe: by-platform.c recoil-win32.c recoil-win32.h ../recoil.c ../recoil.h
	$(DO)$(CC)

../formats.h: ../formats.h.xsl ../formats.xml
	$(DO)$(XSLTPROC)

# http://www.cmcrossroads.com/article/rules-multiple-outputs-gnu-make
%.c %.h: %.ci ../atari8.fnt ../altirrapal.pal ../c16.pal ../c64.fnt ../zx81.fnt
	$(DO)$(CITO) -o $*.c -I .. $<

recoil.ico: ../recoil-512x512.png
	$(DO)$(MAGICK) $< -resize 48x48 $@

COPYING.txt: ../COPYING
	$(DO)$(UNIX2DOS)

setup: ../../recoil-$(VERSION)-win32.msi

x64-setup: ../../recoil-$(VERSION)-win64.msi

../../recoil-$(VERSION)-win32.msi: i686/recoil.wixobj recoil.ico setup/dialog.jpg setup/banner.jpg setup/license.rtf $(WIN32_BIN)
	$(DO)$(LIGHT) -ext WixUIExtension -sice:ICE69 $<

../../recoil-$(VERSION)-win64.msi: x86_64/recoil.wixobj recoil.ico setup/dialog.jpg setup/banner.jpg setup/license.rtf $(WIN64_BIN)
	$(DO)$(LIGHT) -ext WixUIExtension -sice:ICE69 -sice:ICE80 $<

i686/recoil.wixobj x86_64/recoil.wixobj: setup/recoil.wxs setup/formats.wxi
	$(DO)$(CANDLE) $(and $(@D:i686=), -arch x64) $<

setup/formats.wxi: setup/formats.wxi.xsl ../formats.xml
	$(DO)$(XSLTPROC)

setup/dialog.jpg: ../recoil-512x512.png
	$(DO)$(MAGICK) $< -resize 148 -extent 493x312-16-82 -strip $@

setup/banner.jpg: ../recoil-512x512.png
	$(DO)$(MAGICK) $< -resize 48 -extent 493x58-425-5 -strip $@

../MANIFEST:
	$(DO)test -e ../.git && ( \
		($(GIT) --git-dir=../.git ls-files | grep -vF .gitignore \
			&& echo MANIFEST && echo recoil.c && echo recoil.h) | sort | dos2unix >$@; \
	)

srcdist: ../../recoil-$(VERSION).tar.gz

../../recoil-$(VERSION).tar.gz: ../MANIFEST ../recoil.c ../recoil.h
	$(DO)$(RM) $@ && $(TAR) -c --numeric-owner --owner=0 --group=0 --mode=644 -C .. -T ../MANIFEST --transform=s,,recoil-$(VERSION)/, | $(SEVENZIP) -tgzip -si $@

dist: srcdist ../../recoil-$(VERSION)-win32.zip setup x64-setup

../../recoil-$(VERSION)-win32.zip: COPYING.txt $(WIN32_BIN)
	$(DO)$(RM) $@ && $(SEVENZIP) -tzip $@ COPYING.txt $(WIN32_BIN:%=./%)

deb64:
	scp ../../recoil-$(VERSION).tar.gz vm:.
	ssh vm 'rm -rf recoil-$(VERSION) && tar xf recoil-$(VERSION).tar.gz && make -C recoil-$(VERSION) deb'
	scp vm:recoil-2png_$(VERSION)-1_amd64.deb vm:recoil-xnview_$(VERSION)-1_amd64.deb vm:recoil-thumbnailer_$(VERSION)-1_all.deb ../../

deb32:
	scp ../../recoil-$(VERSION).tar.gz vm:.
	ssh vm 'rm -rf recoil-$(VERSION) && tar xf recoil-$(VERSION).tar.gz && make -C recoil-$(VERSION) deb'
	scp vm:recoil-2png_$(VERSION)-1_i386.deb vm:recoil-xnview_$(VERSION)-1_i386.deb ../../

rpm64:
	scp ../../recoil-$(VERSION).tar.gz vm:.
	ssh vm 'rpmbuild -tb recoil-$(VERSION).tar.gz'
	scp vm:rpmbuild/RPMS/x86_64/recoil-2png-$(VERSION)-1.x86_64.rpm vm:rpmbuild/RPMS/noarch/recoil-thumbnailer-$(VERSION)-1.noarch.rpm ../../

rpm32:
	scp ../../recoil-$(VERSION).tar.gz vm:.
	ssh vm 'rpmbuild -tb recoil-$(VERSION).tar.gz'
	scp vm:rpmbuild/RPMS/i686/recoil-2png-$(VERSION)-1.i686.rpm ../../

clean:
	$(RM) $(WIN64_BIN) i686/recoilwin-res.o recoil.ico COPYING.txt \
		i686/recoil.wixobj x86_64/recoil.wixobj setup/formats.wxi setup/dialog.jpg setup/banner.jpg \
		paint.net/RecoilFileTypeFactory.cs paint.net/RECOIL.cs ../formats.h

.PHONY: all clean setup x64-setup ../MANIFEST srcdist dist deb64 deb32 rpm64 rpm32 ../../recoil-$(VERSION).tar.gz

.DELETE_ON_ERROR:
