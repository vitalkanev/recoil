all:recoil qlrecoil-Info.plist qlrecoil.qlgenerator

debug:recoil qlrecoil-Info.plist qlrecoil.qlgenerator-debug

recoil: 
	(cd .. && make && cd -)
	
qlrecoil-Info.plist: qlrecoil/qlrecoil-Info.xsl ../formats.xml
	xsltproc -o qlrecoil/$@ qlrecoil/qlrecoil-Info.xsl ../formats.xml

qlrecoil.qlgenerator:
	xcodebuild -project qlrecoil.xcodeproj -configuration “Release” CONFIGURATION_BUILD_DIR=dist

qlrecoil.qlgenerator-debug:
	xcodebuild -project qlrecoil.xcodeproj -configuration “Debug” CONFIGURATION_BUILD_DIR=dist

clean:
	rm -rf qlrecoil/qlrecoil-Info.plist build/qlrecoil.build dist/qlrecoil.qlgenerator.dSYM dist/qlrecoil.qlgenerator