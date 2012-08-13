/* mysql-cache_tftp_bin.c
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



typedef struct config_cache {
	u_int32_t	cfg_id;
	u_int8_t	*cfg_bin;
	u_int32_t	cfg_len;
	u_int8_t	cfg_ver;
} ConfigCache;

ConfigCache	*the_configs = NULL;
int		num_configs = 0;

/* ******************************************************************** */
void  Clear_Config_Cache( void ) {
	int	i;

	if (the_configs == NULL) return;

	for (i=0; i < num_configs; i++) {
		if (the_configs[i].cfg_bin != NULL)
			free( the_configs[i].cfg_bin );
	}
	free( the_configs );
	the_configs = NULL;
	num_configs = 0;
}
/* ******************************************************************** */

int   my_Check_for_New_Configs( void ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	char		*qbuf;
	time_t		real_time, last_update;
	static time_t	check_time = 0;
	time_t		update_time;

	last_update = check_time;
	real_time = time(NULL);
	if (real_time - check_time < 120) return 1;	// if less than 120 sec return "no change"
	check_time = real_time;

	qbuf = "select unix_timestamp( max(cfg_update) ) from config_modem_bin ";
	SQL_QUERY_RET1( qbuf );
        res = mysql_store_result( my_sock );
        if (res == NULL) { return 1; }

        myrow = mysql_fetch_row( res );
        if (myrow == NULL) { mysql_free_result(res); return 1; }
	if ( (myrow)[0] == NULL ) { mysql_free_result(res); return 1; }

	update_time = strtoll( (myrow)[0], NULL, 10 );

	if ( update_time > last_update ) {
		mysql_free_result(res);
		return 0;
	}
	mysql_free_result(res);
	return 1;
}



void  my_CONFIG_Load( void ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	unsigned long	*flen;
	char		*qbuf;
	long		i = 0;

	if (the_configs != NULL) {
		if ( my_Check_for_New_Configs() ) { return; }
		Clear_Config_Cache();
	}

	qbuf = "select cfg_id, cfg_bin, cfg_ver+0 from config_modem_bin order by cfg_id";
	SQL_QUERY_RET( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) { return; }
	num_configs = mysql_num_rows( res );
	the_configs = (ConfigCache *) calloc( num_configs +1, sizeof(ConfigCache) );

	while ( (myrow = mysql_fetch_row( res )) ) {
		flen = mysql_fetch_lengths( res );

		the_configs[i].cfg_id = strtoull( (myrow)[0], NULL, 10 );
		the_configs[i].cfg_len = flen[1];
		the_configs[i].cfg_bin = (u_int8_t *) calloc(1, flen[1] + 2 );
		memcpy( the_configs[i].cfg_bin, (myrow)[1], flen[1] );
		the_configs[i].cfg_ver = strtoul( (myrow)[2], NULL, 10 );
		i++;
	}
	mysql_free_result(res);
	my_Check_for_New_Configs();	// prep the check_time
	return;
}


/* ******************************************************************** */

int  add_Config_Item( u_int8_t *dst, u_int32_t idnum, u_int32_t maxsize, u_int8_t modver ) {
	int	high = 0, mid = 0, low = 0;

	if (idnum == 0) return 0;

	my_CONFIG_Load();
	high = num_configs - 1;

	if (the_configs == NULL) return 0;

	while (high >= low) {
		mid = ((high - low) >> 1) + low;
		if (the_configs[mid].cfg_id >  idnum) { high = mid - 1; continue; }
		if (the_configs[mid].cfg_id <  idnum) { low  = mid + 1; continue; }
		if (the_configs[mid].cfg_id == idnum) {
			if (the_configs[mid].cfg_len > maxsize) return 0;
			// if (the_configs[mid].cfg_ver != Modem_Ver_ANY) {
			//	if (the_configs[mid].cfg_ver != modver) return 0;
			//}
			memcpy( dst, the_configs[mid].cfg_bin, the_configs[mid].cfg_len );
			return the_configs[mid].cfg_len;
		}
	}
	return 0;
}

