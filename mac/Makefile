all:qlrecoil-Info.plist qlrecoil.qlgenerator
	
qlrecoil-Info.plist: qlrecoil/qlrecoil-Info.xsl ../formats.xml
	xsltproc -o qlrecoil/$@ qlrecoil/qlrecoil-Info.xsl ../formats.xml

qlrecoil.qlgenerator:
	xcodebuild -project qlrecoil.xcodeproj -configuration “Debug/Release” CONFIGURATION_BUILD_DIR=dist

clean:
	rm -rf qlrecoil/qlrecoil-Info.plist build/qlrecoil.build dist/qlrecoil.qlgenerator.dSYM dist/qlrecoil.qlgenerator