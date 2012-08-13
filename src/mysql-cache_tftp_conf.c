/* mysql-cache_tftp_conf.c
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

static ModemSettings	*mset = NULL;
static int		num_mset = 0;

/* ******************************************************************** */

// typedef struct modem_settings {
//	u_int32_t	ipaddr;
//	int		version_flag;
//	int		num_dyn;
//	u_int32_t	*dynamic;
//	int		static_ip;
//	int		dynamic_ip;
//	char		*config_file;
//	char		*dynamic_file;
//} ModemSettings;

void  my_Modem_Settings_Clear( void ) {
	int	i;

	if (mset == NULL) return;

	for (i=0; i < num_mset; i++) {
		if (mset[i].dynamic != NULL)
			free( mset[i].dynamic );
		if (mset[i].config_file != NULL)
			free( mset[i].config_file );
		if (mset[i].dynamic_file != NULL)
			free( mset[i].dynamic_file );
	}
	free( mset );
	mset = NULL;
	num_mset = 0;
}

/* ******************************************************************** */

void  my_Modem_Settings_Load( void ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	static time_t	old_time = 0;
	time_t		new_time;
	char		*qbuf;
	int		i = 0;
	struct in_addr	s_ip;

	new_time = time(NULL);
	if ( (new_time - old_time) < 600 ) return;
	old_time = new_time;

	my_Modem_Settings_Clear();

	qbuf = "select docsis_update.ipaddr, docsis_update.mce_us_sid, "
	"config_file, dynamic_config_file, static_ip, dynamic_ip from docsis_modem, docsis_update "
	"where docsis_modem.modem_macaddr = docsis_update.modem_macaddr and ipaddr is not null "
	"and config_file is not null order by inet_aton(ipaddr)";
	SQL_QUERY_RET( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) { return; }
	num_mset = mysql_num_rows( res );
	mset = (ModemSettings *) calloc(num_mset + 1, sizeof(ModemSettings));
	while ( (myrow = mysql_fetch_row( res )) ) {
		int		mce_us_sid = 0, ips = 0;
		char		dyn[257], *p, *q;
		u_int32_t	d[100], x = 0;

		inet_aton( (myrow)[0], &s_ip );
		mset[i].ipaddr = s_ip.s_addr;

		if ((myrow)[1] != NULL ) { mce_us_sid = strtoul( (myrow)[1], NULL, 10 ); }
		if (mce_us_sid == 0) mset[i].version_flag = Modem_Ver_v1_0;
		if (mce_us_sid > 0)  mset[i].version_flag = Modem_Ver_v1_1;
		
		mset[i].config_file = (char *) calloc(1, strlen((myrow)[2]) +2);
		strcpy( mset[i].config_file, (myrow)[2] );

		strncpy( dyn, (myrow)[3], 256 );
		mset[i].static_ip  = strtoul( (myrow)[4], NULL, 10 );
		mset[i].dynamic_ip = strtoul( (myrow)[5], NULL, 10 );
		ips = mset[i].static_ip + mset[i].dynamic_ip;
		mset[i].dynamic_file = (char *) calloc(1, strlen(dyn) + 10);
		snprintf( mset[i].dynamic_file, strlen(dyn) + 10, "%s,%d", dyn, ips );
		p = dyn;
		while( *p != 0) {
			d[x] = 0;
			d[x] = strtoul( p, &q, 10); p = q;
			if (d[x] != 0 && x < 100) x++;
			if (*p == ',') p++;
		}
		mset[i].num_dyn = x;
		mset[i].dynamic = (u_int32_t *) calloc(x +1, sizeof(u_int32_t) );
		memcpy( mset[i].dynamic, d, sizeof(u_int32_t) * x );

		i++;
	}
	mysql_free_result(res);

	return;
}

/* ******************************************************************** */

int  my_findModem_INDEX( u_int32_t ipaddr ) {
	int	low = 0, mid = 0;
	int	high = num_mset - 1;

	if (mset == NULL) return -1;

	while( high >= low ) {
		mid = ((high - low) >> 1) + low;
		if (mset[mid].ipaddr == ipaddr) return mid;
		if (mset[mid].ipaddr >  ipaddr) high = mid - 1;
		if (mset[mid].ipaddr <  ipaddr) low  = mid + 1;
	}
	fprintf(stderr,"CACHE not found tftp_cache\n");
	return -1;
}

/* ******************************************************************** */

ModemSettings  *my_findModem_Settings( u_int32_t ipaddr ) {
	MYSQL_RES		*res;
	MYSQL_ROW		myrow;
	char			qbuf[500];
	int			i = 0;
	char			s_ipaddr[20];
	struct in_addr		s_ip;
	static ModemSettings	*p = NULL;
	int			mce_us_sid = 0, ips = 0;
	char			dyn[256], *m, *q;
	u_int32_t		d[100], x = 0;

	if (High_Load_Flag) {
		my_Modem_Settings_Load();
		i = my_findModem_INDEX( ipaddr );
		if (i >= 0) return &( mset[i] );
	}

	s_ip.s_addr = ipaddr;
	strcpy( s_ipaddr, inet_ntoa( s_ip ) );
	if (p == NULL) {
		p = (ModemSettings *) calloc(1, sizeof( ModemSettings ) );
		p->config_file = (char *) calloc(1, 75 );
		p->dynamic_file = (char *) calloc(1, 300 );
		p->dynamic = (u_int32_t *) calloc(100, sizeof( u_int32_t ) );
	} else {
		p->ipaddr = 0;
		p->version_flag = 0;
		p->num_dyn = 0;
		p->static_ip = 0;
		p->dynamic_ip = 0;
		memset( p->config_file, 0, 75 );
		memset( p->dynamic_file, 0, 300 );
		memset( p->dynamic, 0, 100 * sizeof( u_int32_t ) );
	}

	snprintf( qbuf, 500, "select docsis_update.ipaddr, docsis_update.mce_us_sid, "
	"config_file, dynamic_config_file, static_ip, dynamic_ip from docsis_modem, docsis_update "
	"where docsis_modem.modem_macaddr = docsis_update.modem_macaddr and ipaddr is not null "
	"and config_file is not null and ipaddr = '%s'", s_ipaddr );
	SQL_QUERY_RETNULL( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) return NULL;
	myrow = mysql_fetch_row( res );
	if (myrow == NULL) return NULL;

	p->ipaddr = ipaddr;

	if ((myrow)[1] != NULL ) { mce_us_sid = strtoul( (myrow)[1], NULL, 10 ); }
	if (mce_us_sid == 0) p->version_flag = Modem_Ver_v1_0;
	if (mce_us_sid > 0)  p->version_flag = Modem_Ver_v1_1;
		
	strncpy( p->config_file, (myrow)[2], 75 );
	strncpy( dyn, (myrow)[3], 256 );
	p->static_ip  = strtoul( (myrow)[4], NULL, 10 );
	p->dynamic_ip = strtoul( (myrow)[5], NULL, 10 );
	ips = p->static_ip + p->dynamic_ip;
	snprintf( p->dynamic_file, 299, "%s,%d", dyn, ips );
	m = dyn;
	while( *m != 0) {
		d[x] = 0;
		d[x] = strtoul( m, &q, 10); m = q;
		if (d[x] != 0 && x < 100) x++;
		if (*m == ',') m++;
	}
	if (x > 100) x = 100;
	p->num_dyn = x;
	memcpy( p->dynamic, d, sizeof(u_int32_t) * x );

	mysql_free_result(res);
	return( p );
}


