/* list_messages.c
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


int main( int argc, char *argv[] ) {
	int log_type = 0;
	int num_mess = 20;

	if (argc < 2) {
		printf( "usage: %s log_type [number of messages]\n", argv[0] );
		printf( "where log_type is either 'dhcp','dtftp','tftp' or 'sys'\n" );
		printf( "      and num of messages defaults to 20 \n" );
		exit(1);
	}

	if (strcmp(argv[1],"dhcp") == 0) log_type = DHCP_LOG_TYPE;
	if (strcmp(argv[1],"tftp") == 0) log_type = TFTP_LOG_TYPE;
	if (strcmp(argv[1],"dtftp") == 0) log_type = TFTP_DYNAMIC_LOG_TYPE;
	if (strcmp(argv[1],"sys") == 0)  log_type = SYS_LOG_TYPE;

	if (log_type == 0) {
		printf( "usage: %s log_type [number of messages]\n", argv[0] );
		printf( "where log_type is either 'dhcp','tftp' or 'sys'\n" );
		printf( "      and num of messages defaults to 20 \n" );
		exit(1);
	}

	if (argc >= 3) {
		num_mess = strtoul( argv[2], NULL, 10 );
		if (num_mess < 1 || num_mess > 10000) {
			printf( "usage: %s log_type [number of messages]\n", argv[0] );
			printf( "where log_type is either 'dhcp','tftp' or 'sys'\n" );
			printf( "      and num of messages no bigger than 10000 \n" );
			exit(1);
		}
	}

	ReadConfigFile( 0 );
	InitSQLconnection();

	Print_Log_Entries( log_type, num_mess );

	exit(0);
}
