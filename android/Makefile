ANDROID_SDK = C:/bin/android-sdk-windows
ANDROID_JAR = $(ANDROID_SDK)/platforms/android-17/android.jar

AAPT = $(ANDROID_SDK)/platform-tools/aapt
DX = java -jar "$(ANDROID_SDK)/platform-tools/lib/dx.jar" --no-strict
APKBUILDER = java -classpath "$(ANDROID_SDK)/tools/lib/sdklib.jar" com.android.sdklib.build.ApkBuilderMain $@
JARSIGNER = jarsigner -sigalg SHA1withDSA -digestalg SHA1
ZIPALIGN = $(ANDROID_SDK)/tools/zipalign
ADB = $(ANDROID_SDK)/platform-tools/adb
ANDROID = $(ANDROID_SDK)/tools/android.bat
EMULATOR = $(ANDROID_SDK)/tools/emulator

ANDROID_RELEASE = recoil-3.0.0-android.apk

all: $(ANDROID_RELEASE)

install-dev: $(ANDROID_RELEASE)
	$(ADB) -d install -r $(ANDROID_RELEASE)

log-dev:
	$(ADB) -d logcat

$(ANDROID_RELEASE): AndroidRECOIL-unaligned.apk
	$(ZIPALIGN) -f 4 $< $@

AndroidRECOIL-unaligned.apk: AndroidRECOIL-unsigned.apk
	$(JARSIGNER) -storepass walsie -signedjar $@ $< pfusik-android

AndroidRECOIL-unsigned.apk: AndroidRECOIL-resources.apk classes.dex
	$(APKBUILDER) -u -z AndroidRECOIL-resources.apk -f classes.dex

AndroidRECOIL-debug.apk: AndroidRECOIL-resources.apk classes.dex
	$(APKBUILDER) -z $< -f classes.dex

classes.dex: classes/net/sf/recoil/RECOIL.class
	$(DX) --dex --output=$@ classes

classes/net/sf/recoil/RECOIL.class: FileSelector.java FileUtil.java GalleryAdapter.java Viewer.java java/net/sf/recoil/RECOIL.java AndroidRECOIL-resources.apk
	mkdir -p classes && javac -d classes -bootclasspath $(ANDROID_JAR) FileSelector.java FileUtil.java GalleryAdapter.java Viewer.java java/net/sf/recoil/*.java

java/net/sf/recoil/RECOIL.java: ../recoil.ci ../atari8.fnt ../jakub.act
	mkdir -p $(@D) && cito -n net.sf.recoil -o $@ -I .. $<

AndroidRECOIL-resources.apk: AndroidManifest.xml res/drawable/icon.png res/layout/filename_list_item.xml res/values/strings.xml raw/net/sf/recoil/atari8.fnt raw/net/sf/recoil/jakub.act
	mkdir -p java && $(AAPT) p -f -m -M $< -I $(ANDROID_JAR) -S res -F $@ -J java raw

raw/net/sf/recoil/%: ../%
	mkdir -p $(@D) && cp $< $@

clean:
	$(RM) *.apk classes.dex
	rm -rf classes java raw

.PHONY: all install-dev log-dev clean

.DELETE_ON_ERROR:
