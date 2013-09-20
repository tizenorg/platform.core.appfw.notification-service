Name:       notification-service
Summary:    Simple notification service
Version:    0.0.3
Release:    1
Group:      Application Framework/Notifications
License:    Flora
Source0:    %{name}-%{version}.tar.bz2
BuildRequires: pkgconfig(eina) 
BuildRequires: pkgconfig(ecore) 
BuildRequires: pkgconfig(com-core) 
BuildRequires: pkgconfig(notification)
BuildRequires: dbus-devel
BuildRequires: pkgconfig(dbus-glib-1)
BuildRequires: bluetooth-frwk-devel
BuildRequires: bundle-devel

%description
Headless notification service that collects incoming notifications from the
the platform, maintains a database of active notifications, and broadcast updates
to all listeners.

%package test
Summary: Test clients for %{name}
Group:   Application Framework/Notifications
%description test
This package provides unit test used in the development of the notification service.

%prep
%setup -q -n %{name}-%{version}

%define PREFIX %{_prefix}/apps/

%build
%autogen
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%make_install

%files
%defattr(-,root,root,-)
%{_bindir}/notification-service
%{_libdir}/systemd/system/notifications.service
%{_libdir}/systemd/system/notifications.socket
%{_libdir}/systemd/system/sockets.target.wants/notifications.socket

%files test
%defattr(-,root,root,-)
%{_bindir}/sample-display-client
%{_bindir}/send-notification
%{_bindir}/bluetooth_notification_client
