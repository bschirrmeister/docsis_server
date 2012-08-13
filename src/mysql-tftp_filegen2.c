/* my_sql_tftp_filegen2.c
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

// Part 2  Takes the small snippets of binary config files and
//         puts them together for a specific modem.

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


int  gen_Config_File( u_int32_t ipaddr, u_int8_t *dst, u_int32_t maxsize ) {
	int		cfg_ip;
	u_int32_t	bytes, savb;
	ModemSettings	*msc;
	struct stat	tftp_stat;
	int		i;
	static char	*key = NULL;
	static char	*ip_pref = NULL;
	static int	keylen = 0;

	if (key == NULL) {
		key = GetConfigVar( "config-secret-key" );
		if (*key == ' ') { key = "secret-key"; }
		keylen = strlen( key );
	}

	if (ip_pref == NULL) {
		ip_pref = GetConfigVar( "ip-http-mgmt" );
	}

	msc = my_findModem_Settings( ipaddr );

	if (msc == NULL) {
		fprintf(stderr,"could not find modem settings (%ld)\n", ipaddr );
		return 0;
	}

	if ( stat( msc->config_file, &tftp_stat ) == 0 ) {
		fprintf(stderr,"no dynamic needed '%s' exists\n", msc->config_file );
		return 0;
	}

	bytes = 0;

	for (i=0; i < msc->num_dyn; i++) {	// NOTE: version_flag is not used.
		bytes += add_Config_Item( dst + bytes, msc->dynamic[i], maxsize, msc->version_flag );
	}

	cfg_ip = msc->static_ip + msc->dynamic_ip;
	if (cfg_ip > 16) cfg_ip = 16;
	if (*ip_pref == 'y') cfg_ip = 16;
	bytes += add_Config_Item( dst + bytes, cfg_ip, maxsize, msc->version_flag );

	bytes = docsis_add_cm_mic( dst, bytes, maxsize );
	bytes = docsis_add_cmts_mic( dst, bytes, maxsize, (unsigned char *) key, keylen );
	bytes = docsis_add_eod_and_pad( dst, bytes, maxsize );

	fprintf(stderr, "dynamic config file %d bytes\n", bytes );
	return( bytes );
}

/* ******************************************************************** */
/* ******************************************************************** */

void  Config_StressTest( void ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	u_int8_t	buffer[20000];
	char		*qbuf;
	struct in_addr	inp;
	int		cc, c2, bytes;
	long		totbyte = 0;
	time_t		t1, t2;

	qbuf = "select distinct ipaddr from docsis_update";
	SQL_QUERY_RET( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) { return; }

	cc = c2 = 0; t1 = time(NULL);
	while ( (myrow = mysql_fetch_row( res )) ) {
		cc++;
		inet_aton( (myrow)[0], &inp );
		bytes = gen_Config_File( inp.s_addr, buffer, 20000 );
		totbyte += bytes;
		if (bytes > 0) c2++;
	}
	t2 = time(NULL);

	printf( "We checked %d modems and generated %d actual config files\n", cc, c2 );
	printf( "The files use %ld bytes (%ld KB) (%ld MB)\n", totbyte, (totbyte / 1024), (totbyte /1024/1024)); 
	printf( "It took %d seconds.\n\n", (int) (t2 - t1) );
}

/* ******************************************************************** */

