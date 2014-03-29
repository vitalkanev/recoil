<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" encoding="UTF-8">
<xsl:output method="xml" indent="yes" />
	<xsl:template match="/formats">
		  <xsl:text disable-output-escaping='yes'>
&lt;!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
		  </xsl:text>
<plist version="1.0">
		  <dict>
		  	<key>CFBundleDevelopmentRegion</key>
		  	<string>English</string>
		  	<key>CFBundleDocumentTypes</key>
		  	<array>
		  		<dict>
		  			<key>CFBundleTypeRole</key>
		  			<string>QLGenerator</string>
		  			<key>LSItemContentTypes</key>
					<array>
						<xsl:for-each select="platform">
							<xsl:for-each select="format/ext[not(. = ../following-sibling::format/ext)]">
								<string>
									<xsl:value-of select="translate(../../@name, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ- ','abcdefghijklmnopqrstuvwxyz')" />
									<xsl:text>.</xsl:text>
									<xsl:value-of select="translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')" />
								</string>
							</xsl:for-each>
						</xsl:for-each>						
					</array>
				</dict>
			</array>					
		<key>CFBundleExecutable</key>
		<string>${EXECUTABLE_NAME}</string>
		<key>CFBundleIconFile</key>
		<string></string>
		<key>CFBundleIdentifier</key>
		<string>PP.${PRODUCT_NAME:rfc1034identifier}</string>
		<key>CFBundleInfoDictionaryVersion</key>
		<string>6.0</string>
		<key>CFBundleName</key>
		<string>${PRODUCT_NAME}</string>
		<key>CFBundleShortVersionString</key>
		<string>1</string>
		<key>CFBundleVersion</key>
		<string>1.0</string>
		<key>CFPlugInDynamicRegisterFunction</key>
		<string></string>
		<key>CFPlugInDynamicRegistration</key>
		<string>NO</string>
		<key>CFPlugInFactories</key>
		<dict>
			<key>5D0A7038-458C-4BC7-9F9F-C02385B3D67C</key>
			<string>QuickLookGeneratorPluginFactory</string>
		</dict>
		<key>CFPlugInTypes</key>
		<dict>
			<key>5E2D9680-5022-40FA-B806-43349622E5B9</key>
			<array>
				<string>5D0A7038-458C-4BC7-9F9F-C02385B3D67C</string>
			</array>
		</dict>
		<key>CFPlugInUnloadFunction</key>
		<string></string>
		<key>NSHumanReadableCopyright</key>
		<string>Copyright © 2014 Piotr Fusik, Adrian Matog, Petri Pyy. All rights reserved.</string>
		<key>QLNeedsToBeRunInMainThread</key>
		<false/>
		<key>QLPreviewHeight</key>
		<real>600</real>
		<key>QLPreviewWidth</key>
		<real>800</real>
		<key>QLSupportsConcurrentRequests</key>
		<false/>
		<key>QLThumbnailMinimumSize</key>
		<real>17</real>
		<key>UTImportedTypeDeclarations</key>
		<array>
			<xsl:for-each select="platform">
				<xsl:for-each select="format">
					<xsl:for-each select="ext[not(. = ../following-sibling::format/ext)]">
					<dict>
						<key>UTTypeConformsTo</key>
						<array>
							<string>public.image</string>
						</array>
						<key>UTTypeDescription</key>
						<string><xsl:value-of select="../../@name" /><xsl:text> </xsl:text><xsl:value-of select="../@name" /></string>
						<key>UTTypeIdentifier</key>
						<string><xsl:value-of select="translate(../../@name, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ- ','abcdefghijklmnopqrstuvwxyz')" /><xsl:text>.</xsl:text><xsl:value-of select="translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')" /></string>
						<key>UTTypeTagSpecification</key>
						<dict>
							<key>public.filename-extension</key>
							<array>
									<string>
										<xsl:value-of select="translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')" />
									</string>
							</array>
							<key>public.mime-type</key>
							<array>
									<string>
										<xsl:text>application/x-</xsl:text>
										<xsl:value-of select="translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')" />
									</string>
									<string>
										<xsl:text>image/</xsl:text>
										<xsl:value-of select="translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')" />
									</string>
									<string>
										<xsl:text>image/x-</xsl:text>
										<xsl:value-of select="translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')" />
									</string>
							</array>
						</dict>
					</dict>
					</xsl:for-each>
				</xsl:for-each>
			</xsl:for-each>	
		</array>
	</dict>
	</plist>			
  </xsl:template>

</xsl:stylesheet>