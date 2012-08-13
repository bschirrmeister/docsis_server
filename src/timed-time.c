/* time.c
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


/* UDP port 37 Socket  */
int			time_serv_sock_num = 0;
struct sockaddr_in	time_serv_sock;
u_int32_t		time_client_ip;
u_int32_t		time_client_port;

extern int errno;

void initTimeSocket( void );
int getTimePacket( void );
void sendTimePacket( void );



void  time_server( void ) {
	struct passwd	*pass_struct;
	char		*euid_pref, *dbhost_pref, *force_chroot;
	time_t		the_time1, oldtime;
	int		cc = 0, retval = 0;

	InitSQLconnection();
	my_LoadChecker();

	initTimeSocket();

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

	my_syslog( MLOG_TIME, "docsis_server TIME Version %s activated", VERSION);

	oldtime = time(NULL);
	while(1) {
		// Update the PID file
		update_pid_file();

		// ping the MySQL server
		my_SQL_Ping();

		the_time1 = time( NULL );
		if ( (the_time1 - oldtime) > 3600) {
			oldtime = the_time1;
			my_syslog( MLOG_STATS, "time: requests %d", cc );
			cc = 0;
		}
		retval = getTimePacket();

		if (retval) {
			sendTimePacket();
			cc++;
		}
	}
}



void initTimeSocket( void ) {
	int	bind_res = 0;
	int	serv_sock_len = 0;

	serv_sock_len = sizeof( time_serv_sock );
	memset( &time_serv_sock, 0, serv_sock_len );

	time_serv_sock.sin_family = AF_INET;
	time_serv_sock.sin_port = htons( 37 );
	time_serv_sock.sin_addr.s_addr = INADDR_ANY;

	time_serv_sock_num = socket(AF_INET, SOCK_DGRAM, 0);
	if (time_serv_sock_num == -1) {
		my_syslog( MLOG_ERROR, "Cannot open Time Server Socket." );
		exit(1);
	}

	bind_res = bind( time_serv_sock_num, (struct sockaddr *) &time_serv_sock, serv_sock_len );
	if (bind_res == -1) {
		my_syslog( MLOG_ERROR, "Cannot Bind Time Server Socket to Port 37" );
		exit(1);
	}
}


int getTimePacket( void ) {
	unsigned char	buf[1024];
	int		bytes, retval;
	struct sockaddr_in	client_sock;
	socklen_t		client_len;
	fd_set			rfds;
	struct timeval		tv;

	FD_ZERO(&rfds);
	FD_SET(time_serv_sock_num, &rfds);
	tv.tv_sec = 60; /* we wait maximum 60 seconds */
	tv.tv_usec = 0;

	retval = select(time_serv_sock_num + 1, &rfds, NULL, NULL, &tv);
	if (!FD_ISSET(time_serv_sock_num, &rfds)) {
		/* Timeout from Select */
		return 0;
	}

	memset( buf, 0, 1000 );
	client_len = sizeof( client_sock );
	memset( &client_sock, 0, client_len );

	bytes = recvfrom( time_serv_sock_num, (char *) buf, 1000,
		0, (struct sockaddr *) &client_sock, &client_len );
	if (bytes < 0) {
		my_syslog( MLOG_ERROR, "time recvfrom: %s", strerror(errno) );
		return 0;
	}

	time_client_ip = client_sock.sin_addr.s_addr;
	time_client_port = client_sock.sin_port;
	return 1;
}


void sendTimePacket( void ) {
	struct sockaddr_in	send_sock;
	int		send_sock_len;
	int		send_res;
	int		send_len;
	time_t		the_time1;
	u_int32_t	the_time2;

	send_sock_len = sizeof( send_sock );
        memset( &send_sock, 0, send_sock_len );

	send_sock.sin_family = AF_INET;
	send_sock.sin_addr.s_addr = time_client_ip;
	send_sock.sin_port = time_client_port;

	the_time1 = time( NULL );
	the_time1 += 2208988800U;
	the_time2 = htonl( the_time1 );

	send_len = sizeof( the_time2 );
	send_res = sendto( time_serv_sock_num, (char *) &the_time2, send_len, 0,
			(struct sockaddr *) &send_sock, send_sock_len );

	if (send_res < 0) {
		my_syslog( MLOG_ERROR, "time sendto: %s", strerror(errno) );
	}
}



