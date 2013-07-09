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
CFLAGS = -s -O2 -Wall
INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644

FORMATS    = GR8 HIP MIC INT TIP INP HR GR9 PIC CPR CIN CCI APC PLM AP3 ILC RIP FNT SXS MCP GHG HR2 MCH IGE 256 AP2 JGP DGP ESC PZM IST RAW RGB MGP WND CHR SHP MBG FWA RM0 RM1 RM2 RM3 RM4 XLP MAX SHC ALL APP SGE DLM BKG G09 BG9 APV SPC APL GR7 G10 G11 ART DRG AGP PLA MIS 4PL 4MI 4PM PGF PGC PI1 PI2 PI3 PC1 PC2 PC3 NEO DOO SPU TNY TN1 TN2 TN3 CA1 CA2 CA3 ING PAC SPS GFB PI4 PI9 DGU DG1 TRP TRU GOD FTC DEL DPH IPC NLQ PMD ACS PCS HPM MCS A4R DC1 DGC CPT IFF BL1 BL2 BL3 BRU IP2 IMN ICN DIN VZI IRG IR2 ICE
FORMATS_LC = gr8 hip mic int tip inp hr gr9 pic cpr cin cci apc plm ap3 ilc rip fnt sxs mcp ghg hr2 mch ige 256 ap2 jgp dgp esc pzm ist raw rgb mgp wnd chr shp mbg fwa rm0 rm1 rm2 rm3 rm4 xlp max shc all app sge dlm bkg g09 bg9 apv spc apl gr7 g10 g11 art drg agp pla mis 4pl 4mi 4pm pgf pgc pi1 pi2 pi3 pc1 pc2 pc3 neo doo spu tny tn1 tn2 tn3 ca1 ca2 ca3 ing pac sps gfb pi4 pi9 dgu dg1 trp tru god ftc del dph ipc nlq pmd acs pcs hpm mcs a4r dc1 dgc cpt iff bl1 bl2 bl3 bru ip2 imn icn din vzi irg ir2 ice

all: fail2png $(if $(CAN_INSTALL_MAGICK),fail.so)

fail2png: fail2png.c pngsave.c pngsave.h fail.c fail.h
	$(CC) $(CFLAGS) fail2png.c pngsave.c fail.c -lpng -lz -lm -o $@

ifdef CAN_INSTALL_MAGICK
fail.so: failmagick.c fail.c fail.h
ifdef MAGICK_INCLUDE_PATH
	$(CC) $(CFLAGS) $(MAGICK_CFLAGS) -I"$(MAGICK_INCLUDE_PATH)" failmagick.c fail.c \
		-shared $(MAGICK_LDFLAGS) -ldl $(MAGICK_LIBS) -o $@;
else
	@echo "\nDetected ImageMagick version $(MAGICK_VERSION) on your system."; \
	echo "To build FAIL coder for ImageMagick,\nspecify path to ImageMagick sources, e.g.:"; \
	echo "$ make MAGICK_INCLUDE_PATH=/path/to/im/source\n";
endif
endif

# http://www.cmcrossroads.com/article/rules-multiple-outputs-gnu-make
%.c %.h: %.ci atari8.fnt jakub.act
	$(CITO) -o $*.c $<

README.html: README INSTALL
	asciidoc -o - -a failsrc README | sed -e "s/527bbd;/800080;/" >$@

benchmark: benchmark.c
	$(CC) $(CFLAGS) benchmark.c fail.c -o $@

clean:
	rm -f fail2png fail.so coder.xml.new fail-mime.xml benchmark

install: install-thumbnailer $(if $(CAN_INSTALL_MAGICK),install-magick)

uninstall: uninstall-thumbnailer $(if $(CAN_INSTALL_MAGICK),uninstall-magick)

install-fail2png: fail2png
	mkdir -p $(PREFIX)/bin
	$(INSTALL_PROGRAM) fail2png $(PREFIX)/bin/fail2png

uninstall-fail2png:
	rm -f $(PREFIX)/bin/fail2png

ifdef CAN_INSTALL_MAGICK
install-magick: fail.so
ifdef MAGICK_INCLUDE_PATH
	perl addcoders.pl $(FORMATS) <$(MAGICK_CONFIG_PATH)/coder.xml >coder.xml.new; \
	mkdir -p "$(MAGICK_CODER_PATH)"; \
	$(INSTALL) fail.so "$(MAGICK_CODER_PATH)/fail.so"; \
	echo "dlname='fail.so'" >"$(MAGICK_CODER_PATH)/fail.la"; \
	mv coder.xml.new "$(MAGICK_CONFIG_PATH)"/coder.xml;
endif

uninstall-magick:
	perl delcoders.pl <$(MAGICK_CONFIG_PATH)/coder.xml >coder.xml.new; \
	rm -f "$(MAGICK_CODER_PATH)/fail.la" "$(MAGICK_CODER_PATH)/fail.so"; \
	mv coder.xml.new "$(MAGICK_CONFIG_PATH)"/coder.xml;
endif

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

uninstall-thumbnailer: uninstall-fail2png
	rm -f $(PREFIX)/share/mime/packages/fail-mime.xml
	update-mime-database $(PREFIX)/share/mime
	for ext in $(FORMATS_LC); do \
		gconftool-2 -u "/desktop/gnome/thumbnailers/image@x-$$ext/command" "/desktop/gnome/thumbnailers/image@x-$$ext/enable" ; \
	done

missing-examples:
	@echo Missing examples for $(foreach EXT,$(FORMATS),$(if $(wildcard ../examples/*.$(EXT)),,$(EXT)))

.PHONY: all clean install uninstall install-fail2png uninstall-fail2png $(if $(CAN_INSTALL_MAGICK),install-magick uninstall-magick) install-thumbnailer uninstall-thumbnailer missing-examples

.DELETE_ON_ERROR:
