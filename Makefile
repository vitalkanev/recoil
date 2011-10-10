PREFIX = /usr/local
MAGICK_VERSION = `MagickCore-config --version`
MAGICK_CFLAGS = `MagickCore-config --cflags --cppflags`
MAGICK_LDFLAGS = `MagickCore-config --ldflags`
MAGICK_LIBS = `MagickCore-config --libs`
MAGICK_CODER_PATH = `MagickCore-config --coder-path`
MAGICK_CONFIG_PATH = `MagickCore-config --exec-prefix`/etc/ImageMagick
TEST_MAGICK = -x "`which MagickCore-config`"

CC = gcc 
CFLAGS = -s -O2 -Wall
INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644

FORMATS    = GR8 HIP MIC INT TIP INP HR GR9 PIC CPR CIN CCI APC PLM AP3 ILC RIP FNT SXS MCP GHG HR2 MCH IGE 256 AP2 JGP DGP ESC PZM IST RAW RGB MGP WND CHR SHP
FORMATS_LC = gr8 hip mic int tip inp hr gr9 pic cpr cin cci apc plm ap3 ilc rip fnt sxs mcp ghg hr2 mch ige 256 ap2 jgp dgp esc pzm ist raw rgb mgp wnd chr shp

all: fail2png fail.so

fail2png: fail2png.c pngsave.c fail.c pngsave.h fail.h palette.h
	$(CC) $(CFLAGS) fail2png.c pngsave.c fail.c -lpng -lz -lm -o $@

fail.so: fail.c failmagick.c fail.h palette.h
	@if [ $(TEST_MAGICK) ]; then \
		if [ -z "$(MAGICK_INCLUDE_PATH)" ]; then \
			echo "\nDetected ImageMagick version $(MAGICK_VERSION) on your system."; \
			echo "To build FAIL coder for ImageMagick,\nspecify path to ImageMagick sources, e.g.:"; \
			echo "$ make MAGICK_INCLUDE_PATH=/path/to/im/source\n"; \
		else \
			echo $(CC) $(CFLAGS) $(MAGICK_CFLAGS) -I"$(MAGICK_INCLUDE_PATH)" fail.c failmagick.c \
				-shared $(MAGICK_LDFLAGS) -ldl $(MAGICK_LIBS) -o $@; \
			$(CC) $(CFLAGS) $(MAGICK_CFLAGS) -I"$(MAGICK_INCLUDE_PATH)" fail.c failmagick.c \
				-shared $(MAGICK_LDFLAGS) -ldl $(MAGICK_LIBS) -o $@; \
		fi; \
	fi;

palette.h: raw2c.pl jakub.act
	perl raw2c.pl jakub.act >$@

README.html: README INSTALL
	asciidoc -o $@ -a failsrc README
	perl -pi -e 's/527bbd;/800080;/' $@

clean:
	rm -f fail2png palette.h fail.so coder.xml.new fail-mime.xml

install: install-thumbnailer install-magick

uninstall: uninstall-thumbnailer uninstall-magick

install-fail2png: fail2png
	mkdir -p $(PREFIX)/bin
	$(INSTALL_PROGRAM) fail2png $(PREFIX)/bin/fail2png

uninstall-fail2png:
	rm -f $(PREFIX)/bin/fail2png

install-magick: fail.so
	if [ $(TEST_MAGICK) -a -n "$(MAGICK_CODER_PATH)" -a -n "$(MAGICK_CONFIG_PATH)" ]; then \
		perl addcoders.pl $(FORMATS) <$(MAGICK_CONFIG_PATH)/coder.xml >coder.xml.new; \
		mkdir -p "$(MAGICK_CODER_PATH)"; \
		$(INSTALL) fail.so "$(MAGICK_CODER_PATH)/fail.so"; \
		echo "dlname='fail.so'" >"$(MAGICK_CODER_PATH)/fail.la"; \
		mv coder.xml.new "$(MAGICK_CONFIG_PATH)"/coder.xml; \
	fi

uninstall-magick:
	if [ $(TEST_MAGICK) ]; then \
		perl delcoders.pl <$(MAGICK_CONFIG_PATH)/coder.xml >coder.xml.new; \
		rm -f "$(MAGICK_CODER_PATH)/fail.la" "$(MAGICK_CODER_PATH)/fail.so"; \
		mv coder.xml.new "$(MAGICK_CONFIG_PATH)"/coder.xml; \
	fi

fail-mime.xml: fail-types.xsl fail-types.xml
	xsltproc -o $@ fail-types.xsl fail-types.xml

install-thumbnailer: fail-mime.xml install-fail2png
	mkdir -p $(PREFIX)/share/mime/packages
	$(INSTALL_DATA) fail-mime.xml $(PREFIX)/share/mime/packages/fail-mime.xml
	update-mime-database $(PREFIX)/share/mime
	for ext in $(FORMATS_LC); do \
		gconftool-2 --config-source xml:readwrite:/etc/gconf/gconf.xml.defaults -s /desktop/gnome/thumbnailers/image@x-$$ext/command -t string "fail2png -o %o %i"; \
		gconftool-2 --config-source xml:readwrite:/etc/gconf/gconf.xml.defaults -s /desktop/gnome/thumbnailers/image@x-$$ext/enable -t boolean true; \
	done

uninstall-thumbnailer:
	rm -f $(PREFIX)/share/mime/packages/fail-mime.xml
	update-mime-database $(PREFIX)/share/mime
	for ext in $(FORMATS_LC); do \
		gconftool-2 -u "/desktop/gnome/thumbnailers/image@x-$$ext/command" "/desktop/gnome/thumbnailers/image@x-$$ext/enable" ; \
	done

.PHONY: all clean install uninstall install-fail2png uninstall-fail2png install-magick uninstall-magick install-thumbnailer uninstall-thumbnailer

.DELETE_ON_ERROR:
