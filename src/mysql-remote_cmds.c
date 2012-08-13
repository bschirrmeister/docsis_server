/* mysql-remote-cmds.c
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
extern char		*process_type;
extern u_int64_t	Super_Flag;
/* ******************************************************************** */

void  Update_Remote_Command( u_int32_t cmd_id, char *result ) {
	char			qbuf[400];

	snprintf( qbuf, 400, "update remote_command set donetime = now(), "
		"result = '%s' where cmd_id = %d", result, cmd_id );
	SQL_QUERY_RET( qbuf );
}

void  Clear_Remote_Commands( void ) {
	MYSQL_RES		*res;
	MYSQL_ROW		myrow;
	char			*qbuf;
	char			*result = "server restart";

	qbuf = "select cmd_id from remote_command where donetime = 0 order by cmd_id";

	SQL_QUERY_RET( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) return;
	while( (myrow = mysql_fetch_row( res )) ) {
		Update_Remote_Command( strtoul( (myrow)[0], NULL, 10 ), result );
	}
	mysql_free_result(res);
}

void  Check_Remote_Commands( void ) {
	MYSQL_RES		*res;
	MYSQL_ROW		myrow;
	static time_t		old_time = 0;
	static u_int32_t	timecc = 0;
	time_t			new_time = 0;
	char			qbuf[400];
	u_int32_t		cmd_id;
	char			command[101];
	char			*result;

	new_time = time(NULL);
	if (new_time - old_time < 5) return;
	old_time = new_time;

	Flush_SQL_Updates();	// do this too :)

	snprintf( qbuf, 400, "select cmd_id, lower(command) from remote_command "
		"where service = '%s' and donetime = 0 "
		"order by cmd_id limit 1", process_type );
	SQL_QUERY_RET( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) return;
	myrow = mysql_fetch_row( res );
	if (myrow == NULL) { mysql_free_result(res); return; }

	cmd_id = strtoul( (myrow)[0], NULL, 10 );
	strncpy( command, (myrow)[1], 100 );
	mysql_free_result(res);

	result = "?";
	if (!strcmp(process_type,"DHCP")) {
		if (!strcmp(command,"quit")) {
			DoCMD_DHCP_Quit();
			result = "ok";
			Update_Remote_Command( cmd_id, result );
			return;
		}
		if (!strcmp(command,"flush_config_nets")) {
			my_Clear_Nets();
			result = "ok";
			Update_Remote_Command( cmd_id, result );
			return;
		}
		if (!strcmp(command,"flush_config_opts")) {
			my_Clear_Opts();
			result = "ok";
			Update_Remote_Command( cmd_id, result );
			return;
		}
	}
	if (!strcmp(process_type,"TFTP")) {
		if (!strcmp(command,"quit")) {
			DoCMD_TFTP_Quit();
			result = "ok";
			Update_Remote_Command( cmd_id, result );
			return;
		}
	}
	result = "?";
	Update_Remote_Command( cmd_id, result );
}

/* ******************************************************************** */
