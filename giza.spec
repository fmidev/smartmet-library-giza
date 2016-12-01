%define LIBNAME giza
Summary: Giza extensions to Cairo Graphics
Name: libsmartmet-%{LIBNAME}
Version: 15.8.27
Release: 1%{?dist}.fmi
License: FMI
Group: Development/Libraries
URL: http://www.weatherproof.fi
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: boost-devel
BuildRequires: librsvg2-devel >= 2.40.6
BuildRequires: cairo-devel
Requires: cairo
Requires: librsvg2 >= 2.40.6
Provides: %{LIBNAME}

%description
FMI Extensions to Cairo Graphics

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{LIBNAME}
 
%build
make %{_smp_mflags}

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(0775,root,root,0775)
%{_libdir}/libsmartmet_%{LIBNAME}.so

%package -n libsmartmet-%{LIBNAME}-devel
Summary: Giza development files
Provides: %{LIBNAME}-devel
Requires: libsmartmet-giza

%description -n libsmartmet-%{LIBNAME}-devel
Giza library development files

%files -n libsmartmet-%{LIBNAME}-devel
%defattr(0664,root,root,0775)
%{_includedir}/smartmet/%{LIBNAME}

%changelog
* Thu Aug 27 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.27-1.fmi
- PNG images are now saved in palette mode when possible

* Tue Aug 25 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.25-1.fmi
- Using precalculated gamma correction tables for speed

* Fri Feb 27 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.2.27-1.fmi
- Fixed the dynamic library to be an executable

* Wed Feb  4 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.2.4-1.fmi
- Added color reduction classes
- Added SVG rendering

