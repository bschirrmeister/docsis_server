/* count_IPs.c
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


typedef struct _net_count {
	u_int32_t	cmts_ip;
	int		cnt;

	struct _net_count *next;
} net_count;


/* ******************************************************************** */
extern ConfigNets	*my_nets;
extern int		num_nets;
/* ******************************************************************** */



int main( int argc, char *argv[] ) {
	int		unused = 0, total = 0;
	u_int16_t	vlan_id;
	int		g_unused = 0, g_total = 0;
	int		old45 = 0, old30 = 0, old15 = 0;
	float		percent_unused;
	net_count	counter, *x = NULL;
	struct in_addr sin;
	int		sn, ii, jj;
	char		tmpstr[50];

	if (argc < 2) {
		printf( "usage: %s vlan_id\n", argv[0] );
		exit(1);
	}

	vlan_id = strtoul( argv[1], NULL, 10 );

	ReadConfigFile( 0 );
	InitSQLconnection();
	my_Load_Nets();

	counter.cmts_ip = 0;
	counter.cnt = 0;
	counter.next = NULL;

	for( sn=0; sn < num_nets; sn++ ) {
		if (my_nets[sn].nettype == NET_TYPE_CPE &&
		    my_nets[sn].grant_flag == 1 &&
		    my_nets[sn].cmts_vlan == vlan_id) {
			x = &counter;
			while( x != NULL ) {
				if (x->cmts_ip == my_nets[sn].cmts_ip) {
					x->cnt++;
					x = NULL;
				} else {
					if (x->next == NULL) {
						x->next = malloc(sizeof(net_count));
						x->next->cmts_ip = my_nets[sn].cmts_ip;
						x->next->cnt = 0;
						x->next->next = NULL;
					}
					x = x->next;
				}
			}
		}
	}

	printf( "   VLAN   CMTS             TOTAL   IPs           IPs \n");
	printf( "          GI IP            IP      USED          unUSED \n");
	// printf( "   99     123.456.789.012  99999   99999 (99%)   99999 (99%) \n");

	x = &counter;
	while( x != NULL ) {
		if (x->cmts_ip == 0) { x = x->next; continue; }

		sin.s_addr = x->cmts_ip;
		printf( "   %2d     %s", vlan_id, inet_ntoa( sin ) );
		ii = strlen( inet_ntoa( sin ) );
		for (jj=ii; jj < 17; jj++) printf(" ");

		my_CountUnusedIP( &unused, &total, vlan_id, x->cmts_ip );
		percent_unused = ( (float) unused / (float) total ) * 100;

		g_unused += unused;
		g_total += total;

		printf( "%d", total );
		snprintf(tmpstr, 50, "%d", total );
		ii = strlen( tmpstr );
		for (jj=ii; jj < 8; jj++) printf(" ");

		printf( "%d", (total - unused) );
		snprintf(tmpstr, 50, "%d", (total - unused) );
		ii = strlen( tmpstr );
		for (jj=ii; jj < 6; jj++) printf(" ");

		printf( "(%2.0f\%)   ", (100.0 - percent_unused) );

		printf( "%d", unused );
		snprintf(tmpstr, 50, "%d", unused );
		ii = strlen( tmpstr );
		for (jj=ii; jj < 6; jj++) printf(" ");

		printf( "(%2.0f\%)\n", percent_unused );

		x = x->next;
	}
	
	my_CountOLDIP( vlan_id, &old45, &old30, &old15 );

	printf( "\n\n" );
	printf( " IPs stale for 45 days = %d\n", old45 );
	printf( " Actual IP Used = %d\n", (g_total - g_unused - old45) );
	printf( " Actual IP Unused = %d\n", (g_unused + old45) );

	printf( "\n" );
	printf( " IPs stale for 30 days = %d\n", old30 );
	printf( " Actual IP Used = %d\n", (g_total - g_unused - old30) );
	printf( " Actual IP Unused = %d\n", (g_unused + old30) );

	printf( "\n" );
	printf( " IPs stale for 15 days = %d\n", old15 );
	printf( " Actual IP Used = %d\n", (g_total - g_unused - old15) );
	printf( " Actual IP Unused = %d\n", (g_unused + old15) );

	exit(0);
}
