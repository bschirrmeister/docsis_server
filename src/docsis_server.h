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

#ifndef __DOCSIS_SERVER_H__
#define __DOCSIS_SERVER_H__ 1

#include <sys/param.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>		/* must come after sys/time.h */

#include <netinet/in.h>
#include <net/if.h>
#include <arpa/tftp.h>
#include <arpa/inet.h>

#include <pwd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <stdarg.h>


#ifndef CONFDIR
# define CONFDIR "/etc"
#endif
#ifndef VARDIR
# define VARDIR "/var"
#endif
#ifndef TMPDIR
# define TMPDIR "/tmp"
#endif
#ifndef CHROOT_PATH
#define CHROOT_PATH  "/home/docsis"
#endif
#ifndef LOGFILE
#define LOGFILE "docsis_server"
#endif


#include <mysql/mysql.h>
#include <mysql/mysql_com.h>
#include <mysql/mysql_version.h>

#define SERVPROC_DHCP	1
#define SERVPROC_TFTP	2
#define SERVPROC_TIME	3
#define SERVPROC_SYSLOG	4
#define SERVPROC_BACKG	5


#include "Version.h"
#include "util_configfile.h"
#include "util_executables.h"
#include "util_macaddr.h"
#include "util_md5.h"
#include "util_time.h"
#include "dhcpd-dhcpd.h"
#include "dhcpd-options.h"
#include "dhcpd-socket.h"
#include "mysql-mysql.h"
#include "tftpd-tftpd.h"
#include "tftpd-file_cache.h"

// include <dmalloc.h>

void update_pid_file (void);

#endif

