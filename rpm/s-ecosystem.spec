Name:           s-ecosystem
Version:        1.0.0
Release:        1%{?dist}
Summary:        Shell utilities: sfetch, scat, sls
License:        GPLv3
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
%autosetup -n S-ecosystem-%{version}

%build
make %{?_smp_mflags}

%install
install -Dm755 sfetch %{buildroot}/usr/bin/sfetch
install -Dm755 scat %{buildroot}/usr/bin/scat
install -Dm755 sls %{buildroot}/usr/bin/sls

%files
/usr/bin/sfetch
/usr/bin/scat
/usr/bin/sls

%changelog
* Thu Aug 21 2026 hubbydenny <hubbydenny@users.noreply.github.com> - 1.0.0-1
- Initial release
