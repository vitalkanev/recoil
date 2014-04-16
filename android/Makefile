ANDROID_SDK = C:/bin/android-sdk-windows
ANDROID_JAR = $(ANDROID_SDK)/platforms/android-19/android.jar
ANDROID_BUILD_TOOLS = $(ANDROID_SDK)/build-tools/19.0.1

AAPT = $(ANDROID_BUILD_TOOLS)/aapt
DX = java -jar "$(ANDROID_BUILD_TOOLS)/lib/dx.jar" --no-strict
APKBUILDER = java -classpath "$(ANDROID_SDK)/tools/lib/sdklib.jar" com.android.sdklib.build.ApkBuilderMain $@
JARSIGNER = jarsigner -sigalg SHA1withRSA -digestalg SHA1
ZIPALIGN = $(ANDROID_SDK)/tools/zipalign
ADB = $(ANDROID_SDK)/platform-tools/adb
ANDROID = $(ANDROID_SDK)/tools/android.bat
EMULATOR = $(ANDROID_SDK)/tools/emulator

ANDROID_RELEASE = recoil-3.0.0-android.apk

all: $(ANDROID_RELEASE)

install-dev: $(ANDROID_RELEASE)
	$(ADB) -d install -r $<

log-dev:
	$(ADB) -d logcat

$(ANDROID_RELEASE): AndroidRECOIL-unaligned.apk
	$(ZIPALIGN) -f 4 $< $@

AndroidRECOIL-unaligned.apk: AndroidRECOIL-unsigned.apk
	$(JARSIGNER) -storepass walsie -signedjar $@ $< pfusik

AndroidRECOIL-unsigned.apk: AndroidRECOIL-resources.apk classes.dex
	$(APKBUILDER) -u -z $< -f classes.dex

AndroidRECOIL-debug.apk: AndroidRECOIL-resources.apk classes.dex
	$(APKBUILDER) -z $< -f classes.dex

classes.dex: classes/net/sf/recoil/RECOIL.class
	$(DX) --dex --output=$@ classes

classes/net/sf/recoil/RECOIL.class: FileSelector.java FileUtil.java GalleryAdapter.java Viewer.java gen/net/sf/recoil/RECOIL.java AndroidRECOIL-resources.apk
	mkdir -p classes && javac -d classes -bootclasspath $(ANDROID_JAR) FileSelector.java FileUtil.java GalleryAdapter.java Viewer.java gen/net/sf/recoil/*.java

gen/net/sf/recoil/RECOIL.java: ../recoil.ci ../atari8.fnt ../altirrapal.pal
	mkdir -p $(@D) && cito -n net.sf.recoil -o $@ -I .. $<

AndroidRECOIL-resources.apk: AndroidManifest.xml res/drawable/ic_launcher.png res/layout/error.xml res/layout/filename_list_item.xml res/layout/gallery.xml res/menu/file_selector.xml res/menu/viewer.xml res/values/strings.xml assets/net/sf/recoil/atari8.fnt assets/net/sf/recoil/altirrapal.pal
	mkdir -p gen && $(AAPT) p -f -m -M $< -I $(ANDROID_JAR) -S res -F $@ -J gen assets

res/drawable/ic_launcher.png: ../recoil-512x512.png
	mkdir -p $(@D) && convert $< -resize 72x72 -quality 95 -strip $@

assets/net/sf/recoil/%: ../%
	mkdir -p $(@D) && cp $< $@

AndroidManifest.xml: AndroidManifest.xsl ../formats.xml
	xsltproc -o $@ AndroidManifest.xsl ../formats.xml

clean:
	$(RM) *.apk classes.dex res/drawable/ic_launcher.png AndroidManifest.xml
	rm -rf classes gen assets

.PHONY: all install-dev log-dev clean

.DELETE_ON_ERROR:
