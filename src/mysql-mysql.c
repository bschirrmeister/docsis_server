/* mysql-mysql.c
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


/* Global Variables */
MYSQL		*my_sock;
MYSQL		my_pri_sql,  *my_pri_sock = NULL;
MYSQL		my_back_sql, *my_back_sock = NULL;
MYSQL		my_pdns_sql, *my_pdns_sock = NULL;
u_int8_t	server_id;
char		*process_type = NULL;
u_int64_t	Super_Flag = 0;


void  Init_Primary_SQL( void ) {
	int	count;
	char	*dt, *tim;
	unsigned int	timeout_secs = 86400;

	mysql_init( &my_pri_sql );
	mysql_options( &my_pri_sql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout_secs );

	get_date_time(&dt,&tim);

	for( count=0; count<10; count++ ) {
		my_pri_sock = mysql_real_connect( &my_pri_sql,
			GetConfigVar( "mysql-host" ), GetConfigVar( "mysql-user" ),
			GetConfigVar( "mysql-password" ),
			GetConfigVar( "mysql-dhcp-database" ), 0, NULL, 0);
		if (my_pri_sock) { my_sock = my_pri_sock; return; }
		fprintf( stderr, "Can not connect to primary database server "
			"(attempt = %d)\nDB '%s', '%s', '%s', '%s'\n",
			(count + 1), GetConfigVar( "mysql-host" ), GetConfigVar( "mysql-user" ),
			GetConfigVar( "mysql-password" ), GetConfigVar( "mysql-dhcp-database" ) );
		if (count < 9) sleep( 30 );
	}

	fprintf( stderr, "Can not connect to database server\n\n" );

	fprintf( stderr, "Config File: Host = '%s' \n", GetConfigVar( "mysql-host" ) );
	fprintf( stderr, "Config File: User = '%s' \n", GetConfigVar( "mysql-user" ) );
	fprintf( stderr, "Config File: Password = '%s' \n", GetConfigVar( "mysql-password" ) );
	fprintf( stderr, "Config File: Dhcp-Database = '%s' \n", GetConfigVar( "mysql-dhcp-database" ) );
	fprintf( stderr, "\n\n" );
}

void  Init_Backup_SQL( void ) {
	int	count;
	char	*dt, *tim, *backup;
	unsigned int	timeout_secs = 86400;

	backup = GetConfigVar( "mysql-backup" );
	if (*backup != 'y') { return; }

	mysql_init( &my_back_sql );
	mysql_options( &my_back_sql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout_secs );

	get_date_time(&dt,&tim);

	for( count=0; count<10; count++ ) {
		my_back_sock = mysql_real_connect( &my_back_sql,
			GetConfigVar( "mysql-backup-host" ),
			GetConfigVar( "mysql-backup-user" ),
			GetConfigVar( "mysql-backup-password" ),
			GetConfigVar( "mysql-backup-dhcp-database" ), 0, NULL, 0);
		if (my_back_sock) { return; }
		fprintf( stderr, "Can not connect to backup database server (attempt = %d)\n\n",
			(count + 1) );
		if (count < 9) sleep( 30 );
	}

	fprintf( stderr, "Can not connect to database server\n\n" );

	fprintf( stderr, "Config File: Host = '%s' \n", GetConfigVar( "mysql-backup-host" ) );
	fprintf( stderr, "Config File: User = '%s' \n", GetConfigVar( "mysql-backup-user" ) );
	fprintf( stderr, "Config File: Password = '%s' \n", GetConfigVar( "mysql-backup-password" ) );
	fprintf( stderr, "Config File: Dhcp-Database = '%s' \n",
		GetConfigVar( "mysql-backup-dhcp-database" ) );
	fprintf( stderr, "\n\n" );
}

void  Init_PowerDNS_SQL( void ) {
	int	count;
	char	*dt, *tim, *pdns;
	unsigned int	timeout_secs = 86400;

	pdns = GetConfigVar( "pdns-mysql" );
	if (*pdns != 'y') { return; }

	mysql_init( &my_pdns_sql );
	mysql_options( &my_pdns_sql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout_secs );

	get_date_time(&dt,&tim);

	for( count=0; count<10; count++ ) {
		my_pdns_sock = mysql_real_connect( &my_pdns_sql,
			GetConfigVar( "pdns-mysql-host" ),
			GetConfigVar( "pdns-mysql-user" ),
			GetConfigVar( "pdns-mysql-password" ),
			GetConfigVar( "pdns-mysql-database" ), 0, NULL, 0);
		if (my_pdns_sock) { return; }
		fprintf( stderr, "Can not connect to pdns database server (attempt = %d)\n\n",
			(count + 1) );
		if (count < 9) sleep( 30 );
	}

	fprintf( stderr, "Can not connect to database server\n\n" );

	fprintf( stderr, "Config File: Host = '%s' \n", GetConfigVar( "pdns-mysql-host" ) );
	fprintf( stderr, "Config File: User = '%s' \n", GetConfigVar( "pdns-mysql-user" ) );
	fprintf( stderr, "Config File: Password = '%s' \n", GetConfigVar( "pdns-mysql-password" ) );
	fprintf( stderr, "Config File: Dhcp-Database = '%s' \n",
		GetConfigVar( "pdns-mysql-database" ) );
	fprintf( stderr, "\n\n" );
}

void  InitSQLconnection() {
	char	*servid;

	server_id = strtoul( GetConfigVar( "server-id" ), NULL, 10 );
	if (server_id == 0) {
		fprintf(stderr,"!!! You need to set the Server-ID\n");
		fprintf(stderr,"I'm DYING!!!\n" );
		exit(0);
	}
	Init_Primary_SQL();
	Init_Backup_SQL();
	Init_PowerDNS_SQL();
}

void	CloseSQLconnection() {
	mysql_close( my_sock );
	mysql_close( my_back_sock );
	mysql_close( my_pdns_sock );
}

int    my_SQL_Ping( void ) {
	int	i;

	if (Primary_SQL_Failure_Flag != 0) {
		i = mysql_ping( my_pri_sock );
		if (i == 0) {
			CLR_Primary_SQL_Failure_Flag;
			fprintf(stderr,"primary SQL server lives again!!\n" );
			my_sock = my_pri_sock;
		}
	}

	if (my_sock == NULL) {
		fprintf(stderr,"my_sock is NULL.  time to die!!\n" );
		exit(1);
	}

	i = mysql_ping( my_sock );
	if (i == 0) {
		return 0;
	} else { fprintf(stderr,"mysql_ping = %d  FF %d\n'%s'\n",
		i, Primary_SQL_Failure_Flag, mysql_error( my_sock ) );
	}

	i = mysql_ping( my_sock );
	if (i == 0) {
		return 0;
	} else { fprintf(stderr,"mysql_ping = %d  PSQFF %d\n'%s'\n",
		i, Primary_SQL_Failure_Flag, mysql_error( my_sock ) );
	}

	if (my_back_sock == NULL) {
		fprintf(stderr, "no backup configured. I hate myself.  AHHHHHHHHHHHHHHH!!\n" );
		exit(1);
	}

	if (Primary_SQL_Failure_Flag == 0) {
		SET_Primary_SQL_Failure_Flag;
		fprintf(stderr, "primary SQL server is DEAD!!\n" );
		fprintf( stderr, "SQL Error '%s'\n", mysql_error( my_sock ) );
		if (my_back_sock == NULL) {
			fprintf(stderr, "no backup configured!!\n" );
			return 1;
		}
		my_sock = my_back_sock;
		i = mysql_ping( my_sock );
		if (i == 0) {
			fprintf(stderr,"backup SQL server is OK!!\n" );
			return 0;
		}
		return 1; // bad news 
	}
}


/* ****************************************************************** */

void my_Check_Load( int high_load ) {
	static time_t		old_time = 0;
	static int		timecc = 0;
	time_t			new_time = 0;
	time_t			diff;
	char			*qbuf;

	new_time = time(NULL);
	diff = new_time - old_time;
	if (diff == 0) {
		timecc++;
		fprintf(stderr, "load +1 %d  high(%d)\n", timecc, high_load );
	} else {
		// we count how many requests we get each second 
		// at the end of each second we divide the prev requests
		// in half. The limit is 128 which means that if we hit
		// 128 requests in a second it will continue to use the
		// cache for 4 more seconds. (with dhcp-high-load = 15)
		if (timecc > 512) { timecc = 512; }
		timecc = timecc >> 1;
		if (timecc > diff) timecc -= diff;
			else timecc = 0;
		old_time = new_time;
		fprintf(stderr, "load /2 %d  high(%d) diff %d\n", timecc, high_load, diff );
	}
	if (High_Load_Flag) {
		if (timecc < high_load)
			CLR_High_Load_Flag;
		return;
	}
	
	if (timecc >= high_load)
		SET_High_Load_Flag;
}

/* ****************************************************************** */
/* Various SQL routines */


/* ****************************************************************** */

void  my_LoadChecker( void ) {
	static int	fd = 0;
	char		buffer[1024], *loadchr, *p;
	float		avg;
	int		count;
	time_t		new_time;
	static time_t	old_time = 0;
	static float	my_LoadAvg = 0.0;
	static float	my_MaxLoad = 0.0;

	if (fd == 0) {
		fd = open ("/proc/loadavg", O_RDONLY);
	}
	if (fd == -1) { my_LoadAvg = 0; return; }

	new_time = time( NULL );
	if ( (new_time - old_time) > 15 ) {	// Check every 15 seconds

		loadchr = GetConfigVar( "suspend-load-avg" );
		if (*loadchr == ' ') { loadchr = "1.0"; }
		my_MaxLoad = atof( loadchr );

		memset( buffer, 0, 1024 );
		lseek( fd, 0, SEEK_SET );
		count = read (fd, buffer, sizeof(buffer));
		if (count <= 0) { my_LoadAvg = 0; return; }
		p = buffer;  while( *p != 0 && *p != ' ') { p++; }
		*p = 0;
		avg = atof( buffer );
		if (avg > my_MaxLoad && my_LoadAvg < my_MaxLoad) {
			my_syslog( MLOG_ERROR, "Load Average is too high. Suspending Logging. %3.3f", avg );
		}
		if (avg < my_MaxLoad && my_LoadAvg > my_MaxLoad) {
			my_syslog( MLOG_ERROR, "Load Average is OK. %3.3f", avg );
		}
		my_LoadAvg = avg;
		old_time = new_time;
	}
}

/* ****************************************************************** */


/* ****************************************************************** */
