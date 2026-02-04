%define DIRNAME giza
%define LIBNAME smartmet-%{DIRNAME}
%define SPECNAME smartmet-library-%{DIRNAME}
Summary: Giza extensions to Cairo Graphics
Name: %{SPECNAME}
Version: 26.1.5
Release: 1%{?dist}.fmi
License: MIT
Group: Development/Libraries
URL: https://github.com/fmidev/smartmet-library-giza
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)

%if 0%{?rhel} && 0%{rhel} < 9
%define smartmet_boost boost169
%else
%define smartmet_boost boost
%endif

BuildRequires: rpm-build
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: %{smartmet_boost}-devel
BuildRequires: smartmet-library-macgyver-devel >= 26.2.4
BuildRequires: librsvg2-devel >= 2.40.6
Requires: librsvg2 >= 2.40.6
BuildRequires: cairo-devel
BuildRequires: libwebp13-devel
Requires: cairo
Requires: libwebp13
Requires: smartmet-library-macgyver >= 26.2.4
Provides: %{SPECNAME}
Obsoletes: libsmartmet-giza < 16.12.21
Obsoletes: libsmartmet-giza-debuginfo < 16.12.21
#TestRequires: fmt-devel
#TestRequires: cairo-devel
#TestRequires: libwebp13-devel
%if 0%{?rhel} && 0%{rhel} < 8
#TestRequires: libwebp13-tools
%endif
#TestRequires: %{smartmet_boost}-devel
#TestRequires: gcc-c++
#TestRequires: smartmet-library-regression
#TestRequires: smartmet-library-macgyver-devel >= 26.2.4
#TestRequires: smartmet-library-macgyver >= 26.2.4
#TestRequires: ImageMagick
#TestRequires: librsvg2-devel >= 2.40.6
#TestRequires: librsvg2 >= 2.40.6
#TestRequires: ImageMagick-c++-devel

%description
FMI Extensions to Cairo Graphics

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{SPECNAME}
 
%build
make %{_smp_mflags}

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(0775,root,root,0775)
%{_libdir}/libsmartmet-%{DIRNAME}.so

%package -n %{SPECNAME}-devel
Summary: Giza development files
Provides: %{SPECNAME}-devel
Requires: %{SPECNAME}
%if 0%{rhel} >= 8
BuildRequires: librsvg2-devel >= 2.40.6
%else
BuildRequires: librsvg2-devel = 2.40.6
%endif
Obsoletes: libsmartmet-giza-devel < 16.12.21

%description -n %{SPECNAME}-devel
Giza library development files

%files -n %{SPECNAME}-devel
%defattr(0664,root,root,0775)
%{_includedir}/smartmet/%{DIRNAME}

%changelog
* Mon Jan  5 2026 Mika Heiskanen <mika.heiskanen@fmi.fi> - 26.1.5-1.fmi
- Use webp13 instead of webp

* Tue Dec 30 2025 Mika Heiskanen <mika.heiskanen@fmi.fi> - 25.12.30-1.fmi
- Fixed webp linkage

* Mon Dec 29 2025 Mika Heiskanen <mika.heiskanen@fmi.fi> - 25.12.29-1.fmi
- webp animation updates

* Thu Nov  6 2025 Mika Heiskanen <mika.heiskanen@fmi.fi> - 25.11.6-1.fmi
- Fixed handling of webp transparency

* Tue Feb 18 2025 Andris Pavēnis <andris.pavenis@fmi.fi> 25.2.18-1.fmi
- Update to gdal-3.10, geos-3.13 and proj-9.5

* Wed Aug  7 2024 Andris Pavēnis <andris.pavenis@fmi.fi> 24.8.7-1.fmi
- Update to gdal-3.8, geos-3.12, proj-94 and fmt-11

* Fri Jul 12 2024 Andris Pavēnis <andris.pavenis@fmi.fi> 24.7.12-1.fmi
- Replace many boost library types with C++ standard library ones

* Fri Jul 28 2023 Andris Pavēnis <andris.pavenis@fmi.fi> 23.7.28-1.fmi
- Repackage due to bulk ABI changes in macgyver/newbase/spine

* Tue Mar 21 2023 Mika Heiskanen <mika.heiskanen@fmi.fi> - 23.3.21-1.fmi
- Fixed release year

* Tue Mar  7 2023 Mika Heiskanen <mika.heiskanen@fmi.fi> - 23.3.7-1.fmi
- Silenced CodeChecker warnings

* Wed Aug 31 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.8.31-1.fmi
- Added webp support

* Fri Jun 17 2022 Andris Pavēnis <andris.pavenis@fmi.fi> 22.6.17-1.fmi
- Add support for RHEL9. Update libpqxx to 7.7.0 (rhel8+) and fmt to 8.1.1

* Fri Jun 18 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.6.18-1.fmi
- Silenced some CodeChecker warnings

* Wed Jun 16 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.6.16-1.fmi
- Use Fmi::Exception

* Thu Jan 14 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.1.14-1.fmi
- Repackaged smartmet to resolve debuginfo issues

* Wed Oct  7 2020 Andris Pavenis <andris.pavenis@fmi.fi> - 20.10.7-1.fmi
- Use makefile.inc from smartmet-library-macgyver
- Fail build in case of unresolved references from shared library

* Sat Apr 18 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.4.18-1.fmi
- Upgrade to Boost 1.69

* Mon Oct  1 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.10.1-1.fmi
- Added option -g to get proper debuginfo packages

* Mon Aug  6 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.8.6-1.fmi
- Silenced CodeChecker warnings

* Wed Aug  1 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.8.1-1.fmi
- Fixed RGB values to be unpremultiplied when saving PNG palettealpha images

* Wed Jun 27 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.6.27-1.fmi
- Maintenance release: use nullptr instead of NULL

* Wed May  2 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.5.2-1.fmi
- Added possibility to control SVG to PNG color reduction

* Mon Mar  5 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.3.5-1.fmi
- Less aggressive color reduction with transparent images even when palette size is smaller than 256

* Mon Feb 12 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.2.12-1.fmi
- Less aggressive color reduction of images with lots of transparency

* Thu Oct 12 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.10.12-1.fmi
- Explicit dependency on the FMI fork of librsvg2

* Sun Aug 27 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.8.28-1.fmi
- Upgrade to boost 1.65

* Wed Jul 12 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.7.12-1.fmi
- Always produce EPS instead of plain PS.

* Wed Dec 21 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.12.21-1.fmi
- Switched to open source naming conventions

* Thu Aug 27 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.27-1.fmi
- PNG images are now saved in palette mode when possible

* Tue Aug 25 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.25-1.fmi
- Using precalculated gamma correction tables for speed

* Fri Feb 27 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.2.27-1.fmi
- Fixed the dynamic library to be an executable

* Wed Feb  4 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.2.4-1.fmi
- Added color reduction classes
- Added SVG rendering

