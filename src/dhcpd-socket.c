/* dhcpd-socket.c
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


/* UDP port 67 Socket  */
int			serv_sock_num = 0;

extern int errno;


void init_DHCP_Socket( u_int32_t server_ip ) {
	int			bind_res = 0;
	int			serv_sock_len = 0;
	struct sockaddr_in	serv_sock;

	serv_sock_len = sizeof( serv_sock );
	memset( &serv_sock, 0, serv_sock_len );

	serv_sock.sin_family = AF_INET;
	serv_sock.sin_port = htons( 67 );
	if (server_ip == 0) {
		serv_sock.sin_addr.s_addr = INADDR_ANY;
	} else {
		serv_sock.sin_addr.s_addr = server_ip;
	}

	serv_sock_num = socket(AF_INET, SOCK_DGRAM, 0);
	if (serv_sock_num == -1) {
		fprintf( stderr, "Cannot Open Server Socket: %s\n",
			strerror(errno) );
		exit(1);
	}

	bind_res = bind( serv_sock_num, (struct sockaddr *) &serv_sock, serv_sock_len );
	if (bind_res == -1) {
		fprintf( stderr, "Cannot Bind Server Socket to Port 67: %s\n",
			strerror(errno) );
		exit(1);
	}
}

u_int32_t getInterfaceIP( char *interface_name ) {
	int			fd = -1;
	struct ifreq		ifr;
	struct sockaddr_in	*sin;
	u_int32_t		server_ip = 0;

	if (*interface_name == ' ') interface_name = "eth0";

	if((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0) {
		strcpy( ifr.ifr_name, interface_name );
		ifr.ifr_addr.sa_family = AF_INET;
		if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
			sin = (struct sockaddr_in *)&ifr.ifr_addr;
			server_ip = sin->sin_addr.s_addr;
		}
	}
	if (server_ip == 0) {
		fprintf( stderr, "Interface '%s' does not exist", interface_name );
		exit(1);
	}
	return( server_ip );
}

/* return 0 if successfull, -1 for failure */
int getPacket( dhcp_message *message ) {
	unsigned char		buf[2048];
	int			bytes, retval;
	struct sockaddr_in	client_sock;
	struct in_addr		s_ip;
	socklen_t		client_len;
	int		max_message_len = sizeof( dhcp_packet );
	fd_set			rfds;
	struct timeval		tv;

	FD_ZERO(&rfds);
	FD_SET(serv_sock_num, &rfds);
	tv.tv_sec = 10; /* we wait maximum 10 seconds */
	tv.tv_usec = 0;

	retval = select(serv_sock_num + 1, &rfds, NULL, NULL, &tv);
	if (!FD_ISSET(serv_sock_num, &rfds)) {
		/* Timeout from Select */
		return -2;
	}

	memset( buf, 0, 1000 );
	client_len = sizeof( client_sock );
	memset( &client_sock, 0, client_len );

	bytes = recvfrom( serv_sock_num, (char *) buf, max_message_len + 1,
		0, (struct sockaddr *) &client_sock, &client_len );
	if (bytes < 0) {
		my_syslog( MLOG_ERROR, "dhcp recvfrom: %s", strerror(errno) );
		return -1;
	}

	if (bytes > max_message_len) {
		my_syslog( MLOG_ERROR, "packet received is too big %d > %d",
			bytes, max_message_len );
		return -1;
	}
	memcpy(&message->in_pack, buf, bytes );

	if(message->in_pack.cookie != DHCP_MAGIC) {
		my_syslog(1, "client sent bogus message -- ignoring");
		return -1;
	}

	if (message->in_pack.hlen != 6 && message->in_pack.hlen != 0) {
		my_syslog(LOG_ERR, "MAC length is %d bytes", message->in_pack.hlen );
		return -1;
	}

	message->rcv_ipaddr = client_sock.sin_addr.s_addr;
	strcpy( message->s_rcv_ipaddr, inet_ntoa( client_sock.sin_addr ) );
	message->macaddr = message->in_pack.macaddr;
	cv_macaddr( message->macaddr, message->s_macaddr, 6 );
	cv_mac2num( &(message->b_macaddr), message->macaddr );
	s_ip.s_addr = message->in_pack.ciaddr;
	strcpy( message->in_ciaddr, inet_ntoa( s_ip ) );
	s_ip.s_addr = message->in_pack.yiaddr;
	strcpy( message->in_yiaddr, inet_ntoa( s_ip ) );
	s_ip.s_addr = message->in_pack.siaddr;
	strcpy( message->in_siaddr, inet_ntoa( s_ip ) );
	s_ip.s_addr = message->in_pack.giaddr;
	strcpy( message->in_giaddr, inet_ntoa( s_ip ) );
	message->in_bytes = bytes;

	return 0;
}


void sendPacket( dhcp_message *message ) {
	int		send_port;
	u_int32_t	send_ip;
	struct sockaddr_in	send_sock;
	int		send_sock_len;
	int		send_res;
	int		send_len;
	unsigned char	*flen;

	if (message->out_pack.giaddr == 0) {
		send_port = 68;
		send_ip = message->out_pack.ciaddr;
	} else {
		send_port = 67;
		send_ip = message->out_pack.giaddr;
	}

	send_sock_len = sizeof( send_sock );
        memset( &send_sock, 0, send_sock_len );

	send_sock.sin_family = AF_INET;
	send_sock.sin_addr.s_addr = send_ip;
	send_sock.sin_port = htons( send_port );

	send_len = sizeof( dhcp_packet );
	flen = (unsigned char *) &message->out_pack;
	while(  *(flen + send_len - 1) == 0 &&
		*(flen + send_len - 2) == 0 &&
		*(flen + send_len - 3) == 0 ) { send_len--; }

	send_res = sendto( serv_sock_num, (char *) &message->out_pack, send_len, 0,
			(struct sockaddr *) &send_sock, send_sock_len );

	if (send_res < 0) {
		my_syslog( MLOG_ERROR, "dhcp sendto: %s", strerror(errno) );
	}
}



