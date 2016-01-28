Name:           fbdev_simple
Version:        1.0.1
Release:        0
Summary:        fbdev_simple
Group:          Frame buffer
License:        MIT
Source: %{name}-%{version}.tar.gz
BuildRequires: pkgconfig(libudev)

%description
Framebuffer init example

%prep
%setup -q

%build
%reconfigure --prefix=%{_prefix} CFLAGS="${CFLAGS} -Wall -Werror" LDFLAGS="${LDFLAGS} -Wl,--hash-style=both -Wl,--as-needed"

#make %{?_smp_mflags}
make %{?jobs:-j%jobs}

%install
%make_install

%files
%defattr(-,root,root,-)
/usr/bin/fbdev_simple
