PREFIX = /usr/local
MAGICK_CFLAGS = `MagickCore-config --cflags --cppflags`
MAGICK_LDFLAGS = `MagickCore-config --ldflags`
MAGICK_LIBS = `MagickCore-config --libs`
MAGICK_CODER_PATH = `MagickCore-config --coder-path`

INSTALL = /usr/bin/install -c

all: fail2png fail.la

fail2png: fail2png.c pngsave.c fail.c pngsave.h fail.h palette.h
	gcc -s -O2 -Wall -o $@ fail2png.c pngsave.c fail.c -lpng -lz -lm

fail.lo: fail.c fail.h palette.h
	libtool --tag=CC --mode=compile gcc -O2 -Wall $(MAGICK_CFLAGS) -c $< -o $@

failmagick.lo: failmagick.c fail.h
	libtool --tag=CC --mode=compile gcc -O2 -Wall $(MAGICK_CFLAGS) -c $< -o $@

fail.la: fail.lo failmagick.lo
	libtool --tag=CC --mode=link gcc -s -O2 -Wall $(MAGICK_CFLAGS) \
	-o $@ -no-undefined -module -avoid-version $(MAGICK_LDFLAGS) \
	-rpath "$(MAGICK_CODER_PATH)" fail.lo failmagick.lo -ldl $(MAGICK_LIBS)

palette.h: raw2c.pl jakub.act
	perl raw2c.pl jakub.act >$@

README.html: README
	asciidoc -o $@ -a doctime -a failsrc README
	perl -pi -e 's/527bbd;/800080;/' $@

clean:
	rm -f fail2png palette.h
	rm -f fail.lo fail.la failmagick.lo
	rm -r -f .libs

install: fail2png
	mkdir -p $(PREFIX)/bin
	cp -f fail2png $(PREFIX)/bin
	chmod 755 $(PREFIX)/bin/fail2png

uninstall:
	rm -f $(PREFIX)/bin/fail2png

install-magick: fail.la
	test -z "$(MAGICK_CODER_PATH)" || mkdir -p "$(MAGICK_CODER_PATH)"
	if test -f fail.la; then \
		libtool --mode=install $(INSTALL) fail.la "$(MAGICK_CODER_PATH)/fail.la"; \
	fi

uninstall-magick:
	libtool --mode=uninstall rm -f "$(MAGICK_CODER_PATH)/fail.la"

.DELETE_ON_ERROR:
