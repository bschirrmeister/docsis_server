/***************************************************************************
 *   Copyright (C) 2006 by Docsis Guy                                      *
 *   docsis_guy@accesscomm.ca                                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "docsis_server.h"

// external Globals
extern char	*process_type;


void my_exit( void );

void dhcp_server( void );
void tftp_server( void );
void time_server( void );
void syslog_server( void );
void cf_generator( void );
void backgrounder( void );
void update_log_file( void );


int main( int argc, char *argv[] ) {
	pid_t		pid[6];
	int		i, rename = 0;
	char		*dt, *tim;
	struct rlimit	rlim;

	setsid();	/* Set Process Independency */
	umask(027);	/* Set File Creation Mask */
	chdir( CHROOT_PATH );	/* Change to /tmp dir */

	rlim.rlim_cur = RLIM_INFINITY;
	rlim.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &rlim );

	/* Lets Fork Really Early. */
	pid[0] = fork();
	if (pid[0] != 0) { exit(0); }
	pid[0] = fork();
	if (pid[0] != 0) { exit(0); }

	for (i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */
	i=open( CHROOT_PATH "/" LOGFILE, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP );
	dup(i);
	dup(i);

	signal( SIGINT,  (sig_t) my_exit );
	signal( SIGQUIT, (sig_t) my_exit );
	signal( SIGTERM, (sig_t) my_exit );

	if (argc > 1) {
		if (strlen(argv[1]) > 10) {
			rename = strlen(argv[1]);
			memset( argv[1], 0, rename );
		}
	}
	for (i=0;i<6;i++) pid[i] = 0;

	ReadConfigFile( 1 );

	get_date_time( &dt, &tim );
	fprintf(stderr, "Starting Version %s  ", VERSION );
	fprintf(stderr, "Date: %s  Time: %s \n", dt, tim );

	if (my_CheckActiveService("dhcp")) {
		fprintf(stderr, "Now Forking DHCP Server." );
		pid[0] = fork();
		if (pid[0] == 0) {
			process_type = "DHCP";
			update_log_file();
			update_pid_file();
			if (rename) { strcpy( argv[0], "docsis_server (DHCP)     " ); }
			dhcp_server();
			exit(0);
		} else {
			fprintf(stderr, " PID = %d \n", pid[0] );
		}
	}

	if (my_CheckActiveService("tftp")) {
		fprintf(stderr, "Now Forking TFTP Server." );
		pid[1] = fork();
		if (pid[1] == 0) {
			process_type = "TFTP";
			update_log_file();
			update_pid_file();
			if (rename) { strcpy( argv[0], "docsis_server (TFTP)    " ); }
			tftp_server();
			exit(0);
		} else {
			fprintf(stderr, " PID = %d \n", pid[1] );
		}
	}

	if (my_CheckActiveService("time")) {
		fprintf(stderr, "Now Forking TIME Server." );
		pid[2] = fork();
		if (pid[2] == 0) {
			process_type = "TIME";
			update_log_file();
			update_pid_file();
			if (rename) { strcpy( argv[0], "docsis_server (TIME)    " ); }
			time_server();
			exit(0);
		} else {
			fprintf(stderr, " PID = %d \n", pid[2] );
		}
	}

	if (my_CheckActiveService("syslog")) {
		fprintf(stderr, "Now Forking SYSLOG Server." );
		pid[3] = fork();
		if (pid[3] == 0) {
			process_type = "SYSLOG";
			update_log_file();
			update_pid_file();
			if (rename) { strcpy( argv[0], "docsis_server (SYSLOG)   " ); }
			syslog_server();
			exit(0);
		} else {
			fprintf(stderr, " PID = %d \n", pid[3] );
		}
	}

	if (my_CheckActiveService("backg")) {
		fprintf(stderr, "Now Forking BACKG Server." );
		pid[4] = fork();
		if (pid[4] == 0) {
			process_type = "BACKG";
			update_log_file();
			update_pid_file();
			if (rename) { strcpy( argv[0], "docsis_server (BACKG)   " ); }
			backgrounder();
			exit(0);
		} else {
			fprintf(stderr, " PID = %d \n", pid[4] );
		}
	}

	return EXIT_SUCCESS;
}

void update_log_file( void ) {
	int 	i;
	char	*fname;
	pid_t	my_pid = 0;

	my_pid = getpid();
	i = strlen( CHROOT_PATH "/" LOGFILE "." );
	i += strlen( process_type ) + 10;

	fname = (char *) alloca( i + 1 );
	snprintf( fname, i, "%s.%s.%d", CHROOT_PATH "/" LOGFILE, process_type, my_pid );

	for (i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */
	i=open( fname , O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP );
	dup(i);
	dup(i);
}

void update_pid_file( void ) {
	static time_t   old_time = 0;
	time_t		new_time;
	static pid_t	my_pid = 0;
	static char	*pid_fname = NULL;
	static int	pid_fd = 0;

	if (pid_fd == -1) { return; }

	new_time = time(NULL);
	if (old_time > new_time) { return; }
	old_time = new_time + 60;

	if (my_pid == 0) { my_pid = getpid(); }

	if (pid_fname == NULL) {
		pid_fname = (char *) calloc(1, 200 );
		snprintf( pid_fname, 200, "%s/run/docsis_server.%s.pid", VARDIR, process_type );
	}

	if (pid_fd == 0) {
		pid_fd = open( pid_fname, O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH );
		if (pid_fd == -1) {
			fprintf(stderr, "Cannot open PID file (%s)\n", pid_fname );
			return;
		}
	}

	dprintf( pid_fd, "%d\n", my_pid );
	fsync( pid_fd );
	lseek( pid_fd, 0, SEEK_SET );
}

void my_exit( void ) {
	if (strcmp(process_type,"TFTP") == 0) {
		logstats();
	}

	Flush_ALL_SQL_Updates();
	exit( EXIT_SUCCESS );
}


