/* mysql-config_opts.c
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
extern u_int8_t		server_id;
extern char		*process_type;
extern u_int64_t	Super_Flag;
/* ******************************************************************** */

ConfigOpts	*my_opts = NULL;
ConfigOpts	*my_opts_mac = NULL;

int		num_opts = 0;
int		num_opts_mac = 0;


/* ************************************************************** */

void my_Clear_Opts( void ) {
	int i;

	if (my_opts != NULL) {
		for (i=0; i < num_opts; i++) {
			if (my_opts[i].val != NULL)
				free( my_opts[i].val );
		}
		free( my_opts );
		my_opts = NULL;
		num_opts = 0;
	}

	if (my_opts_mac != NULL) {
		for (i=0; i < num_opts_mac; i++) {
			if (my_opts_mac[i].val != NULL)
				free( my_opts_mac[i].val );
		}
		free( my_opts_mac );
		my_opts_mac = NULL;
		num_opts_mac = 0;
	}
}

/* ************************************************************** */

int Query_Opt_CACHE( u_int16_t opt_id, u_int64_t b_mac ) {
	static ConfigOpts	*cfo = NULL;
	int			retval;

	if (cfo == NULL) {
		cfo = (ConfigOpts *) calloc(1,sizeof(ConfigOpts) );
		cfo->val = (u_int8_t *) calloc(1,2048);
	}

	retval = Query_Opt( cfo, opt_id, b_mac );
	if (retval == 0) return -1;
	if (cfo->len <= 0) return -1;

	if (b_mac > 0) {
		my_opts_mac[num_opts_mac].b_macaddr = cfo->b_macaddr;
		my_opts_mac[num_opts_mac].id = cfo->id;
		my_opts_mac[num_opts_mac].len = cfo->len;
		my_opts_mac[num_opts_mac].val = (u_int8_t *) calloc(1, cfo->len +2);
		memcpy( my_opts_mac[num_opts_mac].val, cfo->val, cfo->len );

		num_opts_mac++;
		return( num_opts_mac - 1);
	}
	my_opts[num_opts].b_macaddr = cfo->b_macaddr;
	my_opts[num_opts].id = cfo->id;
	my_opts[num_opts].len = cfo->len;
	my_opts[num_opts].val = (u_int8_t *) calloc(1, cfo->len +2);
	memcpy( my_opts[num_opts].val, cfo->val, cfo->len );

	num_opts++;
	return( num_opts - 1);
}

/* ************************************************************** */

void  my_Load_Opts( void ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	char		qbuf[200];
	char		*qbuf2;
	u_int16_t	id;
	u_int64_t	b_mac;
	int		num;
	static time_t	old_time = 0;
	time_t		new_time;

	new_time = time(NULL);
	if ( (new_time - old_time) < 600 ) return;	// ten minutes
	old_time = new_time;

	my_Clear_Opts();

	snprintf( qbuf, 200, "select opt_id from config_opts where "
			"server_id = %d group by opt_id order by opt_id", server_id );
	SQL_QUERY_RET( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) return;
	num = mysql_num_rows( res );
	if (num == 0) { mysql_free_result(res); return; }
	my_opts = (ConfigOpts *) calloc(num, sizeof(ConfigOpts));
	while ( (myrow = mysql_fetch_row( res )) ) {
		id = strtoul( (myrow)[0], NULL, 10 );
		Query_Opt_CACHE( id, 0 );
	}
	mysql_free_result(res);

	qbuf2 = "select cast(conv(macaddr,16,10) as unsigned) as b_mac from config_opts_macs "
		"where opt_id = 1 group by b_mac order by b_mac";
	SQL_QUERY_RET( qbuf2 );
	res = mysql_store_result( my_sock );
	if (res == NULL) return;
	num = mysql_num_rows( res );
	if (num == 0) { mysql_free_result(res); return; }
	my_opts_mac = (ConfigOpts *) calloc(num, sizeof(ConfigOpts));
	while ( (myrow = mysql_fetch_row( res )) ) {
		b_mac = strtoll( (myrow)[0], NULL, 10 );
		Query_Opt_CACHE( 1, b_mac );
	}
	mysql_free_result(res);
}

/* ************************************************************** */

int  Convert_Opt( u_int8_t *outbuf, u_int8_t type, u_int8_t dtype, char *value, u_int64_t b_mac ) {
	u_int8_t	*len, *tmp3 = NULL;
	char		tmp[301], *tmp2 = NULL;
	char		smtmp[4];
	int8_t		o_i8   = 0;
	u_int8_t	o_ui8  = 0;
	int16_t		o_i16  = 0;
	u_int16_t	o_ui16 = 0;
	int32_t		o_i32  = 0;
	u_int32_t	o_ui32 = 0;
	ConfigOpts	*cfo = NULL;
	struct in_addr	inp;

	*outbuf = type; outbuf++;
	len = outbuf;   outbuf++;

	switch( dtype ) {
	case OPT_IP:
		*len = 4;
		inet_aton( value, &inp );
		memcpy( outbuf, &(inp.s_addr), 4 );
		return( 2 + 4 );
		break;

	case OPT_2IP:
		strncpy( tmp, value, 300 );
		tmp2 = tmp;
		while (*tmp2 != 0 && *tmp2 != ',' && (tmp2 - tmp) < 50) tmp2++;
		if (*tmp2 == ',') { *tmp2 = 0; tmp2++; }
		*len = 4;
		inet_aton( tmp, &inp );
		memcpy( outbuf, &(inp.s_addr), 4 );
		if (*tmp2 == 0) return( 2 + 4 );

		*len = 8;
		inet_aton( tmp2, &inp );
		memcpy( outbuf + 4, &(inp.s_addr), 4 );
		return( 2 + 8 );
		break;

	case OPT_INT8:
		*len = 1;
		o_i8 = strtol( value, NULL, 10 );
		memcpy( outbuf, &o_i8, 1 );
		return( 2 + 1 );
		break;

	case OPT_UINT8:
		*len = 1;
		o_ui8 = strtoul( value, NULL, 10 );
		memcpy( outbuf, &o_ui8, 1 );
		return( 2 + 1 );
		break;

	case OPT_INT16:
		*len = 2;
		o_i16 = strtol( value, NULL, 10 ); 
		o_i16 = htons( o_i16 );
		memcpy( outbuf, &o_i16, 2 );
		return( 2 + 2 );
		break;

	case OPT_UINT16:
		*len = 2;
		o_ui16 = strtoul( value, NULL, 10 );
		o_ui16 = htons( o_ui16 );
		memcpy( outbuf, &o_ui16, 2 );
		return( 2 + 2 );
		break;

	case OPT_INT32:
		*len = 4;
		o_i32 = strtol( value, NULL, 10 );
		o_i32 = htonl( o_i32 );
		memcpy( outbuf, &o_i32, 4 );
		return( 2 + 4 );
		break;

	case OPT_UINT32:
		*len = 4;
		o_ui32 = strtoul( value, NULL, 10 );
		o_ui32 = htonl( o_ui32 );
		memcpy( outbuf, &o_ui32, 4 );
		return( 2 + 4 );
		break;

	case OPT_CHAR:
		strncpy( tmp, value, 300 );
		*len = strlen( tmp );
		memcpy( outbuf, tmp, *len );
		return( 2 + *len );
		break;

	case OPT_SUBOPT:
		o_ui16 = strtoul( value, NULL, 10 );
		cfo = (ConfigOpts *) calloc(1,sizeof(ConfigOpts) +2);
		cfo->val = (u_int8_t *) calloc(1,2048);
		o_i32 = Query_Opt( cfo, o_ui16, b_mac );
		if (o_i32 == 0) return 0;
		*len = cfo->len;
		memcpy( outbuf, cfo->val, cfo->len );
		free( cfo->val );
		free( cfo );
		return( 2 + *len );
		break;

	case OPT_HEX:
		smtmp[2] = 0;
		o_i16 = 0;
		while (*value != 0 && o_i16 < 300) {
			if (*value == ' ' || *value == ',') { value++; continue; }
			smtmp[0] = *value; value++;
			smtmp[1] = *value; value++;
			tmp[o_i16] = strtol( smtmp, NULL, 16);
			o_i16++;
		}
		*len = o_i16;
		memcpy( outbuf, tmp, *len );
		return( 2 + *len );
		break;

	case OPT_MTAIP:
		//  MTA Provision Server (example 192.168.10.2)
		//         1  9  2      1  6  8      1  0      2
		// 00  03 31 39 32  03 31 36 38  02 31 30  01 32  00
		// who invented this undocumented SHITE!!!
		// I'm going to find the committee that invented this 
		// and kick them all in the balls!!
		*outbuf = 0;  outbuf++;  *len = 1;
		for (o_i8 = 0; o_i8 < 4; o_i8++) {
			*outbuf = 0;  tmp3 = outbuf;  outbuf++;  *len +=1;
			while(*value != 0 && *value != '.' && *value >= 0x30 && *value <= 0x39) {
				*outbuf = *value; outbuf++; value++; *tmp3 +=1; }
			while (*value == '.') value++;
			*len += *tmp3;
		}
		*outbuf = 0; *len +=1;
		return( 2 + *len );
		break;

	case OPT_MTAR:
		// mta kerberos realm  (example BASIC.1)
		//     B  A  S  I  C       1
		// 05 42 41 53 49 43   01 31 00
		*outbuf = 0;  tmp3 = outbuf;  outbuf++; *len +=1;
		while(*value != 0 && *value != '.') { *outbuf = *value; outbuf++; value++; *tmp3 +=1; }
		*len += *tmp3;
		while (*value == '.') value++;
		*outbuf = 0;  tmp3 = outbuf;  outbuf++; *len +=1;
		while(*value != 0 && *value != '.') { *outbuf = *value; outbuf++; value++; *tmp3 +=1; }
		*len += *tmp3;
		*outbuf = 0; *len +=1;
		return( 2 + *len );
		break;
	default:
		return 0;
		break;
	}
}

/* ************************************************************** */

int Query_Opt( ConfigOpts *cfo, u_int16_t opt_id, u_int64_t b_mac ) {
	MYSQL_RES		*res;
	MYSQL_ROW		myrow;
	char			qbuf[200];
	u_int8_t		*buffer = NULL;
	u_int16_t		id;
	u_int8_t		type, len, dtype, subopt, buff2[300], c;
	int			i, bytes = 0;

	if (b_mac == 0) {
		snprintf( qbuf, 200, "select opt_id, opt_type, opt_dtype+0, opt_value, "
		"sub_opt from config_opts where server_id = %d and opt_id = %d order by opt_id, opt_type ",
		server_id, opt_id );
	} else {
		snprintf( qbuf, 200, "select opt_id, opt_type, opt_dtype+0, opt_value, sub_opt "
			"from config_opts_macs "
			"where opt_id = %d and conv(macaddr,16,10) = %llu "
			"order by opt_id, opt_type ", opt_id, b_mac );
	}
	SQL_QUERY_RET0( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) return 0;

	buffer = cfo->val;

	while ( (myrow = mysql_fetch_row( res )) ) {
		id     = strtoul( (myrow)[0], NULL, 10 );
		type   = strtoul( (myrow)[1], NULL, 10 );
		dtype  = strtoul( (myrow)[2], NULL, 10 );
		subopt = strtoul( (myrow)[4], NULL, 10 );

		strncpy( (char *) buff2, (myrow)[3], 300 );
		if (dtype == OPT_SUBOPT && subopt > 0) strncpy( buff2, (myrow)[4], 50 );

		bytes += Convert_Opt( buffer + bytes, type, dtype, buff2, b_mac );
	}
	mysql_free_result(res);

	if (bytes <= 0) return 0;

	cfo->b_macaddr = b_mac;
	cfo->id = opt_id;
	cfo->len = bytes;

	return( 1 );
}

/* ************************************************************** */


int  find_Opts_Mac( u_int64_t b_macaddr ) {
	int	low = 0, mid = 0;
	int	high = num_opts_mac - 1;

	if (my_opts_mac == NULL) return -1;

	while (high >= low) {
		mid = ((high - low) >> 1) + low;
		if (my_opts_mac[mid].b_macaddr == b_macaddr ) return(mid);
		if (my_opts_mac[mid].b_macaddr > b_macaddr ){ high = mid - 1; continue; }
		if (my_opts_mac[mid].b_macaddr < b_macaddr ){ low  = mid + 1; continue; }
	}
	return( -1 );
}

/* ************************************************************** */

ConfigOpts *my_Get_Opt( u_int16_t opt_id, u_int64_t b_mac ) {
	static ConfigOpts	*cfo = NULL;
	long 			i, bytes;

	if (opt_id == 0) return( NULL );

	my_Load_Opts();
	if (High_Load_Flag) {
		if (b_mac > 0) {
			i = find_Opts_Mac( b_mac );
			if (i >= 0) return &(my_opts_mac[i]);
		} else {
			for (i=0; i < num_opts; i++)
				if (my_opts[i].id == opt_id) return &(my_opts[i]);
		}
	}

	if (cfo == NULL) {
		cfo = (ConfigOpts *) calloc(1, sizeof(ConfigOpts) );
		cfo->val = (u_int8_t *) calloc(1,2048);
	}

	i = Query_Opt( cfo, opt_id, b_mac );
	if (i) { return cfo;  }
	  else { return NULL; }
}

/* ************************************************************** */
