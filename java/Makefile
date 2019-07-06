VERSION = 4.3.2
SEVENZIP = 7z a -mx=9 -bd -bso0

RESOURCES := ../atari8.fnt ../altirrapal.pal ../c16.pal ../c64.fnt ../zx81.fnt

all: recoil-$(VERSION)-java.jar recoil-$(VERSION)-javadoc.zip

recoil-4.2.0-ALPHA3-java.jar: net/sf/recoil/RECOIL.class net/sf/recoil/atari8.fnt
	jar cf $@ net

net/sf/recoil/RECOIL.class: java/RECOIL.java
	mkdir -p $(@D) && javac -d . java/*.java

net/sf/recoil/atari8.fnt: $(RESOURCES)
	mkdir -p $(@D) && cp $^ $(@D)

java/RECOIL.java: ../recoil.ci $(RESOURCES)
	mkdir -p $(@D) && cito -o $@ -n net.sf.recoil -I .. $<

recoil-$(VERSION)-javadoc.zip: recoil-$(VERSION)-javadoc/index.html
	$(SEVENZIP) -tzip $@ $(<D)

recoil-$(VERSION)-javadoc/index.html: java/RECOIL.java
	javadoc -d $(@D) java/*.java

clean:
	$(RM) recoil-$(VERSION)-java.jar recoil-$(VERSION)-javadoc.zip
	rm -rf net java recoil-$(VERSION)-javadoc

.PHONY: clean
