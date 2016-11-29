<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="xml" indent="yes" />
	<xsl:template match="/formats">
		<xsl:comment>Generated automatically from formats.xml and Package.appxmanifest.xsl. Do not edit.</xsl:comment>
		<Package
			xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10"
			xmlns:mp="http://schemas.microsoft.com/appx/2014/phone/manifest"
			xmlns:uap="http://schemas.microsoft.com/appx/manifest/uap/windows10"
			IgnorableNamespaces="uap mp">

			<Identity Name="9998PiotrFusik.RECOIL" Publisher="CN=B70960AE-A992-475F-9A7E-4B341F4530CB" Version="3.5.0.0" />
			<mp:PhoneIdentity PhoneProductId="af64497d-34c9-459a-9f9f-7765f61deaee" PhonePublisherId="c4cb80df-4c1e-4cdc-9dc5-65ed07b84b94"/>

			<Properties>
				<DisplayName>RECOIL</DisplayName>
				<PublisherDisplayName>Piotr Fusik</PublisherDisplayName>
				<Logo>Assets\StoreLogo.png</Logo>
			</Properties>

			<Dependencies>
				<TargetDeviceFamily Name="Windows.Universal" MinVersion="10.0.0.0" MaxVersionTested="10.0.0.0" />
			</Dependencies>

			<Resources>
				<Resource Language="x-generate" />
			</Resources>

			<Applications>
				<Application Id="App" Executable="$targetnametoken$.exe" EntryPoint="RECOIL.App">
					<uap:VisualElements
						DisplayName="RECOIL"
						Square150x150Logo="Assets\Square150x150Logo.png"
						Square44x44Logo="Assets\Square44x44Logo.png"
						Description="Shows images in native formats of classic computers"
						BackgroundColor="#006374">
						<uap:DefaultTile Wide310x150Logo="Assets\Wide310x150Logo.png" />
						<uap:SplashScreen Image="Assets\SplashScreen.png" />
					</uap:VisualElements>
					<Extensions>
						<uap:Extension Category="windows.fileTypeAssociation">
							<uap:FileTypeAssociation Name="recoil">
								<uap:Logo>Assets\FileLogo.png</uap:Logo>
								<uap:SupportedFileTypes>
									<xsl:for-each select="platform/format/ext[not(. = following::ext) and . != 'MSP' and . != 'SCR']"><!-- the '.msp' and '.scr' file type associations are reserved for system use -->
										<xsl:sort />
										<uap:FileType>.<xsl:value-of select="translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')" /></uap:FileType>
									</xsl:for-each>
								</uap:SupportedFileTypes>
							</uap:FileTypeAssociation>
						</uap:Extension>
					</Extensions>
				</Application>
			</Applications>
		</Package>
	</xsl:template>
</xsl:stylesheet>
