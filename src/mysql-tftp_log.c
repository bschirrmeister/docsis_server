/* mysql-tftp_log.c
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



void my_TFTPlog( char * fname, u_int32_t ipaddr ) {
	char	qbuf[900];
	char	*p, *dt, *tim;
	struct in_addr	inp;
	static char	*buf_pref = NULL;

	for (p=fname; *p; p++) {
		if (*p == 34) { *p = '`'; }
		if (*p == 39) { *p = '`'; }
		if (*p == 92) { *p = '/'; }
	}

	inp.s_addr = ipaddr;
	get_date_time( &dt, &tim );

	snprintf( qbuf, 900, "insert delayed into tftp_file_log "
		"( thedate, thetime, serverid, config_file, ipaddr ) "
		" values ( '%s', '%s', %d, '%s', '%s' )",
		dt, tim, server_id, fname, inet_ntoa( inp ) );

	if (buf_pref == NULL) {
		buf_pref = GetConfigVar( "suspend-tftp-logging" );
		if (*buf_pref == ' ') { buf_pref = "yes"; }
	}

// printf("tftp-log '%s'\n", qbuf2 );
	Save_SQL_Update( buf_pref, qbuf, CHECK_LOAD_YES );
}


void my_TFTP_dynamic_log( ModemSettings *msc ) {
	char	qbuf[900];
	char	dbuf[200], *dyn;
	char	*dt, *tim;
	int	i;
	// struct in_addr	inp;
	static char	*buf_pref = NULL;

	get_date_time( &dt, &tim );

	dyn = dbuf;
	for (i=0; i < msc->num_dyn; i++) {
		snprintf( dyn, 10, "%d,", msc->dynamic[i] );
		dyn += strlen( dyn );
	}

	snprintf( qbuf, 900, "insert delayed into tftp_dynamic_log "
		"( thedate, thetime, serverid, b_ipaddr, dynamic_config_file, "
		"static_ip, dynamic_ip ) values ('%s','%s',%d,'%lu','%s',%d,%d)",
		dt, tim, server_id, (unsigned long) ntohl( msc->ipaddr ), dbuf,
		msc->static_ip, msc->dynamic_ip );

	if (buf_pref == NULL) {
		buf_pref = GetConfigVar( "suspend-tftp-logging" );
		if (*buf_pref == ' ') { buf_pref = "yes"; }
	}

// printf("tftp-log '%s'\n", qbuf2 );
	Save_SQL_Update( buf_pref, qbuf, CHECK_LOAD_YES );
}



