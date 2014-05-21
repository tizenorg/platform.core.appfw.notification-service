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
BuildRequires: pkgconfig(notification-service)
BuildRequires: pkgconfig(dbus-1)
BuildRequires: pkgconfig(dbus-glib-1)
BuildRequires: pkgconfig(bluetooth-api)
BuildRequires: pkgconfig(bundle)
%{?systemd_requires}

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

%build
%autogen
make %{?_smp_mflags}

%install
%make_install
%install_service graphical.target.wants notifications.service

%post
%systemd_post notifications.service

%preun
%systemd_preun notifications.service

%postun
%systemd_postun notifications.service

%files
%defattr(-,root,root,-)
%{_bindir}/notification-service
%{_unitdir}/notifications.service
%{_unitdir}/graphical.target.wants/notifications.service

%files test
%defattr(-,root,root,-)
%{_bindir}/sample-display-client
%{_bindir}/send-notification
%{_bindir}/bluetooth_notification_client
