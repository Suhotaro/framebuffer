Name:           fbdev_simple
Version:        1.0.1
Release:        0
Summary:        fbdev_simple
Group:          Development/Libraries
License:        MIT
Source0:        %{name}-%{version}.tar.gz
BuildRequires: pkgconfig(libudev)

%description
Back-End library of Tizen Display Manager DRM : libtdm-mgr DRM library

%prep
%setup -q
cp %{SOURCE1001} .

%build
%reconfigure --prefix=%{_prefix} --libdir=%{_libdir}  --disable-static \
             CFLAGS="${CFLAGS} -Wall -Werror" \
             LDFLAGS="${LDFLAGS} -Wl,--hash-style=both -Wl,--as-needed"
make %{?_smp_mflags}

%install
%make_install

%post
if [ -f %{_libdir}/tdm/libtdm-default.so ]; then
    rm -rf %{_libdir}/tdm/libtdm-default.so
fi
ln -s libtdm-drm.so %{_libdir}/tdm/libtdm-default.so

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%license COPYING
%defattr(-,root,root,-)
%{_libdir}/tdm/libtdm-drm.so
