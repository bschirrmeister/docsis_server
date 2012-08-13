/* tftpd-tftpd.h
 *
 * docsis_server
 *
 * Large portions of this source code are
 * Copyright (C) 2002 Access Communications
 *                    email docsis_guy@accesscomm.ca
 *
 * Designed for use with DOCSIS cable modems and hosts.
 * And for use with MySQL Database Server.
 *
 ********************************************************************
 **  This server still slightly resembles:

/* tftpd
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


#ifndef _TFTPD_H
#define _TFTPD_H


/* Everything we need to keep track of a client */
typedef struct client {
	struct sockaddr_in sin;		/* address and port of client */
	time_t		wakeup;		/* time to wakeup */
	int 		retries;	/* retries left until we give up */
	struct tftphdr	*tp;		/* pointer to received packet */
	struct tftphdr	*dp;		/* pointer to output packet */
	int 		tpcc;		/* size of data in tp */
	int 		dpcc;		/* size of data in dp */
	int 		proc;		/* client proc */
	int 		state;		/* client state */
	int 		block;		/* block in file */
	int		fnum;		/* filenumber */
	int		maxb;		/* number of blocks in file */
	int		dynam;		/* dynamic file flag */

	struct client	*next;		// Pointer to next Item
	struct client	*prev;		// Pointer to prev Item
} Client;


/* Client states */
#define CP_FREE		0
#define CP_TFTP		1		/* ready to start a session */
#define CP_SENDFILE	2		/* ready to send a block */
#define CP_RECVFILE	3		/* ready to recv a block */
#define CP_DONE		4		/* all done */

#define ST_INIT		1		/* initialize */
#define ST_SEND		2		/* send a block */
#define ST_WAITACK	3		/* waiting for a block to be ack'ed */
#define ST_LASTACK	4		/* sent last block, wait for last ack */


/* Internal statistics */
struct stats {
	u_int32_t retrans;		/* number of retransmissions */
	u_int32_t timeout;		/* number of timed out clients */
	u_int32_t badop;		/* number of was expecting ACK */
	u_int32_t errorop;		/* received ERROR from client */
	u_int32_t badfilename;		/* filename wasn't parsable */
	u_int32_t badfilemode;		/* file mode wasn't parsable */
	u_int32_t wrongblock;		/* client ACKed wrong block */
	u_int32_t fileioerror;		/* errors reading the file */
	u_int32_t sent;			/* number of packets sent */
	u_int32_t maxclients;		/* maximum concurrent clients */
};


#define WAIT_TIMEOUT 2			/* seconds between retransmits */
#define WAIT_RETRIES 6			/* retransmits before giving up */

#define MAXCLIENTS 2000			/* max number of clients to service at once */


// function prototypes
char	*errtomsg( int );
Client	*findclient( struct sockaddr_in * );
void	freeclient( Client* );
void	nak( Client*, int );
Client	*newclient( void );
void	process( struct sockaddr_in *, struct tftphdr *, int );
void	runclient( Client * );
void	runtimer( void );
void	my_sendfile( Client * );
void	tftp( Client * );
int	validate_access( Client *, char * );

void	logstats( void );

void    DoCMD_TFTP_Quit( void );


#endif /* _TFTPD_H */

