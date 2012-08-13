sbin_PROGRAMS =		\
	docsis_server	\
	ListMessages	\
	FindIP		\
	FindMAC		\
	Delete_Old_Leases \
	Count_IPs	\
	DB_Config_Encoder \
	Stress_Config_Generator

# set the include path found by configure
INCLUDES= $(all_includes) $(MYSQL_CFLAGS) -ldmalloc

# ##############################################################################################
docsis_server_LDADD = $(MYSQL_LIBS)

docsis_server_SOURCES =				\
	docsis_server.c				\
	backg-backgrounder.c			\
	dhcpd-dhcpd.c				\
	dhcpd-options.c				\
	dhcpd-socket.c				\
	mysql-mysql.c				\
	mysql-cache_dhcp_conf.c			\
	mysql-cache_tftp_bin.c			\
	mysql-cache_tftp_conf.c			\
	mysql-config_nets.c			\
	mysql-config_opts.c			\
	mysql-findip.c				\
	mysql-findmac.c				\
	mysql-misc_lease.c			\
	mysql-newip.c				\
	mysql-remote_cmds.c			\
	mysql-summerize.c			\
	mysql-syslog.c				\
	mysql-tftp_filegen1.c			\
	mysql-tftp_filegen2.c			\
	mysql-tftp_log.c			\
	mysql-update_leases.c			\
	mysql-update_modems.c			\
	mysql-utilities.c			\
	syslogd-syslog.c			\
	tftpd-file_cache.c			\
	tftpd-tftpd.c				\
	timed-time.c				\
	util_configfile.c			\
	util_executables.c			\
	util_macaddr.c				\
	util_md5.c				\
	util_time.c


#	tools-count_IPs.c
#	tools-db_config_encoder.c
#	tools-delete_old_leases.c
#	tools-find_ip.c
#	tools-find_mac.c
#	tools-list_messages.c
#	tools-stress_config_generator.c
#	tools-summerize_logs.c


# the library search path.
docsis_server_LDFLAGS = $(all_libraries)

# ##############################################################################################
ListMessages_LDADD = $(MYSQL_LIBS)

ListMessages_SOURCES = 	tools-list_messages.c 			\
			util_configfile.c			\
			util_time.c				\
			mysql-misc_lease.c			\
			mysql-syslog.c				\
			mysql-utilities.c			\
			mysql-mysql.c

ListMessages_LDFLAGS = $(all_libraries)

# ##############################################################################################
FindIP_LDADD = $(MYSQL_LIBS)

FindIP_SOURCES =	tools-find_ip.c				\
			util_configfile.c			\
			util_time.c				\
			mysql-misc_lease.c			\
			mysql-syslog.c				\
			mysql-utilities.c			\
			mysql-mysql.c

FindIP_LDFLAGS = $(all_libraries)

# ##############################################################################################
FindMAC_LDADD = $(MYSQL_LIBS)

FindMAC_SOURCES =	tools-find_mac.c			\
			util_configfile.c			\
			util_time.c				\
			mysql-misc_lease.c			\
			mysql-syslog.c				\
			mysql-utilities.c			\
			mysql-mysql.c

FindMAC_LDFLAGS = $(all_libraries)

# ##############################################################################################
Delete_Old_Leases_LDADD = $(MYSQL_LIBS)

Delete_Old_Leases_SOURCES = 	tools-delete_old_leases.c  		\
				util_configfile.c  			\
				util_time.c				\
				mysql-misc_lease.c			\
				mysql-syslog.c				\
				mysql-utilities.c			\
				mysql-mysql.c

Delete_Old_Leases_LDFLAGS = $(all_libraries)

# ##############################################################################################
Count_IPs_LDADD = $(MYSQL_LIBS)

Count_IPs_SOURCES =	tools-count_IPs.c			\
			util_configfile.c			\
			util_time.c				\
			util_executables.c			\
			util_macaddr.c				\
			mysql-config_nets.c			\
			mysql-newip.c				\
			mysql-misc_lease.c			\
			mysql-syslog.c				\
			mysql-update_leases.c			\
			mysql-update_modems.c			\
			mysql-utilities.c			\
			mysql-mysql.c

Count_IPs_LDFLAGS = $(all_libraries)

# ##############################################################################################
DB_Config_Encoder_LDADD = $(MYSQL_LIBS)

DB_Config_Encoder_SOURCES = 	tools-db_config_encoder.c		\
				util_configfile.c			\
				util_executables.c			\
				util_time.c				\
				mysql-misc_lease.c			\
				mysql-syslog.c				\
				mysql-tftp_filegen1.c			\
				mysql-utilities.c			\
				mysql-mysql.c

DB_Config_Encoder_LDFLAGS = $(all_libraries)

# ##############################################################################################
Stress_Config_Generator_LDADD = $(MYSQL_LIBS)

Stress_Config_Generator_SOURCES =	tools-stress_config_generator.c		\
					util_configfile.c			\
					util_time.c				\
					util_md5.c				\
					mysql-cache_tftp_bin.c			\
					mysql-cache_tftp_conf.c			\
					mysql-misc_lease.c			\
					mysql-syslog.c				\
					mysql-tftp_filegen2.c			\
					mysql-tftp_log.c			\
					mysql-utilities.c			\
					mysql-mysql.c

Stress_Config_Generator_LDFLAGS = $(all_libraries)

# ##############################################################################################


noinst_HEADERS = 				\
	docsis_server.h				\
	Version.h				\
	dhcpd-dhcpd.h				\
	dhcpd-options.h				\
	dhcpd-socket.h				\
	mysql-mysql.h				\
	tftpd-file_cache.h			\
	tftpd-tftpd.h				\
	util_configfile.h			\
	util_executables.h			\
	util_macaddr.h				\
	util_md5.h				\
	util_time.h

