/* dhcpd-options.c
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
 *
 * changed by Michael Bülte, D-75223 Niefern-Öschelbronn, michael.buelte@bridacom.de (MTA-Support)
 */
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "docsis_server.h"


u_int8_t *pp = NULL;
u_int8_t *pend = NULL;


// Prototype
void Parse_Modem_Caps( dhcp_message *message );
void Parse_CP10_Caps( dhcp_message *message );
void Parse_MTA_Caps( dhcp_message *message );


void  initOpt( dhcp_message *message ) {
	pp = message->out_pack.options;
	pend = message->out_pack.options;
	pend += MAX_OPTIONS_LENGTH;
}


void addOpt( unsigned char code, unsigned char datalength, char *data ) {
	int n;

	if ( (datalength + 2) > (pend - pp) ) {
		my_syslog( MLOG_DHCP, "DHCP Option Too Big - type %d len %d",
			code, datalength );
		return;
	}

	*pp = code;		pp++;

	if (datalength > 0) {
		*pp = datalength;	pp++;

		if (data != NULL) {
			for ( n=0; n < datalength; n++ ) {
				*pp = *data;
				pp++;
				data++;
			}
		}
	}
}

void addBigOpt( ConfigOpts *opt ) {

	if (opt == NULL) return;

	memcpy( pp, opt->val, opt->len );
	pp += opt->len;
}

void DecodeOptions( dhcp_message *message ) {
	packet_opts	*po;
	struct in_addr	s_ip;
	u_int8_t	*ino, *maxo;
	u_int8_t	kT, kL, kRL;
	int		flag = 1;
	char		*p;

	po = &(message->in_opts);
	ino = message->in_pack.options;
	maxo = ino + MAX_OPTIONS_LENGTH;

	while( flag ) {
		if (ino >= maxo) { flag = 0; continue; }
		kT = *ino; ino++; // DHCP-Option
		kL = *ino; ino++; // DHCP-Option-Length
		kRL = kL;
		if (kRL > (maxo - ino)) { kRL = (maxo - ino); }
		//fprintf(stderr,"DEBUG: OPTION: %d LENGTH: %d\n", kT, kL);
		switch( kT ) {
		case 0x00:
			flag = 0;
			break;

		case 0x0c:	/*  host-name  */
			memcpy( po->host_name, ino, kRL ); ino += kL;
			po->host_name_len = kRL;
			for( p = po->host_name; *p; p++ ) {
				if ( (*p < 48 ) ||
				     (*p >= 58 && *p <= 64) ||
				     (*p >= 91 && *p <= 96) ||
				     (*p >= 123 ) ) {
					*p = '-';
				}
			}
			break;

		case 0x2b:	/*  vendor-specific-information  */ {
                        u_int8_t        mmT, mmL, mmRL, mmCK;
			
			po->vsi_len = kRL;
                        mmCK = kRL;
                        while (mmCK > 0) {
                        	mmT = *ino; ino++; mmCK--; //DHCP-SubOption
                        	mmL = *ino; ino++; mmCK--; //DHCP-SubOptionLength
                      		mmRL = mmL;

                        	if (mmRL > (maxo - ino)) { mmRL = (maxo - ino); }
		//		fprintf(stderr,"DEBUG: OPTION: %d SUBOPTION: %d LENGTH: %d\n", kT, mmT, mmL);
                                switch( mmT ) {
                                case 0x04:              // Serialnumber
					memcpy( po->vsi_serialno, ino, mmRL ); ino += mmL; mmCK -= mmL;
                                        //fprintf(stderr,"SERIAL: %d %d %d %s\n", mmT, mmL, mmRL, po->vsi_serialno);
                                        break;
                                case 0x05:              // HW-Version
					memcpy( po->vsi_hwver, ino, mmRL ); ino += mmL; mmCK -= mmL;
                                        //fprintf(stderr,"HW-Version: %d %d %d %s\n", mmT, mmL, mmRL, po->vsi_hwver);
                                        break;
                                case 0x06:              // SW-Version
					memcpy( po->vsi_swver, ino, mmRL ); ino += mmL; mmCK -= mmL;
                                        //fprintf(stderr,"SW-Version: %d %d %d %s\n", mmT, mmL, mmRL, po->vsi_swver);
                                        break;
                                case 0x07:              // Bootrom
					memcpy( po->vsi_bootrom, ino, mmRL ); ino += mmL; mmCK -= mmL;
                                        //fprintf(stderr,"BOOTROM: %d %d %d %s\n", mmT, mmL, mmRL, po->vsi_bootrom);
                                        break;
                                case 0x08:              // Organizationally Uniqe Identifier
					memcpy( po->vsi_oui, ino, mmRL ); ino += mmL; mmCK -= mmL;
                                        //fprintf(stderr,"OUI: %d %d %d %s\n", mmT, mmL, mmRL, po->vsi_oui);
                                        break; 
                                case 0x09:              // Modelnumber
					memcpy( po->vsi_model, ino, mmRL ); ino += mmL; mmCK -= mmL;
                                        //fprintf(stderr,"MODEL: %d %d %d %s\n", mmT, mmL, mmRL, po->vsi_model);
                                        break;
                                case 0x0a:              // Vendorname
					memcpy( po->vsi_vendor, ino, mmRL ); ino += mmL; mmCK -= mmL;
                                        //fprintf(stderr,"VENDOR: %d %d %d %s\n", mmT, mmL, mmRL, po->vsi_vendor);
                                        break;
	                         default:
       		                          ino += mmL;  mmCK -= mmL;
                                 	  break;
                                        }
				}
			}
			break;

		case 0x32:	/*  dhcp-requested-address  */
			memcpy( &(po->request_addr), ino, 4 ); ino += kL;
			s_ip.s_addr = po->request_addr;
			strcpy( po->s_request_addr, inet_ntoa( s_ip ) );
			break;

		case 0x35:	/*  dhcp-message-type  */
			po->message_type = *ino; ino++; break;

		case 0x36:	/*  dhcp-server-identifier  */
			memcpy( &(po->server_ident), ino, 4 ); ino += kL; break;
			break;

		case 0x37:	/*  dhcp-parameter-request-list  */
			memcpy( po->request_list, ino, kRL ); ino += kL;
			po->request_list_len = kRL;
			break;

		case 0x39:	/*  dhcp-max-message-size  */
			memcpy( po->request_list, ino, kRL ); ino += kL; break;

		case 0x3c:	/*  vendor-class-identifier  */
			memcpy( po->vendor_ident, ino, kRL ); ino += kL;
			po->vendor_ident_len = kRL;
			po->docsis_modem = 0;
			if ( memcmp(po->vendor_ident, "docsis", 6) == 0) {
				po->docsis_modem = 1;
				Parse_Modem_Caps( message );
			}
			if ( memcmp(po->vendor_ident, "pktc", 4) == 0) { /* MTA MB-DOPS */
				po->docsis_modem = 1;
				Parse_MTA_Caps( message );
			}
			break;

		case 0x3d:	/*  dhcp-client-identifier  */
			memcpy( po->client_ident, ino, kRL ); ino += kL;
			po->client_ident_len = kRL;
			break;

		case 0x51:	/*  fqdn  */
			memcpy( po->fqdn, ino, kRL ); ino += kL;
			po->fqdn_len = kRL;
			break;

		case 0x52:	/*  relay-agent-information  */
                        {
                                u_int8_t        mmT, mmL, mmRL, mmCK;

                                po->agent_mac_len = kRL;
                                mmCK = kRL;
                                while (mmCK > 0) {

                                        mmT = *ino; ino++; mmCK--;
                                        mmL = *ino; ino++; mmCK--;
                                        mmRL = mmL;
                                        if (mmRL > (maxo - ino)) { mmRL = (maxo - ino); }
				        //fprintf(stderr,"DEBUG: OPTION: %d SUBOPTION: %d LENGTH: %d\n", kT, mmT, mmL);
					switch( mmT ) {
					case 0x01:		// agent id from Cisco CMTS
                                                memcpy( po->agentid, ino, mmRL ); ino += mmL; mmCK -= mmL;
                                                po->agentid_len = mmRL;
                                                cv_macaddr( po->agentid, po->s_agentid, mmRL );
                                                //fprintf(stderr,"AGENTID: %d %d %d %s -- \n", mmT, mmL, mmRL, po->s_agentid );
						break;
                                        case 0x02:		// CM mac address
                                                memcpy( po->modem_mac, ino, mmRL ); ino += mmL; mmCK -= mmL;
                                                po->modem_mac_len = mmRL;
                                                cv_macaddr( po->modem_mac, po->s_modem_mac, mmRL );
                                                cv_mac2num( &(po->b_modem_mac), po->modem_mac );
                                                strcpy( message->s_modem_macaddr, po->s_modem_mac );
                                                message->b_modem_macaddr = po->b_modem_mac;
                                                memcpy( message->modem_macaddr, po->modem_mac, 6 );
                                                //fprintf(stderr,"AGENTMAC: %d %d %d %s\n", mmT, mmL, mmRL, po->s_modem_mac );
						break;
					case 0x2b:		// CM IP from Moto
						ino += mmL;  mmCK -= mmL;
						break;
                                        default:
						ino += mmL;  mmCK -= mmL;
						break;
					}
                                }
                        }
			break;

		case 0xff:
			flag = 0;
			break;
		default:
			ino += kL;
			break;
		}
	}
}


void Parse_Modem_Caps( dhcp_message *message ) {
	packet_opts	*po;
	u_int8_t	tmp[50];
	int		c1, c2, i, j = 0;

	po = &(message->in_opts);
	po->mce_concat = -1;
	po->mce_ver = -1;
	po->mce_frag = -1;
	po->mce_phs = -1;
	po->mce_igmp = -1;
	po->mce_bpi = -1;
	po->mce_ds_said = -1;
	po->mce_us_sid = -1;
	po->mce_filt_dot1p = -1;
	po->mce_filt_dot1q = -1;
	po->mce_tetps = -1;
	po->mce_ntet = -1;
	po->mce_dcc = -1;
	memcpy( po->version, (po->vendor_ident + 6), 3 );
	if (po->vendor_ident[9] == ':' &&
	    po->vendor_ident[10] == '0' &&
	    po->vendor_ident[11] == '5') {
		for (i=14; po->vendor_ident[i]; i += 2 ) {
			c1 = po->vendor_ident[i] - 48;
			if (c1 < 0) c1 = 0;
			if (c1 > 9) {
				if (c1 >= 17 && c1 <= 22) c1 -= 7;
				if (c1 >= 49 && c1 <= 54) c1 -= 39;
				if (c1 >= 16) c1 = 0;
			}
			c2 = po->vendor_ident[i + 1] - 48;
			if (c2 < 0) c2 = 0;
			if (c2 > 9) {
				if (c2 >= 17 && c2 <= 22) c2 -= 7;
				if (c2 >= 49 && c2 <= 54) c2 -= 39;
				if (c2 >= 16) c2 = 0;
			}
			tmp[j] = (c1 << 4) + c2;
			j++;
		}
	}

	for( i=0; i < j; i++ ) {
		switch( tmp[i] ) {
		 case 1:	po->mce_concat = tmp[i + 2];
				i += 2; break;
		 case 2:	po->mce_ver = tmp[i + 2];
				i += 2; break;
		 case 3:	po->mce_frag = tmp[i + 2];
				i += 2; break;
		 case 4:	po->mce_phs = tmp[i + 2];
				i += 2; break;
		 case 5:	po->mce_igmp = tmp[i + 2];
				i += 2; break;
		 case 6:	po->mce_bpi = tmp[i + 2];
				i += 2; break;
		 case 7:	po->mce_ds_said = tmp[i + 2];
				i += 2; break;
		 case 8:	po->mce_us_sid = tmp[i + 2];
				i += 2; break;
		 case 9:	po->mce_filt_dot1p = (tmp[i + 2] & 1);
				po->mce_filt_dot1q = (tmp[i + 2] & 2);
				i += 2; break;
		 case 10:	po->mce_tetps = tmp[i + 2];
				i += 2; break;
		 case 11:	po->mce_ntet = tmp[i + 2];
				i += 2; break;
		 case 12:	po->mce_dcc = tmp[i + 2];
				i += 2; break;
		 default:
				i += 1 + tmp[i + 1];
				break;
		}
	}
}


void Parse_MTA_Caps( dhcp_message *message ) { /* MB-DOPS */
	packet_opts	*po;
	u_int8_t	tmp[50];
	int		c1, c2, i, j = 0;

	po = &(message->in_opts);
	memcpy( po->version, (po->vendor_ident + 0), 5 );
	if (po->vendor_ident[9] == ':' &&
	    po->vendor_ident[10] == '0' &&
	    po->vendor_ident[11] == '5') {
		for (i=14; po->vendor_ident[i]; i += 2 ) {
			c1 = po->vendor_ident[i] - 48;
			if (c1 < 0) c1 = 0;
			if (c1 > 9) {
				if (c1 >= 17 && c1 <= 22) c1 -= 7;
				if (c1 >= 49 && c1 <= 54) c1 -= 39;
				if (c1 >= 16) c1 = 0;
			}
			c2 = po->vendor_ident[i + 1] - 48;
			if (c2 < 0) c2 = 0;
			if (c2 > 9) {
				if (c2 >= 17 && c2 <= 22) c2 -= 7;
				if (c2 >= 49 && c2 <= 54) c2 -= 39;
				if (c2 >= 16) c2 = 0;
			}
			tmp[j] = (c1 << 4) + c2;
			j++;
		}
	}

}


//int Check_Requested_Options ( PacketOptions *po, unsigned char option ) {
//	int	i;
//
//	if (po->request_list_len < 1) { return 0; }
//
//	for (i=0; i < po->request_list_len; i++) {
//		if (po->request_list[i] == option) { return 1; }
//	}
//	return 0;
//}


/*******************************************************/

