/* mysql-tftp_filegen1.c
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

// Part 1  takes the config file pieces and compiles them into
//         binary pieces.
//         It takes the text from config_modem
//         compiles it and puts the binary equiv in config_modem_bin

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



void  my_Check_New_Configs( char *config_encoder ) {
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	char		*qbuf;

	qbuf =  "select config_modem.cfg_id, config_modem.cfg_txt, "
		"config_modem.cfg_ver from config_modem left join config_modem_bin "
		"on config_modem.cfg_id = config_modem_bin.cfg_id "
		"where config_modem.cfg_update > config_modem_bin.cfg_update "
		"or config_modem_bin.cfg_update is NULL";

	SQL_QUERY_RET( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) { return; }

	while ( (myrow = mysql_fetch_row( res )) ) {
		encode_config_morsel( config_encoder,
			(myrow)[0], (myrow)[1], (myrow)[2] );
	}
	mysql_free_result(res);
	return;
}



void  encode_config_morsel( char *config_encoder, char *id, char *text, char *ver ) {
	char		*confenc;
	FILE		*tmp_in, *tmp_out, *pcfg;
	int		stret, cfg_len;
	struct stat	stbuf;
	char		cfg_buf[1025], *error_message, *qbuf, *qbuf2;
	unsigned char	*tlv_buf;
	char		*tlv_fixed;
	int		tlv_size, tlv_siz2;

	tmp_in = fopen( "tmp.config.in", "w" );
	fprintf( tmp_in, "main {\n %s \n}\n", text );
	fclose( tmp_in );
	tmp_out = fopen( "tmp.config.out", "w" );
	fprintf( tmp_out, "-" );
	fclose( tmp_out );

	confenc = (char *) alloca( strlen(config_encoder) + 40 );
	strcpy( confenc, config_encoder );
	strcat( confenc, " -p tmp.config.in tmp.config.out" );
	pcfg = popen( confenc, "r");
	if (pcfg == NULL) {
		error_message = "can not run config";
		goto table_updater;
	}
	cfg_len = fread( cfg_buf, 1024, 1, pcfg );
	pclose( pcfg );

	stret = stat( "tmp.config.out", &stbuf );
	if (stret != 0) {
		error_message = "no output file";
		goto table_updater;
	}
	tlv_size = stbuf.st_size;
	if (tlv_size < 3) {
		error_message = cfg_buf;
		goto table_updater;
	}
	if (tlv_size > 99999) {
		error_message = "config file too big";
		goto table_updater;
	}

	tmp_out = fopen( "tmp.config.out", "r" );
	tlv_buf = (char *) alloca( tlv_size + 1 );
	fread( tlv_buf, tlv_size, 1, tmp_out );
	fclose( tmp_out );

	tlv_fixed = (char *) alloca( tlv_size * 2 + 1 );
	tlv_siz2 = mysql_real_escape_string( my_sock, tlv_fixed, (char *) tlv_buf, tlv_size );
	qbuf = (char *) alloca( tlv_siz2 + 100 );

	snprintf( qbuf, tlv_siz2 + 100,
		"replace into config_modem_bin (cfg_id,cfg_bin,cfg_ver) values ('%s','%s','%s')",
		id, tlv_fixed, ver );
	SQL_QUERY_RET( qbuf );

	error_message = "OK";

table_updater:
	qbuf2 = (char *) alloca( 2000 );
	snprintf( qbuf2, 2000,
		"update config_modem set cfg_errors = '%s' where cfg_id = '%s'",
		error_message, id );
	SQL_QUERY_RET( qbuf2 );
}


