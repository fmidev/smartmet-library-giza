%define DIRNAME giza
%define LIBNAME smartmet-%{DIRNAME}
%define SPECNAME smartmet-library-%{DIRNAME}
Summary: Giza extensions to Cairo Graphics
Name: %{SPECNAME}
Version: 20.10.7
Release: 1%{?dist}.fmi
License: MIT
Group: Development/Libraries
URL: https://github.com/fmidev/smartmet-library-giza
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: rpm-build
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: boost169-devel
BuildRequires: smartmet-library-macgyver-devel >= 20.10.7
%if 0%{rhel} >= 8
BuildRequires: librsvg2-devel >= 2.42
Requires: librsvg2 >= 2.42
%else
BuildRequires: librsvg2-devel = 2.40.6
Requires: librsvg2 = 2.40.6
%endif
BuildRequires: cairo-devel
Requires: cairo
Provides: %{SPECNAME}
Obsoletes: libsmartmet-giza < 16.12.21
Obsoletes: libsmartmet-giza-debuginfo < 16.12.21
#TestRequires: cairo-devel
#TestRequires: boost169-devel
#TestRequires: gcc-c++
#TestRequires: smartmet-library-regression
#TestRequires: smartmet-library-macgyver-devel >= 20.10.7
#TestRequires: ImageMagick
%if 0%{rhel} >= 8
#TestRequires: librsvg2-devel >= 2.42
#TestRequires: librsvg2 >= 2.42
%else
#TestRequires: librsvg2-devel = 2.40.6
#TestRequires: librsvg2 = 2.40.6
%endif

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
BuildRequires: librsvg2-devel >= 2.42
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

