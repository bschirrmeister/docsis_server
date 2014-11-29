/* dhcpd-dhcpd.c
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

 * Moreton Bay DHCP Server
 * Copyright (C) 1999 Matthew Ramsay <matthewr@moreton.com.au>
 *			Chris Trew <ctrew@moreton.com.au>
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
extern MYSQL		my_sql, *my_sock;
extern float		my_LoadAvg;
extern float		my_MaxLoad;
extern char		*process_type;
extern u_int64_t	Super_Flag;
/* ******************************************************************** */

// global variable
int dhcpd_exit_flag = 1;


// prototypes
int   send_positive_message( dhcp_message *message, int mess_type );
int   send_Offer( dhcp_message *message );
int   send_ACK( dhcp_message *message );
void  send_NAK( dhcp_message *message );
void  send_Query_NAK( dhcp_message *message );
void  send_Query_ACK( dhcp_message *message );
int   leaseQuery( dhcp_message *message );



void dhcp_server( void ) {
	char			*iface_pref, *euid_pref, *dbhost_pref, *force_chroot;
	char			*dhcp_high_load_c;
	int			dhcp_high_load;
	dhcp_message		message;
	u_int32_t		server_ip;
	struct passwd		*pass_struct;
	int			retval = 0, cc = 0, sentreply;

	iface_pref = my_GetDHCPint();
	server_ip = getInterfaceIP( iface_pref );
	init_DHCP_Socket( server_ip );

//	euid_pref = GetConfigVar( "effective-userid" );
//	pass_struct = getpwnam( euid_pref );
//	if (pass_struct) {
//		seteuid( pass_struct->pw_uid );
//		setegid( pass_struct->pw_gid );
//	}

	dbhost_pref = GetConfigVar( "mysql-host" );
	force_chroot = GetConfigVar( "force-localhost-chroot" );
//	if (strcmp(dbhost_pref,"localhost") || !strcmp(force_chroot, "yes")) {
//		// We cannot chroot if connect to the DB via
//		// a unix socket vs tcp
//		chroot( CHROOT_PATH );
//	}

	dhcp_high_load_c = GetConfigVar( "dhcp-high-load" );
	if (*dhcp_high_load_c == ' ') { dhcp_high_load_c = "16"; }
	dhcp_high_load = strtoul( dhcp_high_load_c, NULL, 10 );
	if (dhcp_high_load <= 4) { dhcp_high_load = 4; }
	if (dhcp_high_load >= 128) { dhcp_high_load = 128; }
	my_Check_Load( dhcp_high_load );

	InitSQLconnection();

	my_syslog( MLOG_DHCP, "docsis_server DHCP version %s activated", VERSION);
	Clear_Remote_Commands();

	while (dhcpd_exit_flag) { /* loop until universe collapses */
		/* update the PID file */
		update_pid_file();

		/* Clear Message Struct */
		memset( &message, 0, sizeof( dhcp_message ) );

		retval = getPacket( &message );
		if (retval == -2) Check_Remote_Commands();
		if (retval < 0) {
			// ping the MySQL server
			my_SQL_Ping();
			continue;
		}
		message.server_ip = server_ip;

		if (Check_Canary) { fprintf(stderr,"canary A died %llu %llu %llu "
				" %llu %llu %llu  %llu %llu %llu\n",Canaries); continue; }

 		DecodeOptions( &message );
		if (message.in_opts.message_type == 0) {
			message.in_opts.message_type = DHCP_REQUEST;
		}

		if (Check_Canary) { fprintf(stderr,"canary B died %llu %llu %llu "
				" %llu %llu %llu  %llu %llu %llu\n",Canaries); continue; }

		sentreply = 0;
		switch( message.in_opts.message_type ) {
		case DHCP_DISCOVER:
			sentreply = send_Offer( &message );
			break;
			
		case DHCP_REQUEST:
			sentreply = send_ACK( &message );
			break;

		case DHCP_RELEASE:
			/* checkRelease( &message ); */
			break;
		case DHCP_INFORM:
			sentreply = send_ACK( &message );
			break;

		case DHCP_DECLINE:
			/* ignore */
			break;

		case DHCP_LEASE_QUERY:	/* wierd ubR thingy  0x0d */
			sentreply = leaseQuery( &message );
			break;
			
		default: {
			my_syslog(LOG_WARNING, "unsupported DHCP message (%02x) %s -- ignoring",
				message.in_opts.message_type, message.s_macaddr );
			}
		}

		if (Check_Canary) { fprintf(stderr,"canary C died %llu %llu %llu "
				" %llu %llu %llu  %llu %llu %llu\n",Canaries); continue; }

		if (sentreply) my_Check_Load( dhcp_high_load );
		Check_Remote_Commands();
	}

	my_syslog(LOG_INFO, "exit");
	Flush_ALL_SQL_Updates();
	closelog();
	exit(0);
}



int send_positive_message( dhcp_message *message, int mess_type ) {
	static char	*newip_pref = NULL;
	u_int32_t	lease_time;
	ConfigNets	*netopts;

	/* check SQL table for this MAC address */
	my_findMAC( message );

	if (message->lease_type == LEASE_NOT_FOUND && message->in_opts.docsis_modem == 1 ) {
		my_syslog(LOG_INFO, "NAK -- unknown docsis modem: %s : %s ", message->s_macaddr,message->in_opts.vsi_model );
		my_findMAC_CMUNKNOWN( message );
		//send_NAK( message );
		//return 1;
	}
        
	if (message->lease_type == LEASE_REJECT) { return 0; }
	
	if (message->lease_type == LEASE_CPE &&
	    message->cpe_type == CPE_DYNAMIC &&
	    message->lockip == 0 &&
	    message->lease_time < 60 ) {
		// Your Lease has expired. You get a new IP!
		my_DeleteLease( message );
		message->lease_type = LEASE_NOT_FOUND;
	}

	if ( Verify_Vlan( message ) ) {
		if (message->lease_type == LEASE_CM ) {
			my_syslog(LOG_WARNING,
				"NEW CM GIaddr mismatch (the CM moved) gi %s ip %s vlan %d",
				message->in_giaddr, message->s_ipaddr, message->vlan );
			message->ipaddr = message->b_ipaddr = 0;
			message->s_ipaddr[0] = 0;
		}
                if (message->lease_type == LEASE_MTA ) {
                        my_syslog(LOG_WARNING,
                                "NEW MTA GIaddr mismatch (the CM moved) gi %s ip %s vlan %d",
                                message->in_giaddr, message->s_ipaddr, message->vlan );
                        message->ipaddr = message->b_ipaddr = 0;
                        message->s_ipaddr[0] = 0;
                }
		if (message->lease_type == LEASE_CPE ) {
			my_syslog(LOG_WARNING, "NEW -- GIaddr does not match vlan - gi %s ip %s vlan %d",
				message->in_giaddr, message->s_ipaddr, message->vlan );
			if (message->cpe_type == CPE_STATIC) {
				my_DeleteLease( message );
				message->lease_type = LEASE_NOT_FOUND;
			} else {
				// if message->cpe_type == CPE_DYNAMIC
				my_DeleteLease( message ); /* added 29.11.2014 */
				message->lease_type = LEASE_NOAUTH;
			}
		}
	}

	if (message->lease_type == LEASE_NOT_FOUND && message->in_opts.agent_mac_len == 0) {
		my_syslog(LOG_WARNING, "NAK -- CPE asking for new IP directly - mac %s",
			message->s_macaddr );
		send_NAK( message );
		return 0;
	}

	if (newip_pref == NULL) newip_pref = GetConfigVar( "assign-new-ip" );

	if (message->lease_type == LEASE_NOT_FOUND) {
		if (*newip_pref == 'n') return 0;

		my_getNewIP_CPE( message );

		if (message->ipaddr == 0) {
			send_NAK( message );
			return 1;
		}
		if (message->lease_type == LEASE_CPE) {
			if (message->cpe_type == CPE_STATIC) {
				my_syslog(LOG_INFO, "NEW STATIC -- mac %s ip %s vlan %d",
					message->s_macaddr, message->s_ipaddr, message->vlan );
			}
			if (message->cpe_type == CPE_DYNAMIC) {
				my_syslog(LOG_INFO, "NEW DYNAMIC -- mac %s ip %s vlan %d",
					message->s_macaddr, message->s_ipaddr, message->vlan );
			}
		}
		if (message->lease_type == LEASE_NOAUTH) {
			my_syslog(LOG_INFO, "NEW NOAUTH -- mac %s ip %s vlan %d",
				message->s_macaddr, message->s_ipaddr, message->vlan );
                	send_NAK( message ); /* added 29.11.2014 */
                	return 1;
		}
	}

	if ( my_Get_Net( message ) ) {
		my_syslog(LOG_WARNING, "NAK -- no subnet for ip %s mac %s vlan %d, lease_type %s",
			message->s_ipaddr, message->s_macaddr, message->vlan, message->lease_type );
		send_NAK( message );
		return 1;
	}

	if (message->in_opts.request_addr != 0 ) {
		if (message->ipaddr != message->in_opts.request_addr ) {
			my_syslog(LOG_ERR, "WARN -- bad ip requested %s mac %s real-ip %s",
				message->in_opts.s_request_addr, message->s_macaddr, message->s_ipaddr );
		}
	}

	netopts = Get_Net_Opts( message->netptr );

	message->out_pack.op    = BOOTREPLY;
	message->out_pack.htype = ETH_10MB;
	message->out_pack.hlen  = ETH_10MB_LEN;
	message->out_pack.hops  = message->in_pack.hops;
	message->out_pack.flags = message->in_pack.flags;
	message->out_pack.xid   = message->in_pack.xid;

	message->out_pack.ciaddr = message->in_pack.ciaddr;
	message->out_pack.yiaddr = message->ipaddr;
	message->out_pack.siaddr = message->server_ip;
	message->out_pack.giaddr = message->in_pack.giaddr;
	message->out_pack.cookie = DHCP_MAGIC;

	memcpy( message->out_pack.macaddr, message->macaddr, 6 );

	initOpt( message );

	if (mess_type == DHCP_OFFER) {
		addOpt( 0x35, 0x01, "\x02");
	}
	if (mess_type == DHCP_ACK) {
		addOpt( 0x35, 0x01, "\x05");
	}

	// subnet mask
	addOpt( 0x01, 0x04, (char *) &(netopts->mask) );

	// gateway
	addOpt( 0x03, 0x04, (char *) &(netopts->gateway) );

	// Broadcast Address
	addOpt( 0x1c, 0x04, (char *) &(netopts->broadcast) );

	// Lease Times
	if (message->cpe_type == CPE_STATIC || message->lockip == 1) {
		message->lease_time = netopts->lease_time;
		lease_time = htonl( message->lease_time );
		addOpt( 0x33, 0x04, (char *) &lease_time );
		lease_time = htonl( message->lease_time - 300 );
		addOpt( 0x3A, 0x04, (char *) &lease_time );
		addOpt( 0x3B, 0x04, (char *) &lease_time );
	} else {
		// if message->cpe_type == CPE_DYNAMIC
		lease_time = htonl( (u_int32_t) message->lease_time );
		if (message->lease_time < 0) lease_time = htonl( netopts->lease_time / 2 );
		addOpt( 0x33, 0x04, (char *) &lease_time );
		lease_time = ntohl( lease_time ) - 15;
		if (lease_time < 5) lease_time = 5;
		lease_time = htonl( lease_time );
		addOpt( 0x3A, 0x04, (char *) &lease_time );
		addOpt( 0x3B, 0x04, (char *) &lease_time );
	}

	addBigOpt( my_Get_Opt( 1, message->b_macaddr ) );
	addBigOpt( my_Get_Opt( message->opt, 0 ) );
	if (netopts->opt1 != message->opt) 
		addBigOpt( my_Get_Opt( netopts->opt1, 0 ) );
	if (netopts->opt2 != message->opt)
		addBigOpt( my_Get_Opt( netopts->opt2, 0 ) );
	if (netopts->opt3 != message->opt)
		addBigOpt( my_Get_Opt( netopts->opt3, 0 ) );

	/* Config File Name */
	if (message->cfname != NULL && message->cfname[0] != 0) {
		strncpy( message->out_pack.file, message->cfname, 127 );
		addOpt( 0x43, strlen( message->out_pack.file ), message->cfname );
	} else {
		char	*cf = Lookup_ConfigOptsMacs(message,0x43);
		if (cf != NULL)
			strncpy( message->out_pack.file, cf, 127 );
	}

	/* Add Terminator */
	addOpt( 0xff, 0x00, NULL );

	my_dhcplog( mess_type, message );

	sendPacket( message );

	// if (mess_type == DHCP_OFFER && 
	if (message->lease_type == LEASE_CM) {
		// Update AgentID of cablemodem
		my_UpdateAgent( message );
	}
        if (message->lease_type == LEASE_MTA) {
                // Update AgentID of cablemodem
                my_UpdateAgent( message );
        }

	if (message->lease_type == LEASE_UNKNOWN) {
		// Update AgentID of cablemodem
		my_UpdateAgent( message );
	}

	/* if a customer pc, update the leases table, if a cable modem dont bother */
	if (message->lease_type == LEASE_CPE || message->lease_type == LEASE_NOAUTH ) {
		my_UpdateLease( message );
	}
	return 1;
}

/* send a DHCP OFFER to a DHCP DISCOVER */
int send_Offer( dhcp_message *message ) {
	return( send_positive_message( message, DHCP_OFFER ) );
}

/* send a DHCP ACK to a DHCP REQUEST */
int send_ACK( dhcp_message *message ) {
	return( send_positive_message( message, DHCP_ACK ) );
}


void send_NAK( dhcp_message *message ) {
	message->out_pack.op    = BOOTREPLY;
	message->out_pack.htype = ETH_10MB;
	message->out_pack.hlen  = ETH_10MB_LEN;
	message->out_pack.hops  = message->in_pack.hops;
	message->out_pack.flags = message->in_pack.flags;
	message->out_pack.xid   = message->in_pack.xid;

	message->out_pack.ciaddr = message->in_pack.ciaddr;
	message->out_pack.siaddr = message->server_ip;
	message->out_pack.giaddr = message->in_pack.giaddr;
	message->out_pack.cookie = DHCP_MAGIC;

	memcpy( message->out_pack.macaddr, message->macaddr, 6 );

	initOpt( message );
	addOpt( 0x35, 0x01, "\x06" );  /* DHCP NAK */
	addOpt( 0x36, 0x04, (char *) &message->server_ip );
	addOpt( 0xff, 0x00, NULL );

	my_dhcplog( DHCP_NAK, message );

	sendPacket( message );
}

void send_Query_NAK( dhcp_message *message ) {
	send_NAK( message );
}

void send_Query_ACK( dhcp_message *message ) {
	u_int32_t	lease_time;

	message->out_pack.op    = BOOTREPLY;
	message->out_pack.htype = ETH_10MB;
	message->out_pack.hlen  = ETH_10MB_LEN;
	message->out_pack.hops  = message->in_pack.hops;
	message->out_pack.flags = message->in_pack.flags;
	message->out_pack.xid   = message->in_pack.xid;

	message->out_pack.ciaddr = message->in_pack.ciaddr;
	message->out_pack.yiaddr = message->ipaddr;
	message->out_pack.siaddr = message->server_ip;
	message->out_pack.giaddr = message->in_pack.giaddr;
	message->out_pack.cookie = DHCP_MAGIC;

	memcpy( message->out_pack.macaddr, message->macaddr, 6 );

	if (message->cpe_type == CPE_DYNAMIC) lease_time = htonl( (u_int32_t) message->lease_time );
	if (message->cpe_type == CPE_STATIC) lease_time = htonl( 3600 * 12 );  // give em 12 hours

	initOpt( message );
	addOpt( 0x35, 0x01, "\x05" );
	addOpt( 0x33, 0x04, (char *) &lease_time );
	lease_time = ntohl( lease_time ) - 60;
	if (lease_time < 5) lease_time = 5;
	lease_time = htonl( lease_time );
	addOpt( 0x3A, 0x04, (char *) &lease_time );
	addOpt( 0x3B, 0x04, (char *) &lease_time );
	if( my_findAgent( message ) ) {
		addOpt( 0x52, message->in_opts.agent_mac_len, NULL );
		addOpt( 0x01, message->in_opts.agentid_len,   (char *) message->in_opts.agentid );
		addOpt( 0x02, message->in_opts.modem_mac_len, (char *) message->in_opts.modem_mac );
	}
	addOpt( 0xff, 0x00, NULL );

	my_dhcplog( DHCP_QUERY_ACK, message );

	my_UpdateTime( message );

	sendPacket( message );
}



int	leaseQuery( dhcp_message *message ) {
	if ( (message->in_pack.htype != 0) &&
	     (message->in_pack.hlen  != 0) ) {
		/* Search by MAC address */
		my_findMAC( message );

		if (message->lease_type == LEASE_REJECT) return 0;

		if (message->lease_type == LEASE_CPE &&
		    message->cpe_type == CPE_DYNAMIC &&
		    message->lockip == 0 &&
		    message->lease_time < 0 ) {
			// Your Lease has expired. You get a new IP!
			my_DeleteLease( message );
			message->lease_type = LEASE_NOT_FOUND;
		}

		if (message->lease_type == LEASE_NOT_FOUND ) {
			send_Query_NAK( message );
		} else {
			send_Query_ACK( message );
		}
		return 1;
	} else if (message->in_pack.ciaddr != 0) {
		/* Search by IP -- The majority of requests from Cisco CMTS*/
		my_findIP( message, FIND_CIADDR );

		if (message->lease_type == LEASE_REJECT) return 0;

		if (message->lease_type == LEASE_CPE &&
		    message->cpe_type == CPE_DYNAMIC &&
		    message->lockip == 0 &&
		    message->lease_time < 0 ) {
			// Your Lease has expired. You get a new IP!
			my_DeleteLease( message );
			message->lease_type = LEASE_NOT_FOUND;
		}

		if (message->lease_type == LEASE_NOT_FOUND ) {
			send_Query_NAK( message );
		} else {
			send_Query_ACK( message );
		}
		return 1;
	} else if (message->in_opts.client_ident_len > 0) {
		/* Search by dhcp-client-ident flag */
		/* Im not sure how I will do this. Log it */
		char	tmpstr[4096];
		cv_macaddr( message->in_opts.client_ident, tmpstr,
			    message->in_opts.client_ident_len );
		my_syslog( LOG_INFO, "DHCPlq - dhcp-client-ident search -%s-", tmpstr );
		return 0;
	}
}

void DoCMD_DHCP_Quit( void ) {
	dhcpd_exit_flag = 0;
}

