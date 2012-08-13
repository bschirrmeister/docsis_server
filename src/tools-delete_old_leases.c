/* delete_old_leases.c
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
	int		del_num, n, num_days;
	u_int16_t	vlan_id;

	if (argc < 2) {
		printf( "usage: %s vlan_id [num_ips_to_delete] [num_days]\n", argv[0] );
		printf( "num_ips will default to 10\n" );
		printf( "num_days will default to 45\n" );
		exit(1);
	}
	vlan_id = strtoul( argv[1], NULL, 10 );

	del_num = 10;
	if (argc >= 3) {
		del_num = strtoul( argv[2], NULL, 10 );
	}

	num_days = 45;
	if (argc >= 4) {
		num_days = strtoul( argv[3], NULL, 10 );
	}

	ReadConfigFile( 0 );
	InitSQLconnection();

	printf( "Deleting %d old leases \n", del_num );

	my_syslog(LOG_WARNING, "Delete Old Leases num_del=%d num_days=%d", del_num, num_days );

	for( n=0; n < del_num; n++ ) {
		my_DeleteOldestLease( vlan_id, num_days );
		printf( "\n" );
	}

	exit(0);
}
