/*
 * Copyright (c) 1994, 1995, 1997
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1994, 1995, 1997\n\
The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static const char sccsid[] = "@(#)tftpd.c	8.1 (Berkeley) 6/4/93";
#endif /* not lint */

/*
 * Trivial file transfer protocol server.
 *
 * This version includes many modifications by Jim Guyton
 * <guyton@rand-unix>.
 *
 * Rewritten to handle multiple concurrent clients in a single process
 * by Craig Leres, leres@ee.lbl.gov.
 *
 * Rewritten again to be part of docsis_server.
 * by MM.
 *
 * netascii op was removed.
 * a file cache was implimented. be carefull with big files and your memory
 * if you use this server to transmit files bigger than your RAM you're in trouble
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "docsis_server.h"

/* ******************************************************************** */
extern MYSQL		*my_sock;
extern char		*process_type;
extern int 		high_load_flag;
/* ******************************************************************** */


/* Globals */
int	tftpd_exit_flag = 1;
char	*errstr;			// detail for DENIED syslog
int	debug = 0;

int	tftp_socket;			// tftp socket
time_t	now;				// updated after packet reception
struct stats  stats;			// tftpd statistics
struct timeval   x_tv;
struct timezone  x_tz;

Client		*clientlist = NULL;
u_int32_t	clientlistcnt = 0;		// number in use


void  Init_TFTP_Socket( void ) {
	static struct sockaddr_in	sin;

	/* Setup our socket */
	tftp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (tftp_socket < 0) {
		my_syslog( MLOG_ERROR, "tftp socket: %s", strerror(errno) );
		exit(1);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons( 69 );
	if (bind(tftp_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		my_syslog( MLOG_ERROR, "tftp bind: %s", strerror(errno) );
		exit(1);
	}
}


void tftp_server( void ) {
	int		cc;
	struct timeval	*tvp;
	time_t		last, hour6, min1;
	socklen_t	fromlen;
	struct sockaddr_in	from;
	struct timeval	tv;
	struct stat	tftp_stat;
	struct passwd	*pass_struct;
	char		*euid_pref, *tftp_dir, *dbhost_pref, *force_chroot;
	char		*tftp_high_load_c;
	int		tftp_high_load;
	fd_set		rfds;
	char		buf[SEGSIZE + 4];

	tftp_dir = my_GetTFTPdir();
	if ( stat( tftp_dir, &tftp_stat ) != 0 ) {
		fprintf(stderr,"Could not read tftp dir: %s", tftp_dir );
		exit(1);
	}
	chdir( tftp_dir );

	Init_TFTP_Socket();

	dbhost_pref = GetConfigVar( "mysql-host" );
	euid_pref = GetConfigVar( "effective-userid" );
	force_chroot = GetConfigVar( "force-localhost-chroot" );
	pass_struct = getpwnam( euid_pref );

	//if (strcmp(dbhost_pref,"localhost") || !strcmp(force_chroot, "yes")) {
		// We cannot chroot if connect to the DB via
		// a unix socket vs tcp
	//	chroot( tftp_dir );
	//}
	//if (pass_struct) {
	//	seteuid( pass_struct->pw_uid );
	//	setegid( pass_struct->pw_gid );
	//}

	tftp_high_load_c = GetConfigVar( "tftp-high-load" );
	if (*tftp_high_load_c == ' ') { tftp_high_load_c = "16"; }
	tftp_high_load = strtoul( tftp_high_load_c, NULL, 10 );
	if (tftp_high_load <= 4) { tftp_high_load = 4; }
	if (tftp_high_load >= 512) { tftp_high_load = 512; }
	my_Check_Load( tftp_high_load );

	InitSQLconnection();

	my_syslog( MLOG_TFTP, "docsis_server TFTP Version %s activated", VERSION );
	Clear_Remote_Commands();

	last = 0;
	hour6 = time(NULL);
	min1 = hour6;
	/* my_syslog( MLOG_TFTP, "tftp max num files %d", getdtablesize()); */

	while (tftpd_exit_flag) {
		// Update the PID file
		update_pid_file();

		FD_ZERO( &rfds );
		FD_SET( tftp_socket, &rfds );
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		tvp = &tv;

		if (select(tftp_socket + 1, &rfds, NULL, NULL, &tv ) < 0) {
			/* Don't choke when we get ptraced */
			if (errno == EINTR) continue;
			my_syslog( MLOG_ERROR, "tftp select: %s", strerror(errno) );
			exit(1);
		}


		if (FD_ISSET( tftp_socket, &rfds ) ) {
			/* Process a packet */
			fromlen = sizeof(from);
			cc = recvfrom(tftp_socket, buf, sizeof(buf), 0,
			    (struct sockaddr *)&from, &fromlen);
			if (cc < 0) {
				my_syslog( MLOG_ERROR, "tftp recvfrom: %s", strerror(errno) );
				continue;
			}

			/* Update now */
			now = time(NULL);

			/* Process this packet */
			process(&from, (struct tftphdr *)buf, cc);
			my_Check_Load( tftp_high_load );
		} else {
			now = time(NULL);
		}

		/* Run the timer list, no more than once a second */
		if (clientlistcnt > 0) {
			if (last != now) {
				last = now;
				runtimer();
			}
		}

		if ( (now - min1) >= 120) {
			min1 = now;
			free_file_cache();

			// ping the MySQL server
			my_SQL_Ping();
		}

		/* run the log stats 4 times a day */
		if ( (now - hour6) >= 21600) {
			hour6 = now;
			logstats();
		}
		Check_Remote_Commands();
	}
}

/* Process an incoming tftp packet */
void process( struct sockaddr_in *fromp, struct tftphdr *tp, int cc ) {
	Client *cl;

	/* Look for old session */
	cl = findclient( fromp );

	if (cl == NULL) {
		cl = newclient();
		if (cl == NULL) return;
		cl->sin = *fromp;
		cl->dynam = 0;
		cl->proc = CP_TFTP;
		/* log(LOG_DEBUG, "%s.%d starting", cl->s_ipaddr, cl->sin.sin_port); */
	}
	cl->tp = tp;
	cl->tpcc = cc;

	/* Start up this client */
	runclient( cl );
}

/*
 * Scan client list for guys that need to retransmit.
 */
void runtimer( ) {
	Client *cl, *cln;

	cl = clientlist;
	while( cl != NULL ) {
		cln = cl->next;
		cl->tp = NULL;
		cl->tpcc = 0;
		if (cl->wakeup != 0 && cl->wakeup <= now) {
			runclient( cl );
		} else if (cl->wakeup == 0) {
			cl->wakeup = now;
		}
		cl = cln;
	}
}

/* Execute a client proc */
void runclient( Client *cl ) {
	if (cl->proc == CP_TFTP) tftp( cl );

	if (cl->proc == CP_TFTP) { freeclient( cl ); return; }

	if (cl->proc == CP_SENDFILE) { my_sendfile( cl ); return; }

	if (cl->proc == CP_DONE) { freeclient( cl ); return; }

	// my_syslog( MLOG_ERROR, "tftp bad client proc %d", cl->proc );
}


/* Handle initial connection protocol  */
void tftp( Client *cl ) {
	struct tftphdr	*tp;
	char		*cp, *cp2, *ep;
	int		ecode;
	char		*mode;
	char		file[MAXPATHLEN];

	tp = cl->tp;
	tp->th_opcode = ntohs((u_short)tp->th_opcode);
	if (tp->th_opcode != RRQ) {
		// nak( cl, EBADOP );
		return;
	}

	errstr = NULL;
	cp = (char *)&tp->th_block;
	ep = (char *)cl->tp + cl->tpcc;

	if ( (ep - cp) > MAXPATHLEN ) {
		stats.badfilename++;
		nak( cl, EBADOP );
		return;
	}

	if ( *cp == '/' ) { cp++; }

	/* Extract filename */
	cp2 = file;
	while (cp < ep && *cp != '\0')
		*cp2++ = *cp++;
	if (cp >= ep) {
		++stats.badfilename;
		nak(cl, EBADOP);
		return;
	}
	*cp2 = '\0';
	++cp;

	/* Make sure mode is in packet */
	mode = cp;
	while (cp < ep && *cp != '\0') cp++;
	if (cp >= ep) {
		stats.badfilemode++;
		nak( cl, EBADOP );
		return;
	}

	if (strcasecmp(mode, "octet") != 0) {
		stats.badfilemode++;
		nak( cl, EBADOP );
		return;
	}

	ecode = validate_access( cl, file );
	if (ecode) {
		my_syslog( MLOG_TFTP, "%s %s \"%s\" DENIED (%s)",
			inet_ntoa(cl->sin.sin_addr),
			tp->th_opcode == WRQ ? "write" : "read",
			file,
			errstr != NULL ? errstr : errtomsg(ecode));
		cl->proc = CP_DONE;
		return;
	}

	if (cl->dynam == 0)
		my_TFTPlog( file, cl->sin.sin_addr.s_addr );
	/* my_syslog( MLOG_TFTP, "%s-%s req for \"%s\" from %s",
	    tp->th_opcode == WRQ ? "write" : "read",
	    mode, file, inet_ntoa(cl->sin.sin_addr));
	*/

	cl->state = ST_INIT;
	cl->tp = NULL;
	cl->tpcc = 0;
	cl->proc = CP_SENDFILE;
	// sendfile( cl );
}

/*
 * Validate file access. Since we have no uid or gid, for now require
 * file to exist and be publicly readable/writable.
 * If we were invoked with arguments from inetd then the file must also
 * be in one of the given directory prefixes. Note also, full path name
 * must be given as we have no login directory.
 */
int validate_access( Client *cl, char *file ) {
	struct stat stbuf;

	/*
	 * Prevent tricksters from getting around the directory restrictions
	 */
	if (strstr(file, "/../")) {
		errstr = "\"..\" in path";
		return (EACCESS);
	}
	if (strstr(file, "../")) {
		errstr = "..\" in path";
		return (EACCESS);
	}
	if (stat(file, &stbuf) < 0) {
		if (errno == ENOENT) {
			if (read_dynamic_file( cl->sin.sin_addr.s_addr,
				(u_int32_t *) &(cl->fnum), (u_int32_t *) &(cl->maxb) )) {
				errno = 0;
				cl->dynam = 1;
				return 0;
			}
		}
		errstr = strerror(errno);
		return (errno == ENOENT ? ENOTFOUND : EACCESS);
	}
	if ((stbuf.st_mode & S_IFMT) != S_IFREG) {
		errstr = "not a regular file";
		return (ENOTFOUND);
	}
	if ((stbuf.st_mode & S_IROTH) == 0) {
		errstr = "not readable";
		return (EACCESS);
	}
	if ( read_static_file( file, (u_int32_t *) &(cl->fnum), (u_int32_t *) &(cl->maxb) ) != 0 ) {
		return( EACCESS );
	}
	return 0;
}

/* Send the requested file */
void my_sendfile( Client *cl ) {
	int oerrno;
	struct tftphdr *tp = cl->tp;
	int opcode, block, readblock;

	readblock = 0;
	if (tp != NULL) {
		/* Process an input packet */
		opcode = ntohs((u_short)tp->th_opcode);

		/* Give up if the other side wants to */
		if (opcode == ERROR) {
			stats.errorop++;
			my_syslog( MLOG_TFTP, "%s.%d ERROR",
			    inet_ntoa(cl->sin.sin_addr), cl->sin.sin_port);
			cl->proc = CP_DONE;
			return;
		}

		/* If we didn't get an ACK, blow him away */
		//if (opcode != ACK) {
		//	/* XXX this probably can't happen */
		//	my_syslog( MLOG_TFTP, "%s.%d expected ACK",
		//	    inet_ntoa(cl->sin.sin_addr), cl->sin.sin_port);
		//	nak( cl, ERROR );
		//	return;
		//}
		if (opcode == RRQ) {
			cl->state = ST_INIT;
		}

		if (cl->state == ST_WAITACK || cl->state == ST_LASTACK) {
			/* If he didn't ack the right block, ignore packet */
			block = ntohs((u_short)tp->th_block);
			if (block > cl->maxb) { nak( cl, ERROR ); return; }

			/* Check if we're done */
			if (cl->state == ST_LASTACK && block == cl->maxb) { cl->proc = CP_DONE; return; }

			if (block != cl->block) cl->block = block;

			/* Setup to send the next block */
			cl->block++;
			readblock++;
			cl->state = ST_SEND;
			cl->wakeup = 0;
			cl->retries = WAIT_RETRIES;
		}
	}

	/* See if it's time to resend */
	if (cl->wakeup != 0 && cl->wakeup <= now &&
	   (cl->state == ST_WAITACK || cl->state == ST_LASTACK)) {
		stats.retrans++;
		my_syslog( MLOG_TFTP, "%s retrans blk %d",
		    inet_ntoa(cl->sin.sin_addr), cl->block );
		cl->state = ST_SEND;
	}

	/* Initialize some stuff the first time through */
	if (cl->state == ST_INIT) {
		cl->block = 1;
		readblock++;
		cl->state = ST_SEND;
		cl->wakeup = 0;
		cl->retries = WAIT_RETRIES;
		cl->dp = NULL;
	}

	/* If our first time through or the last block was ack'ed, read */
	if (readblock) {
		errno = 0;
		if (read_block( cl->fnum, cl->block, &(cl->dp), (u_int32_t *) &(cl->dpcc) ) != 0) {
			stats.fileioerror++;
			nak( cl, errno + 100 );
			return;
		}

		/* Bail if we run into trouble */
		if (cl->dpcc < 0) {
			stats.fileioerror++;
			nak( cl, errno + 100 );
			return;
		}
	}

	if (cl->state == ST_SEND) {
		/* See if it's time to give up on this turkey */
		cl->retries--;
		if (cl->retries < 0) {
			stats.timeout++;
			my_syslog( MLOG_TFTP, "%s timeout blk %d",
			    inet_ntoa(cl->sin.sin_addr), cl->block );
			cl->proc = CP_DONE;
			return;
		}

		/* Send the block */
		if (sendto(tftp_socket, (char *)cl->dp, cl->dpcc, 0,
		    (struct sockaddr *)&cl->sin, sizeof(cl->sin)) != cl->dpcc) {
			oerrno = errno;
			my_syslog( MLOG_TFTP, "sendfile: write: %m");
			nak( cl, oerrno + 100 );
			return;
		}
		stats.sent++;

		/*	log(LOG_DEBUG, "%s.%d > %d",
			    inet_ntoa(cl->sin.sin_addr),
			    cl->sin.sin_port,
			    cl->block);
		*/

		if (cl->dpcc == (SEGSIZE + 4))
			cl->state = ST_WAITACK;
		else
			cl->state = ST_LASTACK;
		cl->wakeup = now + WAIT_TIMEOUT;
	}
}


static struct errmsg {
	int	e_code;
	char	*e_msg;
} errmsgs[] = {
	{ EUNDEF,	"Undefined error code" },
	{ ENOTFOUND,	"File not found" },
	{ EACCESS,	"Access violation" },
	{ ENOSPACE,	"Disk full or allocation exceeded" },
	{ EBADOP,	"Illegal TFTP operation" },
	{ EBADID,	"Unknown transfer ID" },
	{ EEXISTS,	"File already exists" },
	{ ENOUSER,	"No such user" },
	{ 0,		NULL }
};

char * errtomsg( int error ) {
	struct errmsg	*pe;
	static char	*buf = NULL;

	if (buf == NULL)
		buf = (char *) calloc(1,80);
	if (error == 0)
		return "success";
	for (pe = errmsgs; pe->e_msg != NULL; pe++)
		if (pe->e_code == error)
			return (pe->e_msg);
	sprintf(buf, "error %d", error);
	return (buf);
}

/*
 * Send a nak packet (error message).
 * Error code passed in is one of the
 * standard TFTP codes, or a UNIX errno
 * offset by 100.
 */
void nak( Client *cl, int error ) {
	int size;
	struct tftphdr *tp;
	struct errmsg *pe;
	char buf[SEGSIZE + 4];

	cl->proc = CP_DONE;
	memset(buf, 0, sizeof(buf));
	tp = (struct tftphdr *)buf;
	tp->th_opcode = htons((u_short)ERROR);
	tp->th_code = htons((u_short)error);
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0) {
		pe->e_msg = strerror(error - 100);
		tp->th_code = EUNDEF;   /* set 'undef' errorcode */
	}
	strncpy(tp->th_msg, pe->e_msg, SEGSIZE);
	size = strlen(pe->e_msg);
	size += 4 + 1;
	if (sendto(tftp_socket, buf, size, 0, (struct sockaddr *)&cl->sin,
	    sizeof(cl->sin)) != size)
		my_syslog( MLOG_TFTP, "nak: %s", strerror(errno) );	
}



/* Signal handler to output statistics */
void	logstats( void ) {

	my_syslog( MLOG_STATS, "tftp: clientlistcnt %d", clientlistcnt );
	my_syslog( MLOG_STATS, "tftp: retrans %ld", stats.retrans );
	my_syslog( MLOG_STATS, "tftp: timeout %ld", stats.timeout );
	my_syslog( MLOG_STATS, "tftp: badop %ld", stats.badop );
	my_syslog( MLOG_STATS, "tftp: errorop %ld", stats.errorop );
	my_syslog( MLOG_STATS, "tftp: badfilename %ld", stats.badfilename );
	my_syslog( MLOG_STATS, "tftp: badfilemode %ld", stats.badfilemode );
	my_syslog( MLOG_STATS, "tftp: wrongblock %ld", stats.badfilemode );
	my_syslog( MLOG_STATS, "tftp: fileioerror %ld", stats.fileioerror );
	my_syslog( MLOG_STATS, "tftp: sent %ld", stats.sent );
	my_syslog( MLOG_STATS, "tftp: maxclients %ld", stats.maxclients );

	memset( &stats, 0, sizeof(struct stats) );
}

/* Allocate a new client struct */
Client * newclient() {
	Client	*cl;

	if (clientlistcnt >= MAXCLIENTS) return( NULL );

	cl = calloc(1, sizeof( Client ) +2);
	if (cl == NULL) return( NULL );

	clientlistcnt ++;
	if (clientlistcnt > stats.maxclients) stats.maxclients = clientlistcnt;

	if (clientlist == NULL) {
		clientlist = cl;
		return( cl );
	}

	cl->next = clientlist;
	clientlist->prev = cl;
	clientlist = cl;

	return( cl );
}

Client * findclient( struct sockaddr_in *sinp ) {
	Client			*cl;

	if (clientlist == NULL) return( NULL );

	cl = clientlist;
	while ( cl != NULL ) {
		if (sinp->sin_port == cl->sin.sin_port &&
		    sinp->sin_addr.s_addr == cl->sin.sin_addr.s_addr)
			return( cl );
		cl = cl->next;
	}
	return( NULL );
}

void freeclient( Client *cl ) {

	if (clientlistcnt == 1) {
		clientlistcnt = 0;
		clientlist = NULL;

		memset( cl, 0, sizeof( Client ) );
		free( cl );
		return;
	}

	clientlistcnt--;
	if (cl->prev != NULL) cl->prev->next = cl->next;
	if (cl->next != NULL) cl->next->prev = cl->prev;

	if (cl == clientlist) {
		clientlist = clientlist->next;
	}

	memset( cl, 0, sizeof( Client ) );
	free( cl );
	return;
}

void DoCMD_TFTP_Quit( void ) {
	tftpd_exit_flag = 0;
}
