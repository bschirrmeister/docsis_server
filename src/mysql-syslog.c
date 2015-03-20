/* mysql-logging.c
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
extern u_int8_t		server_id;
extern char		*process_type;
extern u_int64_t	Super_Flag;
/* ******************************************************************** */


void	my_syslog( const int priority, const char * fmt, ... ) {
	va_list		list;
	static char	mbuf[1024 + 1];
	char		*p = NULL, *dt = NULL, *tim = NULL;
	char		qbuf[500];
	static char	*buf_pref = NULL;

	memset( mbuf, 0, 1024 );
	memset( qbuf, 0, 499 );
	//memset( qval, 0, 499 );

	va_start (list, fmt);
	vsnprintf (mbuf, sizeof mbuf, fmt, list);
	va_end (list);

	for( p = mbuf; *p != '\0'; p++) {
		if (*p == 34) { *p = '`'; }
		if (*p == 39) { *p = '`'; }
		if (*p == 92) { *p = '/'; }
	}
	*(mbuf + 200) = 0;

	get_date_time( &dt, &tim );

	fprintf(stderr,"LOG %s\n", mbuf );

	snprintf( qbuf, 500, "insert delayed into sys_log "
		"( thedate, thetime, serverid, priority, message ) values "
		"( '%s', '%s', %d, %d, '%s' ) ",
		dt, tim, server_id, priority, mbuf );

	if (buf_pref == NULL) {
		buf_pref = GetConfigVar( "suspend-system-logging" );
		if (*buf_pref == ' ') { buf_pref = "yes"; }
	}

	Save_SQL_Update( buf_pref, qbuf, CHECK_LOAD_NO );
}

/* ****************************************************************** */

void  my_dhcplog( int sending, dhcp_message *message ) {
	char	qbuf[900];
	char	*s_send, *s_type, *s_dynam, *s_mcache, *dt, *tim;
	static char	*buf_pref = NULL;
	u_int32_t	ipaddr;

	ipaddr = message->b_ipaddr;

	s_send = "ACK";  /* sending = DHCP_ACK */
	if (sending == DHCP_OFFER) { s_send = "OFFER"; } else
	if (sending == DHCP_NAK)   { s_send = "NAK";   } else
	if (sending == DHCP_Q_ACK) { s_send = "Q_ACK"; } else
	if (sending == DHCP_Q_NAK) { s_send = "Q_NAK"; }

	if (*s_send == 'Q' && ipaddr == 0) { ipaddr = ntohl( message->in_pack.ciaddr ); }

	s_type = "CM";   /* lease_type = LEASE_CM */
	if (message->lease_type == LEASE_CPE ) { s_type = "CPE"; }
	if (message->lease_type == LEASE_MTA ) { s_type = "MTA"; }
	if (message->lease_type == LEASE_NOAUTH ) { s_type = "NO-AUTH"; }

	s_dynam = "NO";
	if (message->cpe_type == CPE_DYNAMIC ) { s_dynam = "YES"; }

	s_mcache = "NO";   /* mcache = 0 */
	if (message->mcache ) { s_mcache = "YES"; }

	get_date_time( &dt, &tim );

	snprintf( qbuf, 900, "insert delayed into dhcp_log "
		"( thedate, thetime, serverid, sending, type, dynamic_flag, mcache_flag, " 
		"b_macaddr, b_ipaddr, b_modem_macaddr, subnum ) "
		" values ( '%s', '%s', %d, '%s', '%s', '%s', '%s', "
		"'%llu', '%lu', '%llu', '%llu' )",
		dt, tim, server_id, s_send, s_type, s_dynam, s_mcache,
		message->b_macaddr, (long unsigned) ipaddr,
		message->b_modem_macaddr, message->subnum );

	if (buf_pref == NULL) {
		buf_pref = GetConfigVar( "suspend-dhcp-logging" );
		if (*buf_pref == ' ') { buf_pref = "yes"; }
	}

	Save_SQL_Update( buf_pref, qbuf, CHECK_LOAD_YES );
}

typedef struct aos {
	char	buff[150];
	int	len;
} A_O_S;

void my_LogSyslog( u_int32_t ipaddr, char * message ) {
	time_t		t1;
	struct tm	*t2;
	char		dbname[50], qbuf[1502], time_str[100], mess[1001];
	char		*p, *q;
	int		priority = 0, severity = 0, i, j, wc;
	char		ip_addr[20], timestamp[32], qtag[32], qmessage[260];
	struct in_addr	inp;
	static int	day_save = 0;
	static char	*buf_pref = NULL;
	static char	*db_pref = NULL;
	static A_O_S	*aa = NULL;

	inp.s_addr = ipaddr;
	strncpy( ip_addr, inet_ntoa( inp ), 18 );

	if (db_pref == NULL) {
		db_pref = GetConfigVar( "mysql-syslog-database" );
		if (*db_pref == ' ') { db_pref = "syslog_server"; }
	}
	if (aa == NULL) { aa = (A_O_S *) calloc(200, sizeof(A_O_S) ); }
	memset( aa, 0, 200 * sizeof(A_O_S) );

	t1 = time(NULL);
	t2 = localtime( &t1 );
	sprintf(dbname,"%s.log_%d_%d_%d", db_pref, (1900 + t2->tm_year), (1 + t2->tm_mon), t2->tm_mday );
	sprintf(time_str,"%d:%d:%d", t2->tm_hour, t2->tm_min, t2->tm_sec );

	if (day_save != t2->tm_mday) {
		snprintf( qbuf, 1500, "CREATE TABLE IF NOT EXISTS %s " \
			"( thetime time not null, ipaddr varchar(16) not null," \
			"priority tinyint unsigned not null, " \
			"severity tinyint unsigned not null, " \
			"timestamp varchar(30) not null, tag varchar(30) not null, " \
			"message varchar(255) not null, " \
			"index(thetime),index(ipaddr),index(priority,severity)," \
			"index(severity) ) ENGINE=MyISAM",
			dbname );
		SQL_QUERY_RET( qbuf );
		day_save = t2->tm_mday;
	}

	strncpy( mess, message, 1000 );
	for (p=mess; *p; p++) {
		if (*p == 34) { *p = '`'; }
		if (*p == 39) { *p = '`'; }
		if (*p == 92) { *p = '/'; }
		if (*p > 0 && *p < 32) { *p = ' '; }
	}

	p = mess; while(*p == ' ') p++;
	q = p;  wc = 0;
	while (*p != 0) {
		p++;
		if (*p == ' ' || *p == ':' || *p == '>' || *p == '[' || *p == ']' || *p == 0) {
			if (*p == '>' || *p == ':') p++;
			memcpy( aa[wc].buff, q, (p - q) );
			aa[wc].len = (p - q);
			while (*p == ' ' || *p == ':' || *p == '>' || *p == '[' || *p == ']') p++;
			q = p; wc++;
		}
	}

	if (aa[0].buff[0] == '<') {
		int tmpval = strtoul( aa[0].buff + 1, NULL, 10 );
		priority = tmpval / 8;
		severity = tmpval % 8;
	}

	if (strcasecmp(aa[1].buff, "BSR") == 0) {
		char	tmps[1024];
		snprintf( timestamp, 31, "%s%s%s",
			(aa[3].buff + 1), aa[4].buff, aa[5].buff );
		snprintf( qtag, 31, "%s%s", aa[6].buff, aa[7].buff );
		strcpy( tmps, aa[8].buff ); strcat( tmps, " " );
		for (i=9; i < wc; i++) {
			strcat( tmps, aa[i].buff );
			strcat( tmps, " " );
		}
		snprintf( qmessage, 256, "%s", tmps );
	} else if (strncmp(aa[2].buff,"SLOT",4) == 0 && strncmp(aa[9].buff,"%UBR",4) == 0) {
		char	tmps[1024];
		snprintf( timestamp, 31, "%s %s %s %s%s%s",
			aa[1].buff, aa[4].buff, aa[5].buff, aa[6].buff, aa[7].buff, aa[8].buff );
		snprintf( qtag, 31, "%s", aa[9].buff );
		strcpy( tmps, aa[2].buff ); strcat( tmps, aa[3].buff ); strcat( tmps, " " );
		for (i=10; i < wc; i++) {
			strcat( tmps, aa[i].buff );
			strcat( tmps, " " );
		}
		snprintf( qmessage, 256, "%s", tmps );
	} else if (strncmp(aa[7].buff, "%", 1) == 0) {
		char	tmps[1024];
		snprintf( timestamp, 31, "%s %s %s %s%s%s",
			aa[1].buff, aa[2].buff, aa[3].buff, aa[4].buff, aa[5].buff, aa[6].buff );
		snprintf( qtag, 31, "%s", aa[7].buff );
		strcpy( tmps, aa[8].buff ); strcat( tmps, " " );
		for (i=9; i < wc; i++) {
			strcat( tmps, aa[i].buff );
			strcat( tmps, " " );
		}
		snprintf( qmessage, 256, "%s", tmps );
	} else if (strncasecmp(aa[1].buff, "CABLEMODEM", 10) == 0) {
		char	tmps[1024];
		i = 1;
		strncpy( timestamp, aa[i].buff, 31 ); i++;
		for (j=0; j < 4; j++) {
			if (strncmp(aa[i].buff,"<",1) != 0) {
				strcat(timestamp," ");
				strcat(timestamp,aa[i].buff);
				i++;
			}
		}
		strncpy( qtag, aa[i].buff, 31 ); i++;
		strcpy( tmps, aa[i].buff ); strcat( tmps, " " ); i++;
		for (j=i; j < wc; j++) {
			strcat( tmps, aa[j].buff );
			strcat( tmps, " " );
		}
		snprintf( qmessage, 256, "%s", tmps );
	} else {
		char	tmps[1024];
		strncpy( timestamp, aa[1].buff, 31 );
		strcpy( qtag, " " );
		strcpy( tmps, aa[2].buff ); strcat( tmps, " " );
		for (i=3; i < wc; i++) {
			strcat( tmps, aa[i].buff );
			strcat( tmps, " " );
		}
		snprintf( qmessage, 256, "%s", tmps );
	}

	// thetime, ipaddr, priority, severity, timestamp, tag, message
	snprintf( qbuf,1500, "insert into %s values ( '%s', '%s',%d,%d,'%s','%s','%s' )",
		dbname, time_str, ip_addr, priority, severity, timestamp, qtag, qmessage );

	fprintf(stderr, "syslog MESS '%s'\n", message );
	fprintf(stderr, "%s\n", qbuf );

	if (buf_pref == NULL) {
		buf_pref = GetConfigVar( "suspend-syslog-logging" );
		if (*buf_pref == ' ') { buf_pref = "yes"; }
	}
	Save_SQL_Update( buf_pref, qbuf, CHECK_LOAD_YES );
}


