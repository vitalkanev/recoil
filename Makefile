PREFIX = /usr/local
MAGICK_VERSION     := $(shell MagickCore-config --version 2>/dev/null)
ifdef MAGICK_VERSION
MAGICK_CFLAGS      := $(shell MagickCore-config --cflags --cppflags) -fPIC
MAGICK_LDFLAGS     := $(shell MagickCore-config --ldflags)
MAGICK_LIBS        := $(shell MagickCore-config --libs)
MAGICK_PREFIX      := $(shell MagickCore-config --prefix)
MAGICK_CODER_PATH  := $(wildcard $(MAGICK_PREFIX)/lib/ImageMagick-$(word 1,$(MAGICK_VERSION))/modules-$(word 2,$(MAGICK_VERSION))/coders)
MAGICK_CONFIG_PATH := $(wildcard $(shell MagickCore-config --exec-prefix)/etc/ImageMagick)
CAN_INSTALL_MAGICK := $(and $(MAGICK_CODER_PATH),$(MAGICK_CONFIG_PATH))
endif

CITO = cito
CC = gcc 
CFLAGS = -O2 -Wall
INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644

all: recoil2png $(if $(CAN_INSTALL_MAGICK),imagemagick/recoil.so) recoil-mime.xml

recoil2png: recoil2png.c pngsave.c pngsave.h recoil-stdio.c recoil-stdio.h recoil.c recoil.h
	$(CC) $(CFLAGS) recoil2png.c pngsave.c recoil-stdio.c recoil.c -lpng -lz -o $@

ifdef CAN_INSTALL_MAGICK
imagemagick/recoil.so: imagemagick/recoilmagick.c recoil.c recoil.h formats.h
ifdef MAGICK_INCLUDE_PATH
	$(CC) $(CFLAGS) $(MAGICK_CFLAGS) -I$(MAGICK_INCLUDE_PATH) imagemagick/recoilmagick.c recoil.c -shared $(MAGICK_LDFLAGS) -ldl $(MAGICK_LIBS) -o $@
else
	@echo "\nDetected ImageMagick version $(MAGICK_VERSION) on your system."
	@echo "To build RECOIL coder for ImageMagick,"
	@echo "specify path to ImageMagick sources, e.g.:"
	@echo "$ make MAGICK_INCLUDE_PATH=/path/to/im/source"
endif
endif

formats.h: formats.h.xsl formats.xml
	xsltproc -o $@ formats.h.xsl formats.xml

# http://www.cmcrossroads.com/article/rules-multiple-outputs-gnu-make
%.c %.h: %.ci atari8.fnt altirrapal.pal c16.pal zx81.fnt
	$(CITO) -o $*.c $<

benchmark: benchmark.c recoil-stdio.c recoil-stdio.h recoil.c recoil.h
	$(CC) $(CFLAGS) benchmark.c recoil-stdio.c recoil.c -o $@

clean:
	rm -f recoil2png imagemagick/recoil.so imagemagick/coder.xml.new formats.h recoil-mime.xml benchmark

install: install-thumbnailer $(if $(CAN_INSTALL_MAGICK),install-magick)

uninstall: uninstall-thumbnailer $(if $(CAN_INSTALL_MAGICK),uninstall-magick)

install-recoil2png: recoil2png
	mkdir -p $(PREFIX)/bin
	$(INSTALL_PROGRAM) recoil2png $(PREFIX)/bin/recoil2png

uninstall-recoil2png:
	rm -f $(PREFIX)/bin/recoil2png

ifdef CAN_INSTALL_MAGICK

install-magick: imagemagick/updatecoder.pl formats.xml imagemagick/recoil.so
	perl imagemagick/updatecoder.pl formats.xml <$(MAGICK_CONFIG_PATH)/coder.xml >imagemagick/coder.xml.new
	mkdir -p $(MAGICK_CODER_PATH)
	$(INSTALL) imagemagick/recoil.so $(MAGICK_CODER_PATH)/recoil.so
	echo "dlname='recoil.so'" >$(MAGICK_CODER_PATH)/recoil.la
	mv imagemagick/coder.xml.new $(MAGICK_CONFIG_PATH)/coder.xml

uninstall-magick:
	perl imagemagick/updatecoder.pl <$(MAGICK_CONFIG_PATH)/coder.xml >imagemagick/coder.xml.new
	rm -f $(MAGICK_CODER_PATH)/recoil.la $(MAGICK_CODER_PATH)/recoil.so
	mv imagemagick/coder.xml.new $(MAGICK_CONFIG_PATH)/coder.xml

endif

recoil-mime.xml: recoil-mime.xsl formats.xml
	xsltproc -o $@ recoil-mime.xsl formats.xml

install-mime: recoil-mime.xml
	mkdir -p $(PREFIX)/share/mime/packages
	$(INSTALL_DATA) recoil-mime.xml $(PREFIX)/share/mime/packages/recoil-mime.xml
ifndef BUILDING_PACKAGE
	update-mime-database $(PREFIX)/share/mime
endif

uninstall-mime:
	rm -f $(PREFIX)/share/mime/packages/recoil-mime.xml
	update-mime-database $(PREFIX)/share/mime

install-thumbnailer: install-mime install-recoil2png
	mkdir -p $(PREFIX)/share/thumbnailers
	xsltproc -o $(PREFIX)/share/thumbnailers/recoil.thumbnailer recoil.thumbnailer.xsl formats.xml

uninstall-thumbnailer:
	rm -f $(PREFIX)/share/thumbnailers/recoil.thumbnailer

install-gnome2-thumbnailer: install-mime install-recoil2png gnome2-thumbnailer.xsl formats.xml
	for p in `xsltproc gnome2-thumbnailer.xsl formats.xml`; do \
		gconftool-2 --config-source xml:readwrite:/etc/gconf/gconf.xml.defaults -s $$p/command -t string 'recoil2png -o %o %i'; \
		gconftool-2 --config-source xml:readwrite:/etc/gconf/gconf.xml.defaults -s $$p/enable -t boolean true; \
	done

uninstall-gnome2-thumbnailer: uninstall-mime uninstall-recoil2png gnome2-thumbnailer.xsl formats.xml
	for p in `xsltproc gnome2-thumbnailer.xsl formats.xml`; do \
		gconftool-2 -u $$p/command $$p/enable; \
	done

deb:
	debuild -b -us -uc

missing-examples:
# first column: extensions that are missing in examples
# second column: unknown extension in examples, perhaps companion files
	bash -c 'comm -3 <( xsltproc formats.ext.xsl formats.xml ) <( ls ../examples | perl -ne "s/.+\.// and print uc" | /usr/bin/sort -u )'

cmp-examples: recoil2png
	rm -f ../png/*.png
	for p in ../examples/*; do \
		./recoil2png -o "../png/$${p#../examples/}.png" "$$p" && cmp "../ref/$${p#../examples/}.png" "../png/$${p#../examples/}.png"; \
	done

.PHONY: all clean install uninstall install-recoil2png uninstall-recoil2png $(if $(CAN_INSTALL_MAGICK),install-magick uninstall-magick) install-mime uninstall-mime install-thumbnailer uninstall-thumbnailer install-gnome2-thumbnailer uninstall-gnome2-thumbnailer deb missing-examples cmp-examples

.DELETE_ON_ERROR:
