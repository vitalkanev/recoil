Name: recoil
Version: 6.0.0
Release: 1
Summary: Viewer of retro computer image files
License: GPLv2+
Group: Applications/Multimedia
Source: https://downloads.sourceforge.net/recoil/recoil-%{version}.tar.gz
URL: https://recoil.sourceforge.net/
BuildRequires: gcc, libpng-devel, libxslt
BuildRoot: %{_tmppath}/%{name}-root

%description
Decoder of pictures in native formats of
Amiga, Amstrad CPC, Apple II, Atari 8-bit/Portfolio/ST/TT/Falcon, BBC Micro,
Commodore VIC-20/16/64/128, Electronika BK, FM Towns, Macintosh 128K, MSX,
NEC PC-80/88/98, Oric, Psion Series 3, SAM Coupe, Sharp X68000, Tandy 1000,
Timex 2048, TRS-80, TRS-80 Color Computer, ZX81 and ZX Spectrum computers.

%package 2png
Summary: Converter of retro computer image files

%description 2png
Provides "recoil2png" command-line converter of pictures in native formats of
Amiga, Amstrad CPC, Apple II, Atari 8-bit/Portfolio/ST/TT/Falcon, BBC Micro,
Commodore VIC-20/16/64/128, Electronika BK, FM Towns, Macintosh 128K, MSX,
NEC PC-80/88/98, Oric, Psion Series 3, SAM Coupe, Sharp X68000, Tandy 1000,
Timex 2048, TRS-80, TRS-80 Color Computer, ZX81 and ZX Spectrum computers.

%package thumbnailer
Summary: GNOME thumbnailer for retro computer image files
Requires: recoil-2png
BuildArch: noarch

%description thumbnailer
GNOME thumbnailer for pictures in native formats of
Amiga, Amstrad CPC, Apple II, Atari 8-bit/Portfolio/ST/TT/Falcon, BBC Micro,
Commodore VIC-20/16/64/128, Electronika BK, FM Towns, Macintosh 128K, MSX,
NEC PC-80/88/98, Oric, Psion Series 3, SAM Coupe, Sharp X68000, Tandy 1000,
Timex 2048, TRS-80, TRS-80 Color Computer, ZX81 and ZX Spectrum computers.

%global debug_package %{nil}

%prep
%setup -q

%build
make recoil2png recoil-mime.xml

%install
rm -rf $RPM_BUILD_ROOT
make PREFIX=$RPM_BUILD_ROOT/%{_prefix} BUILDING_PACKAGE=1 install-thumbnailer

%clean
rm -rf $RPM_BUILD_ROOT

%files 2png
%defattr(-,root,root)
%{_bindir}/recoil2png
%{_mandir}/man1/recoil2png.1*

%files thumbnailer
%defattr(-,root,root)
%{_datadir}/mime/packages/recoil-mime.xml
%{_datadir}/thumbnailers/recoil.thumbnailer

%post thumbnailer
/usr/bin/update-mime-database %{_datadir}/mime &> /dev/null || :

%postun thumbnailer
/usr/bin/update-mime-database %{_datadir}/mime &> /dev/null || :

%changelog
* Thu Feb 4 2021 Piotr Fusik <fox@scene.pl>
- 6.0.0-1

* Fri Oct 30 2020 Piotr Fusik <fox@scene.pl>
- 5.1.1-1

* Tue Sep 8 2020 Piotr Fusik <fox@scene.pl>
- 5.1.0-1

* Wed Mar 18 2020 Piotr Fusik <fox@scene.pl>
- 5.0.1-1

* Tue Nov 12 2019 Piotr Fusik <fox@scene.pl>
- 5.0.0-1

* Sat Jul 6 2019 Piotr Fusik <fox@scene.pl>
- 4.3.2-1

* Wed Dec 5 2018 Piotr Fusik <fox@scene.pl>
- 4.3.1-1

* Sun Jun 10 2018 Piotr Fusik <fox@scene.pl>
- 4.3.0-1

* Tue Feb 20 2018 Piotr Fusik <fox@scene.pl>
- 4.2.0-1

* Wed Oct 18 2017 Piotr Fusik <fox@scene.pl>
- 4.1.0-1

* Wed May 17 2017 Piotr Fusik <fox@scene.pl>
- 4.0.0-1

* Thu Dec 1 2016 Piotr Fusik <fox@scene.pl>
- 3.5.0-1

* Tue Feb 2 2016 Piotr Fusik <fox@scene.pl>
- 3.4.0-1

* Fri Aug 7 2015 Piotr Fusik <fox@scene.pl>
- 3.3.0-1

* Wed Jan 28 2015 Piotr Fusik <fox@scene.pl>
- 3.2.0-1

* Wed Jun 25 2014 Piotr Fusik <fox@scene.pl>
- 3.1.0-1
- Initial packaging
