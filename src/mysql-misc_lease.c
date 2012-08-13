/* mysql-misc_lease.c
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



/* ******************************************************************** */

void my_CountOLDIP( u_int16_t cmts_vlan, int *ip45, int *ip30, int *ip15 ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	char		*qbuf1, qbuf2[500];
	int		in45, in30, in15;

	qbuf1 = "select count(*) from dhcp_leases "
		"where cmts_vlan = %d and lockip_flag = 'NO' "
		"and dynamic_flag = 'NO' "
		"and   start_time < subdate(now(),Interval %d day) "
		"and     end_time < subdate(now(),Interval %d day) "
		"and  update_time < subdate(now(),Interval %d day) ";

	sprintf( qbuf2, qbuf1, cmts_vlan, 45, 45, 45 );
	SQL_QUERY_RET( qbuf2 );
	res = mysql_store_result( my_sock );
	if (res == NULL) return;
	myrow = mysql_fetch_row( res );
	if (myrow != NULL) {
		in45 = strtoul( (myrow)[0], NULL, 10 );
	}
	mysql_free_result(res);

	sprintf( qbuf2, qbuf1, cmts_vlan, 30, 30, 30 );
	SQL_QUERY_RET( qbuf2 );
	res = mysql_store_result( my_sock );
	if (res == NULL) return;
	myrow = mysql_fetch_row( res );
	if (myrow != NULL) {
		in30 = strtoul( (myrow)[0], NULL, 10 );
	}
	mysql_free_result(res);

	sprintf( qbuf2, qbuf1, cmts_vlan, 15, 15, 15 );
	SQL_QUERY_RET( qbuf2 );
	res = mysql_store_result( my_sock );
	if (res == NULL) return;
	myrow = mysql_fetch_row( res );
	if (myrow != NULL) {
		in15 = strtoul( (myrow)[0], NULL, 10 );
	}
	mysql_free_result(res);

	*ip45 = in45;   *ip30 = in30;  *ip15 = in15;
}

/* ******************************************************************** */

void my_DeleteOldestLease( u_int16_t cmts_vlan, int days ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	char		qbuf[500];

	snprintf( qbuf, 500, "select ipaddr from dhcp_leases where cmts_vlan = %d "
		"and  lockip_flag = 'NO' and dynamic_flag = 'NO' "
		"and   start_time < subdate(now(),Interval %d day) "
		"and     end_time < subdate(now(),Interval %d day) "
		"and  update_time < subdate(now(),Interval %d day) "
		"order by update_time,start_time,end_time limit 1 ",
		cmts_vlan, days, days, days );

	SQL_QUERY_RET( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) { return; }
	myrow = mysql_fetch_row( res );
	if (myrow != NULL) {
		snprintf( qbuf, 500, "insert into dhcp_oldleases "
			"select *, now() from dhcp_leases where ipaddr = '%s'",
			(myrow)[0] );
		SQL_QUERY_RET( qbuf );

		snprintf( qbuf, 500, "delete from dhcp_leases where ipaddr = '%s'", (myrow)[0] );
		SQL_QUERY_RET( qbuf );

		printf( "Saved and Deleted - %s ", (myrow)[0] );
	} else {
		printf( "No IPs to delete" );
	}
	mysql_free_result( res );
}

/* ******************************************************************** */

void  Print_Log_Entries( int log_type, int num_lines ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	char		*qbuf = NULL, qbuf2[500];

	switch( log_type ) {
		case DHCP_LOG_TYPE:
			qbuf = "select thedate, thetime, serverid, "
				"concat(inet_ntoa(b_ipaddr), ' mac=', hex(b_macaddr), "
				"' modem=', hex(b_modem_macaddr), "
				"' sent=', sending, ' type=', type, ' cache=', mcache_flag) "
				"from dhcp_log "
				"order by thedate desc, thetime desc limit %d ";
			break;
		case TFTP_LOG_TYPE:
			qbuf = "select thedate, thetime, serverid, "
				"concat(ipaddr,' ',config_file) "
				"from tftp_file_log "
				"order by thedate desc, thetime desc limit %d ";
			break;
		case TFTP_DYNAMIC_LOG_TYPE:
			qbuf = "select thedate, thetime, serverid, "
				"concat( inet_ntoa(b_ipaddr), ' ', dynamic_config_file, ' ', "
				"'staticIP=', static_ip, ', ', "
				"'dynamicIP=', dynamic_ip ) "
				"from tftp_dynamic_log "
				"order by thedate desc, thetime desc limit %d ";
			break;
		case SYS_LOG_TYPE:
			qbuf = "select thedate, thetime, serverid, "
				"concat(priority,' ',message) "
				"from sys_log "
				"order by thedate desc, thetime desc limit %d ";
			break;
		default:
			return;
	}
	snprintf( qbuf2, 500, qbuf, num_lines );
	SQL_QUERY_RET( qbuf2 );
	res = mysql_store_result( my_sock );
	if (res == NULL) { return; }
	while ( (myrow = mysql_fetch_row( res )) ) {
		int i = strtoul( (myrow)[2], NULL, 10 );
		printf( "%s %s id=%d %s\n", (myrow)[0], (myrow)[1], i, (myrow)[3] );
	}
	mysql_free_result( res );
}
