/* my_sql_summerize.c
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



void  Log_Summerize( void ) {
	char		qbuf[500];
	char		*log_rotate, *oldlog_db;
	int		logrot = 0;

	log_rotate = GetConfigVar( "log-rotate-days" );
	if (*log_rotate == ' ') {
		fprintf(stderr,"log-rotate-days not configd\n" );
		return;
	}
	logrot = strtoul( log_rotate, NULL, 10 );
	if (logrot <= 0) return;

	oldlog_db = GetConfigVar( "mysql-oldlog-database" );
	if (*oldlog_db == ' ') {
		fprintf(stderr,"mysql-oldlog-database not configd\n" );
		return;
	}

	fprintf(stderr,"something something something\n" );
}
