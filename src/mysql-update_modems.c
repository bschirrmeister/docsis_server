/* mysql-update_modems.c
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


void  my_WriteNewCMLease( dhcp_message *message ) {
	char	qbuf[500];

	snprintf( qbuf, 500, "replace into docsis_update ( ipaddr, modem_macaddr, thetime ) "
		"values ( '%s', '%s', now() ) ", message->s_ipaddr, message->s_macaddr );
	SQL_QUERY_RET( qbuf );
	my_Update_PowerDNS( message );
}

void  my_UpdateAgent( dhcp_message *message ) {
	char	qbuf[1000];
	char	*mce_concat, *mce_ver, *mce_frag, *mce_phs, *mce_igmp;
	char	*mce_bpi, *mce_filt_dot1p, *mce_filt_dot1q, *mce_dcc;
	char	version[12], mce_ds_said[10], mce_us_sid[10], mce_tetps[10], mce_ntet[10];

	if (message->in_opts.agent_mac_len == 0) return;

	mce_concat = "NULL";
	if (message->in_opts.mce_concat == 0) mce_concat = "'NO'";
	if (message->in_opts.mce_concat == 1) mce_concat = "'YES'";
	mce_ver = "NULL";
	if (message->in_opts.mce_ver == 0) mce_ver = "'v1.0'";
	if (message->in_opts.mce_ver == 1) mce_ver = "'v1.1'";
	if (message->in_opts.mce_ver == 2) mce_ver = "'v2.0'";
	mce_frag = "NULL";
	if (message->in_opts.mce_frag == 0) mce_frag = "'NO'";
	if (message->in_opts.mce_frag == 1) mce_frag = "'YES'";
	mce_phs = "NULL";
	if (message->in_opts.mce_phs == 0) mce_phs = "'NO'";
	if (message->in_opts.mce_phs == 1) mce_phs = "'YES'";
	mce_igmp = "NULL";
	if (message->in_opts.mce_igmp == 0) mce_igmp = "'NO'";
	if (message->in_opts.mce_igmp == 1) mce_igmp = "'YES'";
	mce_bpi = "NULL";
	if (message->in_opts.mce_bpi == 0) mce_bpi = "'BPI'";
	if (message->in_opts.mce_bpi == 1) mce_bpi = "'BPI+'";
	mce_filt_dot1p = "NULL";
	if (message->in_opts.mce_filt_dot1p == 0) mce_filt_dot1p = "'NO'";
	if (message->in_opts.mce_filt_dot1p == 1) mce_filt_dot1p = "'YES'";
	mce_filt_dot1q = "NULL";
	if (message->in_opts.mce_filt_dot1q == 0) mce_filt_dot1q = "'NO'";
	if (message->in_opts.mce_filt_dot1q == 1) mce_filt_dot1q = "'YES'";
	mce_dcc = "NULL";
	if (message->in_opts.mce_dcc == 0) mce_dcc = "'NO'";
	if (message->in_opts.mce_dcc == 1) mce_dcc = "'YES'";

	strcpy( version, "" );
	if (*message->in_opts.version != 0)
		snprintf( version, 12, "%s", message->in_opts.version );
	strcpy( mce_ds_said, "NULL" );
	if (message->in_opts.mce_ds_said >= 0)
		snprintf( mce_ds_said, 10, "'%d'", message->in_opts.mce_ds_said );
	strcpy( mce_us_sid, "NULL" );
	if (message->in_opts.mce_us_sid >= 0)
		snprintf( mce_us_sid, 10, "'%d'", message->in_opts.mce_us_sid );
	strcpy( mce_tetps, "NULL" );
	if (message->in_opts.mce_tetps >= 0)
		snprintf( mce_tetps, 10, "'%d'", message->in_opts.mce_tetps );
	strcpy( mce_ntet, "NULL" );
	if (message->in_opts.mce_ntet >= 0)
		snprintf( mce_ntet, 10, "'%d'", message->in_opts.mce_ntet );

	snprintf( qbuf, 1000, "replace delayed into docsis_update ( modem_macaddr, ipaddr, cmts_ip, "
		"agentid, version, mce_concat, mce_ver, mce_frag, mce_phs, mce_igmp, mce_bpi, "
		"mce_ds_said, mce_us_sid, mce_filt_dot1p, mce_filt_dot1q, mce_tetps, mce_ntet, "
		"mce_dcc, thetime ) values "
		"( '%s','%s','%s','%s','%s', %s,%s,%s,%s,%s,  %s,%s,%s,%s,%s,  %s,%s,%s, now() )",
		message->s_macaddr, message->s_ipaddr, message->in_giaddr,
		message->in_opts.s_agentid, version,
		mce_concat, mce_ver, mce_frag, mce_phs, mce_igmp, mce_bpi, mce_ds_said, mce_us_sid,
		mce_filt_dot1p, mce_filt_dot1q, mce_tetps, mce_ntet, mce_dcc );
	SQL_QUERY_RET( qbuf );
}

/* ******************************************************************** */

int  my_findAgent_QUERY( char *qbuf, dhcp_message *message );

int   my_findAgent( dhcp_message *message ) {
	char		qbuf[500];
	packet_opts	*po = &(message->in_opts);

	if (((message->lease_type == LEASE_CM)) || ((message->lease_type == LEASE_MTA))) {
		po->modem_mac_len = 6;
		memcpy( po->modem_mac, message->macaddr, po->modem_mac_len );
		strncpy( po->s_modem_mac, message->s_macaddr, 1024 );
		po->b_modem_mac = message->b_macaddr;

		snprintf( qbuf, 500, "select agentid from docsis_update where macaddr = '%s'",
			message->s_macaddr );

		return my_findAgent_QUERY( qbuf, message );
	}

	if (message->lease_type == LEASE_CPE) {
		if (message->b_modem_macaddr == 0) return 0;

		po->modem_mac_len = 6;
		memcpy( po->modem_mac, message->modem_macaddr, po->modem_mac_len );
		strncpy( po->s_modem_mac, message->s_modem_macaddr, 1024 );
		po->b_modem_mac = message->b_modem_macaddr;

		snprintf( qbuf, 500, "select agentid from docsis_update where modem_macaddr = '%s'",
			message->s_modem_macaddr );

		return my_findAgent_QUERY( qbuf, message );
	}
	return 0;
}

int  my_findAgent_QUERY( char *qbuf, dhcp_message *message ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;

	SQL_QUERY_RET0( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) return 0;
	myrow = mysql_fetch_row( res );
	if (myrow == NULL) {  mysql_free_result(res); return 0; }

	strncpy( message->in_opts.s_agentid, (myrow)[0], 1024 );
	message->in_opts.agentid_len = ( strlen( message->in_opts.s_agentid ) / 2 );
	cv_addrmac( message->in_opts.agentid, message->in_opts.s_agentid, message->in_opts.agentid_len );

	message->in_opts.agent_mac_len = message->in_opts.agentid_len + 
					 message->in_opts.modem_mac_len + 4;

	mysql_free_result(res);
	return 1;
}


/* ******************************************************************** */

