/* mysql-newip.c
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
extern u_int8_t		server_id;
extern char		*process_type;
extern u_int64_t	Super_Flag;
/* ******************************************************************** */

/* ******************************************************************** */
extern ConfigNets	*my_nets;
extern int		num_nets;
extern int		cpe_net;
/* ******************************************************************** */


void my_getNewIP_CM( dhcp_message *message ) {
	int		sn;
	u_int32_t	n_ord, h_min, h_max, ss;
	struct in_addr	inp;

	if (server_id > 1) return;
	my_Load_Nets();

	for( sn=0; sn < num_nets; sn++ ) {
		if (my_nets[sn].nettype != LEASE_CM) continue;
		if (my_nets[sn].cmts_ip != message->in_pack.giaddr) continue;
		if (my_nets[sn].cmts_vlan != message->vlan) continue;
		if (my_nets[sn].full_flag == 1) { continue; }

		h_min = ntohl( my_nets[sn].range_min );
		h_max = ntohl( my_nets[sn].range_max );
		if (my_nets[sn].last_used_ip < h_min) my_nets[sn].last_used_ip = h_min;
		if (my_nets[sn].last_used_ip >= h_max) my_nets[sn].last_used_ip = h_min;

		for( ss=my_nets[sn].last_used_ip; ss <= h_max; ss++ ) {
			n_ord = htonl( ss );
			if ( my_CheckCM_IP( n_ord ) ) {
				message->lease_type = LEASE_CM;
				message->ipaddr = n_ord;
				inp.s_addr = n_ord;
				strncpy( message->s_ipaddr, inet_ntoa( inp ), 20 );
				message->b_ipaddr = ntohl( n_ord );
				message->lease_time = my_nets[sn].lease_time;
				my_nets[sn].last_used_ip = ss + 1;
				my_WriteNewCMLease( message );
				return;
			}
		}
		my_nets[sn].last_used_ip = 0;
	}

}

void my_getNewIP_CPE( dhcp_message *message ) {
	int		sn, i;
	int		ip_stat = 0, ip_dyn = 0, lease_type = 0, cpe_type = 0;
	u_int32_t	n_ord, h_min, h_max, ss;
	char		*alert_pgm;
	struct in_addr	inp;
	static time_t	old_time = 0;
	static time_t	old_time2 = 0;
	static char	*ip_pref = NULL;
	time_t		new_time = 0;

	if (server_id > 1) return;
	my_Load_Nets();

	if (ip_pref == NULL) ip_pref = GetConfigVar( "dhcp-ip-mgmt" );

	my_Count_Allowed_IPs( message );
	my_Count_Used_IPs( message );

	lease_type = LEASE_NOAUTH;
	ip_stat = message->static_ip - message->static_cur;
	if (ip_stat > 0) {
		lease_type = LEASE_CPE;
		cpe_type = CPE_STATIC;
	} else {
		ip_dyn  = message->dynamic_ip - message->dynamic_cur;
		if (ip_dyn > 0) {
			lease_type = LEASE_CPE;
			cpe_type = CPE_DYNAMIC;
		}
	}
	if (*ip_pref != 'y') {
		if (lease_type == LEASE_NOAUTH) {
			lease_type = LEASE_CPE;
			if (message->static_ip > 0) {
				cpe_type = CPE_STATIC;
			} else {
				cpe_type = CPE_DYNAMIC;
			}
		}
	}

	for( sn=0; sn < num_nets; sn++ ) {
		//fprintf(stderr,"cmts_ip %lu   vs  giaddr %lu \n", my_nets[sn].cmts_ip, message->in_pack.giaddr );
		//fprintf(stderr,"grant_flag  %d \n", my_nets[sn].grant_flag );
		//fprintf(stderr,"full_flag   %d \n", my_nets[sn].full_flag  );
		//fprintf(stderr,"nettype %d    vs  lease_type %d \n", my_nets[sn].nettype, lease_type );
		//fprintf(stderr,"dyn_flag %d   vs  cpe_type %d \n", my_nets[sn].dynamic_flag, cpe_type );

		if (my_nets[sn].cmts_ip != message->in_pack.giaddr) { continue; }
		if (my_nets[sn].grant_flag == 0) { continue; }
		if (my_nets[sn].full_flag == 1) { continue; }
		if (my_nets[sn].nettype != lease_type) { continue; }
		if (my_nets[sn].dynamic_flag != cpe_type) { continue; }

		h_min = ntohl( my_nets[sn].range_min );
		h_max = ntohl( my_nets[sn].range_max );
		if (my_nets[sn].last_used_ip < h_min) my_nets[sn].last_used_ip = h_min;
		if (my_nets[sn].last_used_ip >= h_max) my_nets[sn].last_used_ip = h_min;

		for( ss=my_nets[sn].last_used_ip; ss <= h_max; ss++ ) {
			n_ord = htonl( ss );
			i = my_CheckCPE_IP( n_ord );
			// fprintf(stderr,"i = %d \n",i);
			if ( i ) {
				message->lease_type = lease_type;
				message->cpe_type = cpe_type;
				message->ipaddr = n_ord;
				inp.s_addr = n_ord;
				strncpy( message->s_ipaddr, inet_ntoa( inp ), 20 );
				message->b_ipaddr = ntohl( n_ord );
				message->vlan = my_nets[sn].cmts_vlan;
				message->lease_time = my_nets[sn].lease_time;
				my_nets[sn].last_used_ip = ss + 1;
				my_WriteNewCPELease( message );
				return;
			}
		}
		my_nets[sn].full_flag = 1;
		my_nets[sn].last_used_ip = 0;
		Update_Network_Full_Flag( FULLFLAG_seton, sn );
	}

	my_syslog(LOG_WARNING, "NAK -- no IP addresses to give gi %s mac %s",
		message->in_giaddr, message->s_macaddr );

	for( sn=0; sn < num_nets; sn++ ) {
		int dummy1, dummy2;

		if (my_nets[sn].cmts_ip != message->in_pack.giaddr) { continue; }
		if (my_nets[sn].grant_flag == 0) { continue; }
		my_nets[sn].last_used_ip = 0;
		my_CountUnusedIP( &dummy1, &dummy2, my_nets[sn].cmts_vlan, message->in_pack.giaddr );
	}

	new_time = time( NULL );
	if ( (new_time - old_time) > 60 ) {	/* if there are no IPs available */
		old_time = new_time;		/* reload every minute */

		my_Load_Nets();

		if ( (new_time - old_time2) > 900 ) {  /* send alert every 15 minutes */
			old_time2 = new_time;

			alert_pgm = GetConfigVar( "alert-program" );
			if (*alert_pgm != ' ') {
				RunExit( alert_pgm, "NO_IP", message->in_giaddr );
			}
		}
	}
}



int my_CheckCPE_IP( u_int32_t ipaddr ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	char		qbuf[500], *ip;
	int		numr;
	struct in_addr	tmpval;

	tmpval.s_addr = ipaddr;
	ip = inet_ntoa( tmpval );

	snprintf( qbuf, 500, "select macaddr from dhcp_leases where ipaddr = '%s'", ip );
	SQL_QUERY_RET0( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) return 0;
	numr = mysql_num_rows( res );
	if (numr > 0) { mysql_free_result(res); return 0; }

	//fprintf(stderr,"numr is %d \n", numr );
	mysql_free_result(res);
	return 1;		// no results. IP is free.
}

int my_CheckCM_IP( u_int32_t ipaddr ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	char		qbuf[75], *ip;
	int		numr;
	struct in_addr	tmpval;

	tmpval.s_addr = ipaddr;
	ip = inet_ntoa( tmpval );

	snprintf( qbuf, 75, "select modem_macaddr from docsis_update where ipaddr = '%s'", ip );
	SQL_QUERY_RET0( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) return 0;
	numr = mysql_num_rows( res );
	if (numr > 0) { mysql_free_result(res); return 0; }

	mysql_free_result(res);
	return 1;		// no results. IP is free.
}




void my_Count_Allowed_IPs( dhcp_message *message ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	char		qbuf[500];

	snprintf( qbuf, 500, "select subnum, static_ip, dynamic_ip "
		"from docsis_modem where modem_macaddr = '%s'",
		message->s_modem_macaddr );
	SQL_QUERY_RET( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) return;
	myrow = mysql_fetch_row( res );
	if (myrow == NULL) { mysql_free_result(res); return; }

	message->subnum = strtoull( (myrow)[0], NULL, 10 );
	message->static_ip = strtoul( (myrow)[1], NULL, 10 );
	message->dynamic_ip = strtoul( (myrow)[2], NULL, 10 );

	mysql_free_result(res);
}

void my_Count_Used_IPs( dhcp_message *message ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	char		qbuf[500], *use_subnum;

	use_subnum = GetConfigVar( "use-subnum" );
	if (*use_subnum == 'y') {
		snprintf( qbuf, 500, "select ipaddr, dynamic_flag from dhcp_leases " \
			"where modem_macaddr = '%s' and subnum = %llu ",
			message->s_modem_macaddr, message->subnum );
	} else {
		snprintf( qbuf, 500, "select ipaddr, dynamic_flag from dhcp_leases " \
			"where modem_macaddr = '%s' ", message->s_modem_macaddr );
	}
	SQL_QUERY_RET( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) return;
	while ( (myrow = mysql_fetch_row( res )) ) {
		if ( strcmp((myrow)[1],"YES") == 0 ) {
			message->dynamic_cur++;
		} else {
			message->static_cur++;
		}
	}
	mysql_free_result(res);
}
