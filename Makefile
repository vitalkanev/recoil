PREFIX = /usr/local
MAGICK_CFLAGS = `MagickCore-config --cflags --cppflags`
MAGICK_LDFLAGS = `MagickCore-config --ldflags`
MAGICK_LIBS = `MagickCore-config --libs`
MAGICK_CODER_PATH = `MagickCore-config --coder-path`

CC = gcc 
CCFLAGS = -s -O2 -Wall -o $@
INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644

all: fail2png fail.so

fail2png: fail2png.c pngsave.c fail.c pngsave.h fail.h palette.h
	$(CC) $(CCFLAGS) fail2png.c pngsave.c fail.c -lpng -lz -lm

fail.so: fail.c failmagick.c fail.h palette.h
	$(CC) $(CCFLAGS) $(MAGICK_CFLAGS) fail.c failmagick.c \
		-shared $(MAGICK_LDFLAGS) -ldl $(MAGICK_LIBS)

palette.h: raw2c.pl jakub.act
	perl raw2c.pl jakub.act >$@

README.html: README
	asciidoc -o $@ -a failsrc README
	perl -pi -e 's/527bbd;/800080;/' $@

clean:
	rm -f fail2png palette.h fail.so

install: fail2png
	mkdir -p $(PREFIX)/bin
	$(INSTALL_PROGRAM) fail2png $(PREFIX)/bin/fail2png

uninstall:
	rm -f $(PREFIX)/bin/fail2png

install-magick: fail.so
	if [ -n "$(MAGICK_CODER_PATH)" ]; then \
		mkdir -p "$(MAGICK_CODER_PATH)"; \
		$(INSTALL) fail.so "$(MAGICK_CODER_PATH)/fail.so"; \
		echo "dlname='fail.so'" >"$(MAGICK_CODER_PATH)/fail.la"; \
	fi

uninstall-magick:
	rm -f "$(MAGICK_CODER_PATH)/fail.la" "$(MAGICK_CODER_PATH)/fail.so"

.DELETE_ON_ERROR: