Name: docsis_server
Summary: DHCP/TFTP/ToD/Syslog server for DOCSIS Cable Modem Termination Systems (CMTS)
Packager: Scott Wunsch <scott.wunsch@accesscomm.ca>
Version: 3.5
Release: 1
Source: %{name}-%{version}.tar.gz
License: GPL
Group: System Environment/Daemons
BuildRoot: %{_tmppath}/%{name}-buildroot
Requires: docsis
BuildRequires: mysql-devel

%define restart_flag /var/run/%{name}-restart-after-rpm-install

%description
docsis_server is a combination DHCP server, TFTP server, Time of Day server,
and a SYSLOG server.  In combination with a Data Over Cable Service Interface
Specification (DOCSIS) Cable Modem Termination System (CMTS) the docsis_server
is designed to boot up cable modems as well as customer PC equipment.

%prep
rm -rf %{buildroot}
%setup

%build
%configure
make

%install
%makeinstall
mkdir -p %{buildroot}/%{_sysconfdir}/rc.d/init.d
mkdir -p %{buildroot}/%{_sysconfdir}/cron.d
install -m0600 scripts_and_docs/docsis-server.conf %{buildroot}/%{_sysconfdir}/docsis-server.conf
install -m0755 scripts_and_docs/docsisd--Redhat_Startup_Script %{buildroot}/%{_sysconfdir}/rc.d/init.d/docsisd
install -m0755 scripts_and_docs/docsis_daemon.Watchdog %{buildroot}/%{_sbindir}/docsis_daemon.Watchdog
echo "*/6 * * * *   root " %{_sbindir}"/docsis_daemon.Watchdog" > %{buildroot}/%{_sysconfdir}/cron.d/docsisd_watchdog.cron
mkdir -p %{buildroot}/home/cable_modem_tftp
mkdir -p %{buildroot}/home/cable_modem_tftp/dynamic

%pre
/usr/sbin/useradd -c "DOCSIS Server" -s /sbin/nologin -r -d /tmp -M docsis 2>/dev/null || :

# Stop service during installation; keep flag if it was running to restart later
rm -f %{restart_flag}
/sbin/service docsisd status >/dev/null 2>&1
if [ $? -eq 0 ]; then
    touch %{restart_flag}
    /sbin/service docsisd stop >/dev/null 2>&1
fi

%post
/sbin/service crond restart
/sbin/chkconfig --add docsisd
# Restart if it had been running before installation
if [ -e %{restart_flag} ]; then
  rm %{restart_flag}
  /sbin/service docsisd start >/dev/null 2>&1
fi
exit 0

%preun
if [ $1 = 0 ]; then
 /bin/rm -f /tmp/docsis_server
 /usr/sbin/userdel docsisd 2>/dev/null || :
 /usr/sbin/groupdel docsisd 2>/dev/null || :
 [ -f /var/run/docsis_server.pid ] && /sbin/service docsisd stop > /dev/null 2>&1
 /sbin/chkconfig --del docsisd
fi


%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_sbindir}/docsis_server
%{_sbindir}/Count_IPs
%{_sbindir}/DB_Config_Encoder
%{_sbindir}/Delete_Old_Leases
%{_sbindir}/FindIP
%{_sbindir}/FindMAC
%{_sbindir}/ListMessages
%{_sbindir}/Stress_Config_Generator
%{_sbindir}/docsis_daemon.Watchdog
%{_sysconfdir}/rc.d/init.d/docsisd
%{_sysconfdir}/cron.d/docsisd_watchdog.cron
%config %{_sysconfdir}/docsis-server.conf
%dir /home/cable_modem_tftp
%dir /home/cable_modem_tftp/dynamic
%doc ChangeLog scripts_and_docs/*

%changelog
* Fri Feb 15 2008 Mike MacNeill <mikem@accesscomm.ca> 3.5-1
* Mon Aug 27 2007 Mike MacNeill <mikem@accesscomm.ca> 3.2-1
* Fri May 11 2007 Scott Wunsch <scott.wunsch@accesscomm.ca> 3.0-1
- First package
