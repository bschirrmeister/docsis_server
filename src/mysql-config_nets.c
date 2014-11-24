/* mysql-config_nets.c
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


ConfigNets	*my_nets = NULL;
int		num_nets = 0;
int		cpe_net = 0;


/* ************************************************************** */

void  my_Clear_Nets( void ) {
	int i;

	if (my_nets == NULL) return;
	if (num_nets == 0) return;

	free( my_nets );
	my_nets = NULL;
	num_nets = 0;
	cpe_net = 0;
}

/* ************************************************************** */

ConfigNets *Get_Net_Opts( int netid ) {
	if (netid < 0) return NULL;
	if (netid > num_nets) return NULL;
	if (my_nets == NULL) return NULL;
	return( &(my_nets[netid]) );
}

/* ************************************************************** */

void  my_Load_Nets( void ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	char		*qbuf, network[21], *mask;
	int		nmask, i = 0, n = 0;
	u_int32_t	nmask2;
	ConfigNets	tmp;
	struct in_addr	inp;
	static time_t	old_time = 0;
	time_t		new_time = 0;

	new_time = time( NULL );
	if ( new_time - old_time < 3600 && my_nets != NULL) return;	// reload every hour
	old_time = new_time;

	my_Clear_Nets();

	qbuf = "select nettype, cmts_ip, cmts_vlan, network, gateway, grant_flag, dynamic_flag, "
		"full_flag, range_min, range_max, lease_time, config_opt1, config_opt2, "
		"config_opt3 from config_nets order by nettype, cmts_ip, cmts_vlan, inet_aton(range_min)";
	SQL_QUERY_RET( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) return;
	num_nets = mysql_num_rows( res );
	if (num_nets == 0) return;
	my_nets = (ConfigNets *) calloc(num_nets +1, sizeof(ConfigNets) );

	while ( myrow = mysql_fetch_row( res ) ) {
		memset( &my_nets[i], 0, sizeof(ConfigNets) );
		strncpy( my_nets[i].s_nettype, (myrow)[0], 7 );
		if (my_nets[i].s_nettype[1] == 'M') my_nets[i].nettype = NET_TYPE_CM;
		if (my_nets[i].s_nettype[1] == 'P') my_nets[i].nettype = NET_TYPE_CPE;
		if (my_nets[i].s_nettype[1] == 'T') my_nets[i].nettype = NET_TYPE_MTA;

		if (cpe_net == 0 && my_nets[i].nettype == NET_TYPE_CPE) cpe_net = i;

		strncpy( my_nets[i].s_cmts_ip, (myrow)[1], 20 );
		inet_aton( (myrow)[1], &inp ); my_nets[i].cmts_ip = inp.s_addr;

		my_nets[i].cmts_vlan = strtoul( (myrow)[2], NULL, 10 );

		strncpy( my_nets[i].s_network, (myrow)[3], 20 );
		strncpy( network, (myrow)[3], 20 );
		nmask = 0;
		mask = network;
		while(*mask != 0) {
			if (*mask == '/') {
				*mask = 0;
				mask++;
				nmask = strtoul( mask, NULL, 10 );
				mask--;
			}
			mask++;
		}
		if (nmask <= 0 || nmask > 30) { nmask = 24; }
		nmask2 = 0xFFFFFFFF << (32 - nmask);

		inet_aton( network, &inp );    my_nets[i].network = inp.s_addr;

		my_nets[i].mask = htonl( nmask2 );

		inet_aton( (myrow)[4], &inp ); my_nets[i].gateway = inp.s_addr;

		if (((myrow)[5][0] & 0x5F) == 'Y') my_nets[i].grant_flag = 1;
		// fprintf(stderr, "grant_flag  %d  %d  %d \n", (myrow)[5][0], ((myrow)[5][0] & 0x5F), 'Y' );

		if (((myrow)[6][0] & 0x5F) == 'Y') my_nets[i].dynamic_flag = CPE_DYNAMIC;

		if (((myrow)[7][0] & 0x5F) == 'Y') my_nets[i].full_flag = 1;

		inet_aton( (myrow)[8], &inp ); my_nets[i].range_min = inp.s_addr;

		inet_aton( (myrow)[9], &inp ); my_nets[i].range_max = inp.s_addr;

		my_nets[i].lease_time = strtoull( (myrow)[10], NULL, 10 );
		my_nets[i].opt1 = strtoul( (myrow)[11], NULL, 10 );
		my_nets[i].opt2 = strtoul( (myrow)[12], NULL, 10 );
		my_nets[i].opt3 = strtoul( (myrow)[13], NULL, 10 );

		/* Check Ranges */
		tmp.network = ntohl( my_nets[i].network );
		tmp.mask = nmask2;
		tmp.gateway = ntohl( my_nets[i].gateway );
		tmp.range_min = ntohl( my_nets[i].range_min );
		tmp.range_max = ntohl( my_nets[i].range_max );

		/* Check Network Address */
		if ( (tmp.network & tmp.mask) != tmp.network) {
			my_syslog( MLOG_ERROR, "dhcp: Load_Nets: " \
				"network address does not match subnet mask (%s %s)",
				(myrow)[1], (myrow)[3] );
			continue;
		}

		/* Calculate Broadcast Address */
		tmp.broadcast = 0xFFFFFFFF;
		tmp.broadcast -= tmp.mask;
		tmp.broadcast += tmp.network;
		my_nets[i].broadcast = htonl( tmp.broadcast );

		/* Check Gateway is in the network */
		if (!((tmp.gateway > tmp.network) && (tmp.broadcast > tmp.gateway))) {
			my_syslog( MLOG_ERROR, "dhcp: Load_Nets: " \
				"gateway address is not in the network specified (%s %s %s)",
				(myrow)[1], (myrow)[3], (myrow)[4] );
			continue;
		}
		/* Check Range_MIN is in the network */
		if (!((tmp.range_min > tmp.network) && (tmp.broadcast > tmp.range_min))) {
			my_syslog( MLOG_ERROR, "dhcp: Load_Nets: " \
				"range_min address is not in the network specified (%s %s %s)",
				(myrow)[1], (myrow)[3], (myrow)[8] );
			continue;
		}
		/* Check Range_MAX is in the network */
		if (!((tmp.range_max > tmp.network) && (tmp.broadcast > tmp.range_max))) {
			my_syslog( MLOG_ERROR, "dhcp: Load_Nets: " \
				"range_max address is not in the network specified (%s %s %s)",
				(myrow)[1], (myrow)[3], (myrow)[9] );
			continue;
		}
		/* Check that the Gateway in not between range min and max */
		if ((tmp.range_min < tmp.gateway) && (tmp.range_max > tmp.gateway)) {
			my_syslog( MLOG_ERROR, "dhcp: Load_Nets: " \
				"gateway address is between range min and max (%s %s %s (%s %s))",
				(myrow)[1], (myrow)[3], (myrow)[4], (myrow)[8], (myrow)[9] );
			continue;
		}
		/* Check that the range max is greater than the range min */
		if (tmp.range_min > tmp.range_max) {
			my_syslog( MLOG_ERROR, "dhcp: Load_Nets: " \
				"range min is greater than range max (%s %s %s %s)",
				(myrow)[1], (myrow)[3], (myrow)[8], (myrow)[9] );
			continue;
		}

		if (my_nets[i].nettype == NET_TYPE_CM) {
			for (n=tmp.range_min; n < tmp.range_max; n++) {
				u_int32_t n_ord = htonl( n );
				if (my_CheckCM_IP( n_ord )) {
					my_nets[i].last_used_ip = n;
					n = tmp.range_max;
				}
			}
		}
                if (my_nets[i].nettype == NET_TYPE_MTA) {
                        for (n=tmp.range_min; n < tmp.range_max; n++) {
                                u_int32_t n_ord = htonl( n );
                                if (my_CheckCM_IP( n_ord )) {
                                        my_nets[i].last_used_ip = n;
                                        n = tmp.range_max;
                                }
                        }
                }
		if (my_nets[i].nettype == NET_TYPE_CPE) {
			for (n=tmp.range_min; n < tmp.range_max; n++) {
				u_int32_t n_ord = htonl( n );
				if (my_CheckCPE_IP( n_ord )) {
					my_nets[i].last_used_ip = n;
					n = tmp.range_max;
				}
			}
		}
		i++;
	}
	mysql_free_result(res);
}



int  my_Get_Net( dhcp_message *message ) {
	u_int32_t	h_ipaddr, h_min, h_max;
	int		sn;
	struct in_addr	inp;

	my_Load_Nets();

	h_ipaddr = message->b_ipaddr;

	for( sn = 0; sn < num_nets; sn++ ) {
		if (my_nets[sn].nettype != message->lease_type) continue;
		if (my_nets[sn].cmts_vlan != message->vlan) continue; 
		if (message->in_pack.giaddr != 0) {
			if (my_nets[sn].cmts_ip != message->in_pack.giaddr) continue;
		}

		h_min = ntohl( my_nets[sn].range_min );
		h_max = ntohl( my_nets[sn].range_max );

		if ( (h_min <= h_ipaddr) && (h_max >= h_ipaddr) ) {
			message->lease_time = my_nets[sn].lease_time;
			message->netptr = sn;
			return 0; /* ok no error */
		}
	}
	fprintf(stderr, "no netptr mac %s ip %s\n", message->s_macaddr, message->s_ipaddr );
	return 1;  /* error */
}

/* ************************************************************** */

int Verify_Vlan( dhcp_message *message ) {
	u_int32_t	h_ipaddr, h_min, h_max;
	int		sn;

	if (num_nets == 0) return 0;

	if (message->in_pack.giaddr == 0) return 0;

	h_ipaddr = message->b_ipaddr;

	for( sn = 0; sn < num_nets; sn++ ) {
		if (my_nets[sn].nettype != message->lease_type) continue;
		if (my_nets[sn].cmts_vlan != message->vlan) continue;
		if (my_nets[sn].cmts_ip != message->in_pack.giaddr) continue;
		h_min = ntohl( my_nets[sn].range_min );
		if (h_min > h_ipaddr) continue;
		h_max = ntohl( my_nets[sn].range_max );
		if (h_max < h_ipaddr) continue;
		// range is good. 
		return 0;
	}
	fprintf(stderr, "bad verify vlan mac %s ip %s  gi %s  vlan %d  num_nets %d  leasetype %d \n",
		message->s_macaddr, message->s_ipaddr, message->in_giaddr,
		message->vlan, num_nets, message->lease_type );
	return 1;  /* wrong giaddr for vlan. reassign ip */
}

/* ************************************************************** */

void Update_Network_Full_Flag( int action_flag, int value ) {
	char	qbuf[200];

	if (my_nets == NULL) return;

	if (action_flag == FULLFLAG_seton) {
		snprintf( qbuf, 200, "update config_nets set full_flag = 'YES' " \
			"where nettype = '%s' and cmts_ip = '%s' " \
			"and cmts_vlan = %d and network = '%s' ",
			my_nets[value].s_nettype, my_nets[value].s_cmts_ip,
			my_nets[value].cmts_vlan, my_nets[value].s_network );
	}

	if (action_flag == FULLFLAG_setoff) {
		snprintf( qbuf, 200, "update config_nets set full_flag = 'NO' " \
			"where nettype = '%s' and cmts_ip = '%s' " \
			"and cmts_vlan = %d and network = '%s' ",
			my_nets[value].s_nettype, my_nets[value].s_cmts_ip,
			my_nets[value].cmts_vlan, my_nets[value].s_network );
	}

	if (action_flag == FULLFLAG_clearall) {
		snprintf( qbuf, 200, "update config_nets set full_flag = 'NO' where cmts_vlan = %d",
			value );
	}
	SQL_QUERY_RET( qbuf );
}

/* ************************************************************** */

void  my_CountUnusedIP( int *unused, int *total, u_int16_t cmts_vlan, u_int32_t cmts_ip ) {
	int			sn;
	unsigned long int	n_ord, h_min, h_max, ss;
	// struct in_addr		s_ip;
	int			x_unused, x_total, sn_unused;

	my_Load_Nets();
	x_unused = 0;
	x_total = 0;


	for( sn=0; sn < num_nets; sn++ ) {
		if (my_nets[sn].nettype == NET_TYPE_CPE &&
		    my_nets[sn].grant_flag == 1 &&
		    my_nets[sn].dynamic_flag == 0 &&
		    my_nets[sn].cmts_ip == cmts_ip &&
		    my_nets[sn].cmts_vlan == cmts_vlan) {
			sn_unused = 0;
			h_min = ntohl( my_nets[sn].range_min );
			h_max = ntohl( my_nets[sn].range_max );
			for( ss=h_min; ss <= h_max; ss++ ) {
				x_total++;
				n_ord = htonl( ss );
				if ( my_CheckCPE_IP( n_ord ) ) {
					// s_ip.s_addr = n_ord;
					// fprintf( stderr, "%lu %s \n", ss, inet_ntoa( s_ip ) );
					x_unused++;
					sn_unused++;
					if (my_nets[sn].full_flag == 1) {
						my_nets[sn].full_flag = 0;
						Update_Network_Full_Flag( FULLFLAG_setoff, sn);
					}
				}
			}
			if (sn_unused == 0)
				Update_Network_Full_Flag( FULLFLAG_seton, sn );
		}
	}
	*unused = x_unused;
	*total = x_total;
}
