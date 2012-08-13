/* mysql-cache_dhcp_conf.c
 *
 * docsis_server
 *
 * Copyright (C) 2006 Access Communications
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



typedef struct modem_cache {
	u_int64_t	b_macaddr;
	u_int32_t	ipaddr;
	u_int16_t	vlan;
	u_int64_t	subnum;
	char		*cfname;
	u_int8_t	static_ip;
	u_int8_t	dynamic_ip;
	u_int8_t	opt;
} ModemCache;

static ModemCache *the_cache = NULL;
int    cache_count;

/* ******************************************************************** */

void  my_Clear_MCACHE( void ) {
	int	i;

	if (the_cache == NULL) return;

	for (i=0; i < cache_count; i++) {
		if (the_cache[i].cfname != NULL)
			free( the_cache[i].cfname );
	}
	free( the_cache );
	the_cache = NULL;
	cache_count = 0;
}

/* ******************************************************************** */

void  my_Clear_MCACHE_Entry( dhcp_message *message ) {
	int	i;

	if (the_cache == NULL) return;

	i = my_findMAC_INDEX( message->b_macaddr );
	if (i >= 0)
		the_cache[i].ipaddr = 0;
}

/* ******************************************************************** */

void  my_MCACHE_Load( void ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	static time_t	old_time = 0;
	time_t		new_time;
	char		*qbuf;
	int		i = 0;

	new_time = time(NULL);
	if ( (new_time - old_time) < 600 ) return;	// ten minutes
	old_time = new_time;

	my_Clear_MCACHE();

	qbuf = "select cast(conv(docsis_modem.modem_macaddr,16,10) as unsigned) as modmac, "
		"docsis_update.ipaddr, cmts_vlan, subnum, config_file, "
		"static_ip, dynamic_ip, config_opt "
		"from docsis_modem left join docsis_update using (modem_macaddr) "
		"where docsis_modem.modem_macaddr is not null "
		"and docsis_update.ipaddr is not null "
		"order by modmac";
	SQL_QUERY_RET( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) { return; }

	cache_count = mysql_num_rows( res );
	if (cache_count == 0) { mysql_free_result(res); return; }

	the_cache = (ModemCache *) calloc(cache_count, sizeof(ModemCache) );

	while ( (myrow = mysql_fetch_row( res )) ) {
		u_int64_t	b_macaddr;
		struct in_addr	inp;

		if ((myrow)[0] == NULL) { cache_count--; continue; }
		if ((myrow)[1] == NULL) { cache_count--; continue; }

		b_macaddr = strtoull( (myrow)[0], NULL, 10);
		if (b_macaddr == 0) { cache_count--; continue; }
		the_cache[i].b_macaddr = b_macaddr;

		inet_aton( (myrow)[1], &inp );
		the_cache[i].ipaddr = inp.s_addr;
		if ((myrow)[2] != NULL)
			the_cache[i].vlan   = strtoul( (myrow)[2], NULL, 10 );
		if ((myrow)[3] != NULL)
			the_cache[i].subnum = strtoull( (myrow)[3], NULL, 10 );

		if ((myrow)[4] != NULL) {
			the_cache[i].cfname = (char *) calloc(1, strlen((myrow)[4]) + 2 );
			strncpy( the_cache[i].cfname, (myrow)[4], strlen((myrow)[4]) );
		}

		if ((myrow)[5] != NULL)
			the_cache[i].static_ip   = strtoul( (myrow)[5], NULL, 10 );
		if ((myrow)[6] != NULL)
			the_cache[i].dynamic_ip  = strtoul( (myrow)[6], NULL, 10 );
		if ((myrow)[7] != NULL)
			the_cache[i].opt         = strtoul( (myrow)[7], NULL, 10 );

		i++;
	}
	mysql_free_result(res);

	return;
}


/* ******************************************************************** */

// If we keep getting the same request over and over again
// we are allowed to start ignoring them.
int  ignore_repeated_customers( dhcp_message *message ) {
	time_t			new_time = 0;
	int			ipc_flag, cc;
	static u_int64_t	ipc1[100];
	static u_int32_t	ipip[100];
	static u_int32_t	ipc2[100];
	static u_int16_t	ipc3[100];
	static int		ipn = 0;

	new_time = time(NULL);

	ipc_flag = 1;
	for (cc=0; (cc < ipn) && ipc_flag; cc++) {
		if (message->b_ipaddr != 0 && message->b_ipaddr == ipip[cc]) {
			if ( new_time - ipc2[cc] < 10) {
				ipc3[cc]++;
				if (ipc3[cc] > 2) {
					// fprintf( stderr, "Repeatedly REJECT ip %s bad %d\n",
					//      message->s_ipaddr, ipc3[cc] );
					message->lease_type = LEASE_REJECT;
					return LEASE_REJECT;
				}
				ipc2[cc] = new_time;
			} else {
				ipc1[cc] = 0;
				ipip[cc] = 0;
				ipc2[cc] = 0;
				ipc3[cc] = 0;
			}
			ipc_flag = 0;
		}
		if (message->b_macaddr != 0 && message->b_macaddr == ipc1[cc]) {
			if ( new_time - ipc2[cc] < 10) {
				ipc2[cc] = new_time;
				ipc3[cc]++;
				if (ipc3[cc] > 6) {
					//fprintf( stderr, "Repeatedly REJECT mac %s bad %d\n",
					//	message->s_macaddr, ipc3[cc] );
					message->lease_type = LEASE_REJECT;
					return LEASE_REJECT;
				}
			} else {
				ipc1[cc] = 0;
				ipip[cc] = 0;
				ipc2[cc] = 0;
				ipc3[cc] = 0;
			}
			ipc_flag = 0;
		}
	}
	if ( ipc_flag ) {
		for (cc=0; (cc < ipn) && ipc_flag; cc++) {
			if ( (new_time - ipc2[cc]) > 10) {
				ipc1[cc] = message->b_macaddr;
				ipip[cc] = message->b_ipaddr;
				ipc2[cc] = new_time;
				ipc3[cc] = 1;
				ipc_flag = 0;
			}
		}
		if ( ipc_flag ) {
			if (ipn > 98) {
				ipc1[0] = message->b_macaddr;
				ipip[0] = message->b_ipaddr;
				ipc2[0] = new_time;
				ipc3[0] = 1;
			} else {
				ipc1[ipn] = message->b_macaddr;
				ipip[ipn] = message->b_ipaddr;
				ipc2[ipn] = new_time;
				ipc3[ipn] = 1;
				ipn++;
			}
		}
	}
	return LEASE_NOT_FOUND;
}


/* ******************************************************************** */
int  my_findMAC_INDEX( u_int64_t b_mac ) {
	int	low = 0, mid = 0;
	int	high = cache_count - 1;

	if (the_cache == NULL) return -1;

	while( high >= low ) {
		mid = ((high - low) >> 1) + low;
		if (the_cache[mid].b_macaddr == b_mac) return mid;
		if (the_cache[mid].b_macaddr >  b_mac) { high = mid - 1; continue; }
		if (the_cache[mid].b_macaddr <  b_mac) { low  = mid + 1; continue; }
	}
	return -1;
}

/* ******************************************************************** */


int   my_findMAC_CACHE( dhcp_message *message ) {
	static time_t	old_time = 0;
	time_t		new_time;
	int		i, cc;
	struct in_addr	inp;

	if (ignore_repeated_customers( message ) == LEASE_REJECT) {
		return LEASE_REJECT;
	}

	my_MCACHE_Load();

	if (High_Load_Flag) {
		i = my_findMAC_INDEX( message->b_macaddr );

		if (i == -1) return LEASE_NOT_FOUND;
		if (the_cache[i].ipaddr == 0) return LEASE_NOT_FOUND;

		message->lease_type = LEASE_CM;
		message->ipaddr = the_cache[i].ipaddr;
		message->b_ipaddr = ntohl( message->ipaddr );
		inp.s_addr = message->ipaddr;
		strncpy( message->s_ipaddr, inet_ntoa( inp ), 19 );
		strncpy( message->cfname, the_cache[i].cfname, MAX_CONFIG_FILE_NAME - 1 );
		message->vlan = the_cache[i].vlan;
		message->subnum = the_cache[i].subnum;
		message->static_ip = the_cache[i].static_ip;
		message->dynamic_ip = the_cache[i].dynamic_ip;
		message->opt = the_cache[i].opt;
		message->mcache = 1;
		if (Verify_Vlan(message)) {
			the_cache[i].ipaddr = 0;
			return LEASE_NOT_FOUND;
		}
		fprintf(stderr,"CACHE my_findMAC_CACHE\n");
		return LEASE_CM;
	}
	return LEASE_NOT_FOUND;
}

void  my_findCM_MAC_CACHE( dhcp_message *message ) {
	int		i;

	my_MCACHE_Load();

	i = my_findMAC_INDEX( message->b_modem_macaddr );
	if (i >= 0) message->subnum = the_cache[i].subnum;
	// fprintf(stderr, "findCM_MAC_CACHE index %d cc %d mac %s modmac %s subnum %llu \n",
	//	i, cache_count, message->s_macaddr, message->s_modem_macaddr, message->subnum );
}


/* ******************************************************************** */

