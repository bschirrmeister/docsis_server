/* mysql-utilities.c
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


/* ******************************************************************** */
SQL_buf		*buffer = NULL;
SQL_buf		*end_buf = NULL;
int		buf_cnt = 0;
long		buf_bytes = 0;
/* ******************************************************************** */

void my_buffer_save( char *qbuf ) {
	SQL_buf		*tb;

	if (buf_bytes > (30 * 1024 * 1024 )) {
		fprintf(stderr, "sql buffer greater than 30 MB (%ld bytes)", buf_bytes );
		return;
	}

	if (buffer == NULL) {
		buffer = (SQL_buf *) calloc(1, sizeof( SQL_buf ) );
		if (buffer == NULL) {
			fprintf( stderr, "calloc failed: %s\n", strerror(errno));
			return;
		}
		buffer->len = strlen( qbuf ) +1;
		buffer->qbuf = (char *) calloc(1, buffer->len );
		if ( buffer->qbuf == NULL ) {
			fprintf( stderr, "calloc failed: %s\n", strerror(errno));
			return;
		}
		buffer->next = NULL;
		strcpy( buffer->qbuf, qbuf );
		end_buf = buffer;
		buf_cnt = 1;
		buf_bytes = buffer->len;
	} else {
		tb = end_buf;
		tb->next = (SQL_buf *) calloc(1, sizeof( SQL_buf ) );
		if (tb->next == NULL) {
			fprintf( stderr, "calloc failed: %s\n", strerror(errno));
			return;
		}
		tb->next->len = strlen( qbuf ) +1;
		tb->next->qbuf = (char *) calloc(1, tb->next->len );
		if (tb->next->qbuf == NULL) {
			fprintf( stderr, "calloc failed: %s\n", strerror(errno));
			return;
		}
		tb->next->next = NULL;
		strcpy( tb->next->qbuf, qbuf );
		end_buf = tb->next;
		buf_cnt++;
		buf_bytes += tb->next->len;
	}
}

/* ******************************************************************** */

void  Flush_ALL_SQL_Updates( void ) {
	SQL_buf		*tb;

	fprintf(stderr,"sql buffer was %d deep (%ld bytes)\n", buf_cnt, buf_bytes );

	while (buffer != NULL) {
		tb = buffer;
		buffer = buffer->next;
		mysql_query( my_sock, tb->qbuf );
		free( tb->qbuf );
		free( tb );
		tb = NULL;
		buf_cnt--;
	}
	buffer = NULL;
	end_buf = NULL;
	buf_cnt = 0;
	buf_bytes = 0;
}

/* ******************************************************************** */
void Flush_SQL_Updates( void ) {
	SQL_buf		*tb;
	int		flushcc = 0;
	int		max_flush = buf_cnt / 4;

	if (max_flush < 500) max_flush = 500;
	if (max_flush > 1000) max_flush = 1000;

	while (buffer != NULL) {
		tb = buffer;
		buffer = buffer->next;
		if ( mysql_query( my_sock, tb->qbuf ) )
			fprintf( stderr, "sql error\nq=%s \ne=%s\n", tb->qbuf, mysql_error( my_sock ) );
		buf_bytes -= tb->len;
		free( tb->qbuf );
		free( tb );
		tb = NULL;
		buf_cnt--;

		flushcc++;
		if (flushcc > max_flush) {
			fprintf(stderr,"sql buffer %d deep (%ld bytes)\n", buf_cnt, buf_bytes);
			return;
		}
	}
	buffer = NULL;		// paranoia sets in...
	end_buf = NULL;
	buf_cnt = 0;
	buf_bytes = 0;
}

/* ******************************************************************** */
void Save_SQL_Update( char *pref, char *qbuf, int checkLoad ) {

	if (High_Load_Flag) {
		my_buffer_save( qbuf );
		return;
	}
	

	if ( mysql_ping( my_sock ) ) {
		fprintf( stderr, " no sql server!\n" );
		/* Hmmmmm. No SQL server. We should Buffer the SQL statements */
		my_buffer_save( qbuf );
		return;
	}

	if (buffer != NULL) {
		my_buffer_save( qbuf );
		Flush_SQL_Updates();
		return;
	}

	/* If Load is not high store any queued sql updates */
	if ( mysql_query( my_sock, qbuf ) )
		fprintf( stderr, "sql error\nq=%s \ne=%s\n", qbuf, mysql_error( my_sock ) );
}

