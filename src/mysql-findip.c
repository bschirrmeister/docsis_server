/* mysql-findip.c
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

/* ******************************************************************** */
extern MYSQL		*my_sock;
extern float		my_LoadAvg;
extern float		my_MaxLoad;
extern char		*process_type;
extern u_int64_t	Super_Flag;
/* ******************************************************************** */


int   my_findIP( dhcp_message *message, int find_type ) {
	char		qbuf[500], *ip;
	int		res;
	struct in_addr	inp;
        my_syslog(LOG_WARNING, "EXISTING IP addresses to give to gi %s mac %s lease %d docsis type %d",
                message->in_giaddr, message->s_macaddr, message->lease_type, message->in_opts.docsis_modem );

	ip = message->s_ipaddr; /* find_type = IPADDR */
	if (find_type == FIND_CIADDR) { ip = message->in_ciaddr; } else
	if (find_type == FIND_YIADDR) { ip = message->in_yiaddr; }

	inet_aton( ip, &inp );
	message->b_ipaddr = ntohl( inp.s_addr );
	strcpy( message->s_ipaddr, ip );
	ignore_repeated_customers( message );
	if (message->lease_type == LEASE_REJECT) return 0;

	snprintf( qbuf, 500, "select macaddr, subnum, dynamic_flag, modem_macaddr, "
		"unix_timestamp(end_time) - unix_timestamp() "
		"from dhcp_leases where ipaddr = '%s'", ip );

	res = my_findIP_QUERY( qbuf, message );
	if ( res ) {
		message->lease_type = LEASE_CPE;
		return 1;
	}

	snprintf( qbuf, 500, "select modem_macaddr, subnum, 'NO', modem_macaddr, "
		"1 as lease_time from docsis_modem "
		"where ipaddr = '%s'", ip );

	res = my_findIP_QUERY( qbuf, message );
	if ( res ) {
		message->lease_type = LEASE_CM;
		return 1;
	}

	return 0;
}

int   my_findIP_QUERY( char *qbuf, dhcp_message *message ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;

	SQL_QUERY_RET0( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) return 0;

	myrow = mysql_fetch_row( res );
	if (myrow == NULL) { mysql_free_result(res); return 0; }

	strncpy( message->s_macaddr, (myrow)[0], 20 );
	cv_addrmac( message->macaddr, message->s_macaddr, 6 );
	cv_mac2num( &(message->b_macaddr), message->macaddr );
	if (message->b_macaddr == 0) { mysql_free_result(res); return 0; }

	message->subnum = strtoull( (myrow)[1], NULL, 10 );

	if (((myrow)[2][0] & 0x5F) == 'Y')
		message->cpe_type = CPE_DYNAMIC;

	if (message->b_modem_macaddr == 0) {
		strcpy( message->s_modem_macaddr, (myrow)[3] );
		cv_addrmac( message->modem_macaddr, message->s_modem_macaddr, 6 );
		cv_mac2num( &(message->b_modem_macaddr), message->modem_macaddr );
		if (message->b_modem_macaddr == 0)	// 123456789012
			strcpy( message->s_modem_macaddr, "000000000000" );
	}

	message->lease_time = strtoull( (myrow)[4], NULL, 10 );

	mysql_free_result(res);

	my_findCM_MAC_CACHE( message );
	return 1;
}

