/* syslog.c
 *
 * docsis_server
 *
 * Copyright (C) 2002 Access Communications
 *                    email docsis_guy@accesscomm.ca
 *
 * Designed for use with DOCSIS cable modems and hosts.
 * And for use with MySQL Database Server.
 *
 ********************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "docsis_server.h"


/* UDP port 514 Socket  */
int			slog_serv_sock_num = 0;
struct sockaddr_in	slog_serv_sock;

extern int errno;

void initSLogSocket( void );
void getSLogPacket( void );



void  syslog_server( void ) {
	struct passwd	*pass_struct;
	char		*euid_pref, *dbhost_pref, *force_chroot;

	InitSQLconnection();
	my_LoadChecker();

	initSLogSocket();

	dbhost_pref = GetConfigVar( "mysql-host" );
	euid_pref = GetConfigVar( "effective-userid" );
	force_chroot = GetConfigVar( "force-localhost-chroot" );
	pass_struct = getpwnam( euid_pref );

	if (strcmp(dbhost_pref,"localhost") || !strcmp(force_chroot, "yes")) {
		// We cannot chroot if connect to the DB via
		// a unix socket vs tcp
		chroot( CHROOT_PATH );
	}
	if (pass_struct) {
		seteuid( pass_struct->pw_uid );
		setegid( pass_struct->pw_gid );
	}

	my_syslog( MLOG_SYSLOG, "docsis_server SYSLOG Version %s activated", VERSION);

	while(1) {
		// Update PID file
		update_pid_file();

		// ping the MySQL server
		my_SQL_Ping();

		getSLogPacket();
	}
}



void initSLogSocket( void ) {
	int	bind_res = 0;
	int	serv_sock_len = 0;

	serv_sock_len = sizeof( slog_serv_sock );
	memset( &slog_serv_sock, 0, serv_sock_len );

	slog_serv_sock.sin_family = AF_INET;
	slog_serv_sock.sin_port = htons( 514 );
	slog_serv_sock.sin_addr.s_addr = INADDR_ANY;

	slog_serv_sock_num = socket(AF_INET, SOCK_DGRAM, 0);
	if (slog_serv_sock_num == -1) {
		my_syslog( MLOG_ERROR, "Cannot open Syslog Server Socket." );
		exit(1);
	}

	bind_res = bind( slog_serv_sock_num, (struct sockaddr *) &slog_serv_sock, serv_sock_len );
	if (bind_res == -1) {
		my_syslog( MLOG_ERROR, "Cannot Bind Syslog Server Socket to Port 514" );
		exit(1);
	}
}


void getSLogPacket( void ) {
	char		buf[1300];
	int		bytes, retval;
	struct sockaddr_in	client_sock;
	socklen_t		client_len;
	fd_set			rfds;
	struct timeval		tv;

	FD_ZERO(&rfds);
	FD_SET(slog_serv_sock_num, &rfds);
	tv.tv_sec = 60; /* we wait maximum 60 seconds */
	tv.tv_usec = 0;

	retval = select(slog_serv_sock_num + 1, &rfds, NULL, NULL, &tv);
	if (!FD_ISSET(slog_serv_sock_num, &rfds)) {
		/* Timeout from Select */
		return;
	}

	memset( buf, 0, 1290 );
	client_len = sizeof( client_sock );
	memset( &client_sock, 0, client_len );

	bytes = recvfrom( slog_serv_sock_num, buf, 1200,
		0, (struct sockaddr *) &client_sock, &client_len );
	if (bytes < 0) {
		my_syslog( MLOG_ERROR, "syslog recvfrom: %s", strerror(errno) );
	}

	my_LogSyslog( client_sock.sin_addr.s_addr, buf );
}



