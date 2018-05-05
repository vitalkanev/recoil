VERSION = 4.2.0

BINARY = bin/RECOIL.qlgenerator/Contents/MacOS/qlrecoil 
PLIST = bin/RECOIL.qlgenerator/Contents/Info.plist

recoil-$(VERSION)-macos.dmg: bin/QuickLook $(BINARY) $(PLIST) bin/XnViewMP bin/Xrecoil.usr
	hdiutil create -volname recoil-$(VERSION)-macos -srcfolder bin -format UDBZ -fs HFS+ -imagekey bzip2-level=3 -ov $@

bin/QuickLook:
	mkdir -p $(@D) && ln -s /Library/QuickLook $@

bin/XnViewMP:
	mkdir -p $(@D) && ln -s /Applications/XnViewMP.app/Contents/MacOS/Plugins $@

$(BINARY): qlrecoil.c ../recoil-stdio.c ../recoil-stdio.h ../recoil.c ../recoil.h
# As of Xcode 4.6.3, -O2 allocates a 2.5MB of stack for RECOIL_Decode, which causes non-deterministic bus errors :(
	mkdir -p $(@D) && gcc -O1 -Wall -o $@ -I .. -dynamiclib -arch x86_64 -arch i386 -mmacosx-version-min=10.5 qlrecoil.c ../recoil-stdio.c ../recoil.c -framework QuickLook -framework ApplicationServices -framework CoreFoundation && strip -x $@

$(PLIST): Info.plist.xsl ../formats.xml
	mkdir -p $(@D) && xsltproc -o $@ Info.plist.xsl ../formats.xml

bin/Xrecoil.usr: ../Xrecoil.c ../formats.h ../recoil-stdio.c ../recoil-stdio.h ../recoil.c ../recoil.h
	mkdir -p $(@D) && gcc -O1 -Wall -o $@ -I .. -dynamiclib -mmacosx-version-min=10.5 ../Xrecoil.c ../recoil-stdio.c ../recoil.c

../formats.h: ../formats.h.xsl ../formats.xml
	xsltproc -o $@ ../formats.h.xsl ../formats.xml

%.c %.h: %.ci ../atari8.fnt ../altirrapal.pal ../c16.pal ../c64.fnt ../zx81.fnt
	cito -o $*.c -I .. $<

clean:
	rm -rf recoil-$(VERSION)-macos.dmg bin

.PHONY: clean

.DELETE_ON_ERROR:
