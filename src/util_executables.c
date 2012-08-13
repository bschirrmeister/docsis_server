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


int CheckExecutable ( char *filename ) {
	struct stat		fst;
	int			status;
	uid_t			uid;
	gid_t			gid;

	if (!strcmp(filename," ")) return 0;

	status = stat( filename, &fst );
	if (status != 0) {
		my_syslog( MLOG_ERROR, "ERROR: Executable not found (%s)", filename );
		return 0;
	}

	uid = geteuid();
	gid = getegid();

	if ( uid == fst.st_uid && (fst.st_mode & S_IWUSR) )
		return 1;

	if ( gid == fst.st_gid && (fst.st_mode & S_IWGRP) )
		return 1;

	if (S_IWOTH & fst.st_mode)
		return 1;

	my_syslog( MLOG_ERROR, "ERROR: File not Executable (%s)", filename );
	return 0;
}


void RunExit ( char *pgm, char *param1, char *param2 ) {
	pid_t		pid = 0;
	char		*argv[4];

	if (CheckExecutable(pgm)) {
		pid = fork();
		if (pid == 0) {
			argv[0] = pgm;
			argv[1] = param1;
			argv[2] = param2;
			argv[3] = NULL;
			execvp( pgm, argv );
			exit(0);
		} else if (pid > 0)
			waitpid( pid, NULL, WNOHANG );
	}
}

