#
# spec file for package s-ecosystem
#
# Copyright (c) 2026 hubbydenny
#
# All modifications and additions to the file contributed by third parties
# remain the property of their respective owners.

Name:           s-ecosystem
Version:        1.0.0
Release:        0
Summary:        Shell utilities: sfetch, scat, sls
License:        GPL-3.0-or-later
Group:          System/Shell
URL:            https://github.com/hubbydenny/S-ecosystem
Source0:        %{url}/archive/v%{version}.tar.gz
BuildRequires:  gcc-c++
BuildRequires:  make

%description
S-ecosystem is a set of shell utilities:
- sfetch: system fetch with 20 distro logos
- scat: file concatenation utility
- sls: simple ls utility

%prep
%setup -q

%build
make %{?_smp_mflags}

%install
install -Dm755 sfetch %{buildroot}/usr/bin/sfetch
install -Dm755 scat %{buildroot}/usr/bin/scat
install -Dm755 sls %{buildroot}/usr/bin/sls

%files
%license LICENSE
/usr/bin/sfetch
/usr/bin/scat
/usr/bin/sls

%changelog
* Fri Aug 21 2026 hubbydenny <hubbydenny@users.noreply.github.com> - 1.0.0-1
- Initial release
