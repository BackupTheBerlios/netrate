# $id$

Distribution:	TFM Linux
Vendor:		TFM Group
Packager:	TFM Developers <dev@tfm.ro>

%define		date %(date +%Y%m%d%H%M)

Name:		netrate
Version:	0.94
Release:	1tfm
Provides:	%{name}
Source:		%{name}-%{version}.tar.gz
Buildroot:	%{_tmppath}/tfm-%{name}-build
Group:		System/Networking
Summary:	This progam monitors the network traffic on the network interfaces
Copyright:	BSD
URL:		http://kman.tfm.ro/projects/netrate/

%description
%{summary}

%prep
rm -rf $RPM_BUILD_DIR/%{name}-%{version}
rm -rf %{buildroot}
%setup -q

%build
export CFLAGS="-march=i386 -mcpu=i386"
cd src
make netrate-linux

%install
mkdir -p %{buildroot}/usr/bin
cd src
make DESTDIR=%{buildroot} install-strip

%clean
rm -rf %{buildroot}
rm -rf $RPM_BUILD_DIR/%{name}-%{version}

%files
%attr(-,root,root) /usr/bin/netrate


%changelog
* Sun May 16 2004 Cristian Andrei Calin <kman@tfm.ro>
- modified for cvs build

* Thu Dec  4 2003 Cristian Andrei Calin <kman@tfm.ro>
- Changed license to BSD
- Reintroduce FreeBSD support
- Changed URL

* Thu Dec  4 2003 Mihai Moldovanu <mihaim@tfm.ro>
- wrote initial spec for it
- ported to TFM/GNU Linux 
