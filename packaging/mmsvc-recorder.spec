Name:       mmsvc-recorder
Summary:    A Recorder library in Tizen Native API
Version:    0.2.2
Release:    0
Group:      Multimedia/API
License:    Apache-2.0
Source0:    %{service}-%{name}-%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(mm-camcorder)
BuildRequires:  pkgconfig(audio-session-mgr)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(mmsvc-camera)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(mused)
BuildRequires:  pkgconfig(capi-media-tool)
BuildRequires:  pkgconfig(json)
BuildRequires:  pkgconfig(capi-media-audio-io)


%description
A Recorder library in Tizen Native API


%package devel
Summary:  A Recorder library in Tizen C API (Development)
Requires: %{name} = %{version}-%{release}


%description devel
A Recorder library in Tizen Native API Development Package.


%prep
%setup -q


%build
MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
%cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DFULLVER=%{version} -DMAJORVER=${MAJORVER}
make %{?jobs:-j%jobs}


%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE.APLv2 %{buildroot}/usr/share/license/%{name}


%make_install


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%manifest mmsvc-recorder.manifest
%{_libdir}/libmmsvc-recorder.so.*
%{_libdir}/libmuse-recorder.so*
%{_datadir}/license/%{name}
%files devel
%{_includedir}/media/mmsvc_recorder.h
%{_includedir}/media/muse_recorder.h
%{_includedir}/media/muse_recorder_msg.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/libmmsvc-recorder.so
%{_libdir}/libmuse-recorder.so
