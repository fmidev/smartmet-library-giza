%define DIRNAME giza
%define LIBNAME smartmet-%{DIRNAME}
%define SPECNAME smartmet-library-%{DIRNAME}
Summary: Giza extensions to Cairo Graphics
Name: %{SPECNAME}
Version: 16.12.21
Release: 1%{?dist}.fmi
License: MIT
Group: Development/Libraries
URL: https://github.com/fmidev/smartmet-library-giza
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: boost-devel
BuildRequires: librsvg2-devel >= 2.40.6
BuildRequires: cairo-devel
Requires: cairo
Requires: librsvg2 >= 2.40.6
Provides: %{SPECNAME}
Obsoletes: libsmartmet-giza < 16.12.21
Obsoletes: libsmartmet-giza-debuginfo < 16.12.21

%description
FMI Extensions to Cairo Graphics

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{DIRNAME}
 
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
Requires: smartmet-library-giza
Obsoletes: libsmartmet-giza-devel < 16.12.21

%description -n %{SPECNAME}-devel
Giza library development files

%files -n %{SPECNAME}-devel
%defattr(0664,root,root,0775)
%{_includedir}/smartmet/%{DIRNAME}

%changelog
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

