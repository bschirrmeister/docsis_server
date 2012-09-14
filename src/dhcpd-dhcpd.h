/* dhcpd-dhcpd.h
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
 *                      Chris Trew <ctrew@moreton.com.au>
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

#ifndef _DHCPD_H
#define _DHCPD_H


/*****************************************************************/
/* Do not modify below here unless you know what you are doing!! */
/*****************************************************************/

/* DHCP protocol -- see RFC */
#define LISTEN_PORT		67
#define SEND_PORT		68

/* Big Endian */
// #define DHCP_MAGIC		0x63825363
/* Little Endian */
#define DHCP_MAGIC		0x63538263

#define DHCP_MESSAGE_TYPE	0x35
#define DHCP_SERVER_ID		0x36
#define DHCP_CLIENT_ID		0x3d

#define BOOTREQUEST		1
#define BOOTREPLY		2

#define ETH_10MB		1
#define ETH_10MB_LEN		6

#define DHCP_DISCOVER		1
#define DHCP_OFFER		2
#define DHCP_REQUEST		3
#define DHCP_DECLINE		4
#define DHCP_ACK		5
#define DHCP_NAK		6
#define DHCP_RELEASE		7
#define DHCP_INFORM		8
#define DHCP_LEASE_QUERY	0x0d
#define DHCP_Q_ACK		98
#define DHCP_QUERY_ACK		98
#define DHCP_Q_NAK		99
#define DHCP_QUERY_NAK		99

#define FIND_IPADDR		1
#define FIND_CIADDR		2
#define FIND_YIADDR		3

#define LEASE_NOT_FOUND		0
#define LEASE_CM		1
#define LEASE_CPE		2
#define LEASE_MTA		3
#define LEASE_UNKNOWN		4
#define LEASE_NOAUTH		8
#define LEASE_REJECT		9

#define CPE_STATIC		0
#define CPE_DYNAMIC		1


/* miscellaneous defines */
#define IPLIST			0
#define LEASED			1
#define TRUE			1
#define FALSE			0
#define MAX_BUF_SIZE		20 /* max xxx.xxx.xxx.xxx-xxx\n */
#define MAX_IP_ADDR		254
#define ADD			1
#define DEL			2

#define MAX_OPTIONS_LENGTH 768

#define MAX_CONFIG_FILE_NAME	50


typedef struct _dhcp_packet {
	u_int8_t op;
	u_int8_t htype;
	u_int8_t hlen;
	u_int8_t hops;
	u_int32_t xid;
	u_int16_t secs;
	u_int16_t flags;
	u_int32_t ciaddr;
	u_int32_t yiaddr;
	u_int32_t siaddr;
	u_int32_t giaddr;
	u_int8_t macaddr[16];
	u_int8_t sname[64];
	u_int8_t file[128];
	u_int32_t cookie;
	u_int8_t options[MAX_OPTIONS_LENGTH];
} dhcp_packet;


typedef struct _packet_opts {
	/* option 0x0c */
	u_int32_t       host_name_len;
	char            host_name[2048];

	/* option 0x2b */
	u_int32_t       vsi_len;
	char		vsi[2048];
	char		vsi_serialno[255];
	char		vsi_hwver[255];
	char		vsi_swver[255];
	char		vsi_bootrom[255];
	char		vsi_oui[255];
	char		vsi_model[255];
	char		vsi_vendor[255];

	/* option 0x32 */
	u_int32_t       request_addr;
	char		s_request_addr[20];

	/* option 0x35 */
	unsigned char   message_type;

	/* option 0x36 */
	u_int32_t       server_ident;

	/* option 0x37 */
	u_int32_t       request_list_len;
	unsigned char   request_list[2048];

	/* option 0x3c */
	u_int32_t       vendor_ident_len;
	unsigned char   vendor_ident[2048];
	int             docsis_modem;
	char		version[10];
	int		mce_concat, mce_ver, mce_frag, mce_phs, mce_igmp, mce_bpi;
	int		mce_ds_said, mce_us_sid, mce_filt_dot1p, mce_filt_dot1q;
	int		mce_tetps, mce_ntet, mce_dcc;

	/* option 0x3d */
	u_int32_t       client_ident_len;
	unsigned char   client_ident[2048];

	/* option 0x51 */
	u_int32_t       fqdn_len;
	char            fqdn[2048];

	/* option 0x52 */
	u_int32_t       agent_mac_len;
	u_int32_t       agentid_len;
	unsigned char   agentid[2048];
	char		s_agentid[1024];
	u_int32_t       modem_mac_len;
	unsigned char   modem_mac[2048];
	char		s_modem_mac[1024];
	u_int64_t	b_modem_mac;
} packet_opts;



typedef struct config_nets {
	int8_t		nettype;
	u_int32_t	cmts_ip;
	u_int16_t	cmts_vlan;
        u_int32_t       network;
        u_int32_t       mask;
        u_int32_t       broadcast;
        u_int32_t       gateway;
	int8_t		grant_flag;
	int8_t		dynamic_flag;
	int8_t		full_flag;
	u_int32_t	lease_time;
	u_int32_t	range_min;
	u_int32_t	range_max;
	u_int32_t	last_used_ip;
	u_int16_t	opt1, opt2, opt3;

	char		s_nettype[10];
	char		s_cmts_ip[21];
	char		s_network[21];
} ConfigNets;
// typedef ConfigNets* ConfigNetsPtr;

typedef struct config_opts {
	u_int64_t	b_macaddr;
	u_int16_t	id;
	u_int8_t	len;
	u_int8_t	*val;
} ConfigOpts;
// typedef ConfigOpts* ConfigOptsPtr;



typedef struct _dhcp_message {
	u_int64_t	canary_1;	// had some memory leak issues. canaries are there for memory check.
	dhcp_packet	in_pack;
	u_int64_t	canary_2;
	packet_opts	in_opts;
	u_int64_t	canary_3;
	dhcp_packet	out_pack;
	u_int64_t	canary_4;

	int		in_bytes;		/* bytes in incoming packet */
	int		out_bytes;		/* bytes we sent */
	u_int32_t	server_ip;		/* DHCP server's IP */
	u_int32_t	rcv_ipaddr;		/* IP we got packet from */
	char		s_rcv_ipaddr[20];
	u_int16_t	rcv_port;		/* port we got packet from */

	u_int64_t	canary_5;

	u_int8_t	*macaddr;		/* macaddr from in_pack.macaddr */
	char		s_macaddr[20];
	u_int64_t	b_macaddr;
	u_int32_t	ipaddr;			/* ipaddr to send back */
	char		s_ipaddr[20];
	u_int32_t	b_ipaddr;
	u_int32_t	old_ipaddr;		/* used for dynamic leases */
	char		olds_ipaddr[20];

	u_int64_t	canary_6;

	u_int8_t	modem_macaddr[10];
	char		s_modem_macaddr[20];
	u_int64_t	b_modem_macaddr;
	int		mcache;			/* flag if we used the memory cache */
	char		cfname[MAX_CONFIG_FILE_NAME];
	u_int16_t	vlan;			/* vlan to use */
	u_int64_t	subnum;			/* subscriber id */
	int		static_ip, static_cur;
	int		dynamic_ip, dynamic_cur;

	u_int64_t	canary_7;

	char		in_ciaddr[20];		/* char version of in_pack.  */
	char		in_yiaddr[20];
	char		in_siaddr[20];
	char		in_giaddr[20];

	int		netptr;			/* config_net ptr */
	u_int16_t	opt;			/* per device dhcp options */

	u_int64_t	canary_8;

	int		lease_type;		/* cust_pc, cable_modem, or not found */
	int		cpe_type;		/* static, dynamic or other */
	int		lockip;
	long		lease_time;		/* calculated lease time */
	long		lease_expire;		/* 1/10th of lease_time for Dynamic */

	u_int64_t	canary_9;
	
} dhcp_message;

#define Check_Canary			\
	message.canary_1 != 0 || message.canary_2 != 0 || message.canary_3 != 0 ||	\
	message.canary_4 != 0 || message.canary_5 != 0 || message.canary_6 != 0 ||	\
	message.canary_7 != 0 || message.canary_8 != 0 || message.canary_9 != 0

#define Canaries			\
	message.canary_1, message.canary_2, message.canary_3,		\
	message.canary_4, message.canary_5, message.canary_6,		\
	message.canary_7, message.canary_8, message.canary_9

// function prototypes
void DoCMD_DHCP_Quit( void );


#endif /* _DHCPD_H */

