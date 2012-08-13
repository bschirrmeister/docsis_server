/* mysql_update_leases.c
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
extern MYSQL		*my_pdns_sock;
extern u_int8_t		server_id;
extern char		*process_type;
extern u_int64_t	Super_Flag;
/* ******************************************************************** */


char  *Lookup_ConfigOptsMacs( dhcp_message *message, int opt_type ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	int		numr;
	char		qbuf[120];
	static char	*retval = NULL;

	if (retval == NULL) retval = (char *) calloc(1,300);

	snprintf(qbuf,120, "select opt_value from config_opts_macs where "
		"macaddr = '%s' and opt_id = 1 and opt_type = %d",
		message->s_macaddr, opt_type );
	SQL_QUERY_RETNULL( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) { return NULL; }
	numr = mysql_num_rows( res );
	if (numr == 0) { mysql_free_result(res); return NULL; }
	myrow = mysql_fetch_row( res );
	if (myrow == NULL) { mysql_free_result(res); return NULL; }
	strncpy( retval, (myrow)[0], 300 );
	mysql_free_result(res);
	return( retval );
}

char  *Lookup_ConfigOpts( dhcp_message *message, int opt_type ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	int		numr;
	ConfigNets	*netopts;
	char		qbuf[500];
	static char	*retval = NULL;

	if (retval == NULL) retval = (char *) calloc(1,300);

	netopts = Get_Net_Opts( message->netptr );
	snprintf(qbuf,500, "select opt_value from config_opts where "
		"server_id = %d and opt_type = %d and opt_id in (%d,%d,%d) order by opt_id",
		server_id, opt_type, netopts->opt1, netopts->opt2, netopts->opt3 );
	SQL_QUERY_RETNULL( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) { return NULL; }
	numr = mysql_num_rows( res );
	if (numr == 0) { mysql_free_result(res); return NULL; }
	myrow = mysql_fetch_row( res );
	if (myrow == NULL) { mysql_free_result(res); return NULL; }
	strncpy( retval, (myrow)[0], 300 );
	mysql_free_result(res);
	return( retval );
}

void  my_Update_PowerDNS( dhcp_message *message ) {
	MYSQL_RES	*res;
	char		*hname, *dname;
	char		qbuf[500];
	int		numr;

	hname = Lookup_ConfigOptsMacs( message, 12 );
	if (hname == NULL) return;
	dname = Lookup_ConfigOpts( message, 15 );
	if (dname == NULL) return;

	snprintf(qbuf,500,"delete from records where "
		"name = concat('%s','.','%s') and content != '%s'", hname, dname, message->s_ipaddr );
	fprintf(stderr,"qv %d %60s\n", strlen(qbuf), qbuf );
	if ( mysql_query( my_pdns_sock, qbuf ) ) {
		fprintf( stderr, "sql error\nq=%s \ne=%s\n", qbuf,
			mysql_error( my_pdns_sock ) ); return;
	}

	snprintf(qbuf,500,"delete from records where "
		"name != concat('%s','.','%s') and content = '%s'", hname, dname, message->s_ipaddr );
	fprintf(stderr,"qv %d %60s\n", strlen(qbuf), qbuf );
	if ( mysql_query( my_pdns_sock, qbuf ) ) {
		fprintf( stderr, "sql error\nq=%s \ne=%s\n", qbuf,
			mysql_error( my_pdns_sock ) ); return;
	}

	snprintf(qbuf,500,"select * from records where name = concat('%s','.','%s') and content = '%s'",
		hname, dname, message->s_ipaddr );
	fprintf(stderr,"qv %d %60s\n", strlen(qbuf), qbuf );
	if ( mysql_query( my_pdns_sock, qbuf ) ) {
		fprintf( stderr, "sql error\nq=%s \ne=%s\n", qbuf,
			mysql_error( my_pdns_sock ) ); return;
	}
	res = mysql_store_result( my_pdns_sock );
	if (res != NULL) {
		numr = mysql_num_rows( res );
		if (numr != 0) { mysql_free_result(res); return; }
		mysql_free_result(res);
	}

	snprintf(qbuf,500,"insert into records (domain_id,name,type,content,ttl) values "
		"( (select id from domains where name = '%s'), "
		"concat('%s','.','%s'),'A','%s',600)",
		dname, hname, dname, message->s_ipaddr );

	fprintf(stderr,"qv %d %60s\n", strlen(qbuf), qbuf );
	if ( mysql_query( my_pdns_sock, qbuf ) ) {
		fprintf( stderr, "sql error\nq=%s \ne=%s\n", qbuf,
			mysql_error( my_pdns_sock ) ); return;
	}
}

/* ****************************************************************** */

void  my_WriteNewCPELease( dhcp_message *message ) {
	char	*s_dynam;
	char	qbuf[220];

	s_dynam = "NO";
	if (message->cpe_type == CPE_DYNAMIC) { s_dynam = "YES"; }

	snprintf( qbuf, 220, "replace into dhcp_leases "
		"( ipaddr, macaddr, start_time, end_time, update_time, "
		"cmts_vlan, dynamic_flag, subnum ) "
		" values ( '%s', '%s', now(), now(), now(), %d, '%s', %llu ) ",
		message->s_ipaddr, message->s_macaddr, message->vlan,
		s_dynam, message->subnum );
	SQL_QUERY_RET( qbuf );
	my_Update_PowerDNS( message );
}

/* ****************************************************************** */

void  my_UpdateLease( dhcp_message *message ) {
	char	qbuf[500];

	snprintf( qbuf, 100, "delete from dhcp_leases "
		"where macaddr = '%s-x' and lockip_flag = 'NO' ",
		message->s_macaddr );
	SQL_QUERY_RET( qbuf );

	if (message->in_opts.agent_mac_len > 0) {
		snprintf( qbuf, 100, "update dhcp_leases "
			"set modem_macaddr = '%s' where ipaddr = '%s' ",
			message->s_modem_macaddr, message->s_ipaddr );
		SQL_QUERY_RET( qbuf );
	}

	if (message->in_opts.host_name_len > 0) {
		snprintf( qbuf, 500, "update dhcp_leases "
			"set pc_name = '%s' where ipaddr = '%s' ",
			message->in_opts.host_name, message->s_ipaddr );
		SQL_QUERY_RET( qbuf );
	}

	if (message->subnum > 0) {
		snprintf( qbuf, 100, "update dhcp_leases "
			"set subnum = '%llu' where ipaddr = '%s' ",
			message->subnum, message->s_ipaddr );
		SQL_QUERY_RET( qbuf );
	}

	snprintf( qbuf, 150, "update dhcp_leases set start_time = now(), "
		"end_time = date_add(now(),interval %ld second), "
		"cmts_vlan = %d where ipaddr = '%s' ",
		message->lease_time, message->vlan, message->s_ipaddr );
	SQL_QUERY_RET( qbuf );
	my_Update_PowerDNS( message );
}


/* ****************************************************************** */


void  my_UpdateTime( dhcp_message *message ) {
	char	qbuf[500];

	snprintf( qbuf, 500, "update dhcp_leases set update_time = now() " \
		"where ipaddr = '%s' ", message->s_ipaddr );
	SQL_QUERY_RET( qbuf );
}


/* ****************************************************************** */

void  my_DeleteLease( dhcp_message *message ) {
	char    qbuf[500];

	snprintf( qbuf, 500, "insert into dhcp_oldleases "
		"select *, now() from dhcp_leases where ipaddr = '%s' and lockip_flag = 'NO'",
		message->s_ipaddr );
	SQL_QUERY_RET( qbuf );

	snprintf( qbuf, 500, "delete from dhcp_leases where ipaddr = '%s' and lockip_flag = 'NO'",
		message->s_ipaddr );
	SQL_QUERY_RET( qbuf );

	strcpy( message->s_ipaddr, "" );
	message->b_ipaddr = 0;
	message->ipaddr = 0;
	message->vlan = 0;
}


/* ****************************************************************** */
