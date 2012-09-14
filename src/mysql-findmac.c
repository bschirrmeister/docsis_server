/* mysql-findmac.c
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
extern float		my_LoadAvg;
extern float		my_MaxLoad;
extern char		*process_type;
extern u_int64_t	Super_Flag;
/* ******************************************************************** */


/* return 1 if found. return 0 if not found */
int   my_findMAC( dhcp_message *message ) {
	int	result;

	result = my_findBADMAC( message );
	if (result) return result;

	result = my_findMAC_CACHE( message );
	if (result) return result;

	result = my_findMAC_CM( message );
	if (result) return LEASE_CM;

	result = my_findMAC_CPE( message );
	if (result) return LEASE_CPE;

	result = my_findMAC_PRECPE( message );
	if (result) return LEASE_CPE;

	result = my_findMAC_CMUNKNOWN( message );
	if (result) return LEASE_UNKNOWN;

	return LEASE_NOT_FOUND;
}

int   my_findBADMAC( dhcp_message *message ) {
	char		qbuf[500];
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	struct in_addr	inp;
	int		numr;

	snprintf( qbuf, 500, "select * from dhcp_bad_macs "
		"where macaddr = '%s'", message->s_macaddr );
	SQL_QUERY_RET0( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) { return 0; }

	numr = mysql_num_rows( res );
	if (numr == 0) { mysql_free_result(res); return 0; }

	message->lease_type = LEASE_REJECT;

	mysql_free_result(res);
	return 1;
}


int   my_findIP_CM( dhcp_message *message ) {
	char		qbuf[500];
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	struct in_addr	inp;
	int		numr;

	snprintf( qbuf, 500, "select ipaddr from docsis_update "
		"where modem_macaddr = '%s'", message->s_macaddr );
	SQL_QUERY_RET1( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) { return 1; }

	numr = mysql_num_rows( res );
	if (numr == 0) { mysql_free_result(res); return 1; }

	myrow = mysql_fetch_row( res );
	if (myrow == NULL) { mysql_free_result(res); return 1; }

	strncpy( message->s_ipaddr, (myrow)[0], 20 );
	inet_aton( (myrow)[0], &inp );
	message->ipaddr = inp.s_addr;
	message->b_ipaddr = ntohl( message->ipaddr );

	mysql_free_result(res);
	return 0;
}


int   my_findMAC_CM( dhcp_message *message ) {
	char		qbuf[150];
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	struct in_addr	inp;
	int		numr, retv;

	snprintf( qbuf, 150, "select cmts_vlan, subnum, config_file, "
		"static_ip, dynamic_ip, config_opt "
		"from docsis_modem where modem_macaddr = '%s'",
		message->s_macaddr );
	SQL_QUERY_RET0( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) { return 0; }

	numr = mysql_num_rows( res );
	if (numr == 0) { mysql_free_result(res); return 0; }

	myrow = mysql_fetch_row( res );
	if (myrow == NULL) { mysql_free_result(res); return 0; }

	message->lease_type = LEASE_CM;

	message->vlan        = strtoul( (myrow)[0], NULL, 10 );
	message->subnum      = strtoull( (myrow)[1], NULL, 10 );
	strncpy( message->cfname, (myrow)[2], MAX_CONFIG_FILE_NAME -1 );
	message->static_ip   = strtoul( (myrow)[3], NULL, 10 );
	message->dynamic_ip  = strtoul( (myrow)[4], NULL, 10 );
	message->opt         = strtoul( (myrow)[5], NULL, 10 );

	mysql_free_result(res);

	if ( my_findIP_CM(message) ) { // error finding IP for CM
		my_getNewIP_CM( message );
	} else {
		if ( Verify_Vlan(message) ) { // IP in wrong subnet
			my_Clear_MCACHE_Entry(message);
			my_getNewIP_CM( message );
		}
	}
	return 1;
}

int   my_findMAC_CMUNKNOWN( dhcp_message *message ) {
//THIS FUNKCTION IS INTEDED TO FIND A DEFAULT-BOOTFILE DEPENDING ON THE CABLEMODEMs MODEL-NAME
//THEREFOR AN ADDITIONAL DATABASE HAS BEEN CREATED.

//TODO: ADDING A FUNCTION FOR A TRUE DEFAULT-FILE IF NO OTHER FILE IS FOUND.

        char            qbuf[150];
        MYSQL_RES       *res;
        MYSQL_ROW       myrow;
        struct in_addr  inp;
        int             numr, retv;

	if(my_GetModemDefault() != 1) { return 0; } 	
	if ( message->in_opts.docsis_modem != 1 ) { return 0; }
	my_syslog(LOG_INFO, "searching for bootfile");

        snprintf( qbuf, 150, "select BOOTFILE from config_default where MODEL = '%s'",
                message->in_opts.vsi_model );
        SQL_QUERY_RET0( qbuf );
        res = mysql_store_result( my_sock );
        if (res == NULL) { return 0; }

        numr = mysql_num_rows( res );
        my_syslog(LOG_INFO, "found %d lines", numr) ;

        if (numr == 0) {	//	mysql_free_result(res); return 0; }
		mysql_free_result(res); 
        	snprintf( qbuf, 150, "select BOOTFILE from config_default where VENDOR = '%s'",
                message->in_opts.vsi_vendor );
        	SQL_QUERY_RET0( qbuf );
        	res = mysql_store_result( my_sock );
                numr = mysql_num_rows( res );
        
      		if (numr == 0) { 
			 mysql_free_result(res); 
			 strncpy( message->cfname, my_GetModemDefaultFile(), MAX_CONFIG_FILE_NAME -1 );
			 if ( strlen(message->cfname) == 1 ) { 
			 	my_syslog(LOG_INFO, "Default-Bootfile not set in configuration!");
			 	return 0; 
				}
			} else { 
        	 	 myrow = mysql_fetch_row( res );
        	 	 strncpy( message->cfname, (myrow)[0], MAX_CONFIG_FILE_NAME -1 );
        		 mysql_free_result(res);
			}
		}

	// if (res == NULL) { return 0; }

        //if (myrow == NULL) { mysql_free_result(res); return 0; }

        message->lease_type = LEASE_CM;

        message->vlan        = strtoul( "1", NULL, 10 );
        message->subnum      = strtoull( message->in_opts.vsi_serialno, NULL, 10 );
        message->static_ip   = strtoul( "0", NULL, 10 );
        message->dynamic_ip  = strtoul( "0", NULL, 10 );
        message->opt         = strtoul( "0", NULL, 10 );
        
	my_syslog(LOG_INFO, "selected %s for modem: %s model: %s vendor: %s ", message->cfname, message->s_macaddr,message->in_opts.vsi_model,message->in_opts.vsi_vendor );

        if ( my_findIP_CM(message) ) { // error finding IP for CM
                my_getNewIP_CM( message );
        } else {
                if ( Verify_Vlan(message) ) { // IP in wrong subnet
                        my_Clear_MCACHE_Entry(message);
                        my_getNewIP_CM( message );
                }
        }
        return 1;
}

int   my_findMAC_CPE( dhcp_message *message ) {
	char		qbuf[230];
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	struct in_addr	inp;
	int		numr;

	snprintf( qbuf, 230, "select ipaddr, dynamic_flag, lockip_flag, "
		"modem_macaddr, cmts_vlan, "
		"unix_timestamp(end_time) - unix_timestamp(), "
		"config_opt, config_file "
		"from dhcp_leases where macaddr = '%s' "
		"order by ipaddr limit 1", message->s_macaddr );
	SQL_QUERY_RET0( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) { return 0; }
	numr = mysql_num_rows( res );
	if (numr == 0) { mysql_free_result(res); return 0; }
	myrow = mysql_fetch_row( res );
	if (myrow == NULL) { mysql_free_result(res); return 0; }

	message->lease_type = LEASE_CPE;

	strncpy( message->s_ipaddr, (myrow)[0], 20 );
	inet_aton( (myrow)[0], &inp );
	message->ipaddr = inp.s_addr;
	message->b_ipaddr = ntohl( message->ipaddr );

	if (((myrow)[1][0] & 0x5F) == 'Y')
		message->cpe_type = CPE_DYNAMIC;

	if (((myrow)[2][0] & 0x5F) == 'Y')
		message->lockip = 1;

	if (message->b_modem_macaddr == 0) {
		strcpy( message->s_modem_macaddr, (myrow)[3] );
		cv_addrmac( message->modem_macaddr, message->s_modem_macaddr, 6 );
		cv_mac2num( &(message->b_modem_macaddr), message->modem_macaddr );
	}

	message->vlan = strtoul( (myrow)[4], NULL, 10 );
	message->lease_time = strtoull( (myrow)[5], NULL, 10 );
	message->opt = strtoul( (myrow)[6], NULL, 10 );
	if((myrow)[7] != NULL && (myrow)[7][0] != 0)
		strncpy( message->cfname, (myrow)[7], MAX_CONFIG_FILE_NAME -1 );

	mysql_free_result(res);

	my_findCM_MAC_CACHE( message );
	return 1;
}

int   my_findMAC_PRECPE( dhcp_message *message ) {
	char		qbuf[280];
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	struct in_addr	inp;
	int		numr, sn;
	u_int32_t	h_min, h_max;

	snprintf( qbuf, 280, "select dp.ipaddr, dp.modem_macaddr, "
		"dp.cmts_vlan, dp.config_file, dp.config_opt from "
		"dhcp_preleases as dp LEFT JOIN dhcp_leases as dl using (ipaddr) "
		"where dp.modem_macaddr = '%s' and "
		"(dl.end_time is NULL or dl.end_time < now() ) "
		"order by dp.ipaddr limit 1", message->in_opts.s_modem_mac );
	SQL_QUERY_RET0( qbuf );
	res = mysql_store_result( my_sock );
	if (res == NULL) { return 0; }
	numr = mysql_num_rows( res );
	if (numr == 0) { mysql_free_result(res); return 0; }
	myrow = mysql_fetch_row( res );
	if (myrow == NULL) { mysql_free_result(res); return 0; }

	message->lease_type = LEASE_CPE;

	strncpy( message->s_ipaddr, (myrow)[0], 20 );
	inet_aton( (myrow)[0], &inp );
	message->ipaddr = inp.s_addr;
	message->b_ipaddr = ntohl( message->ipaddr );

	if (message->b_modem_macaddr == 0) {
		strcpy( message->s_modem_macaddr, (myrow)[1] );
		cv_addrmac( message->modem_macaddr, message->s_modem_macaddr, 6 );
		cv_mac2num( &(message->b_modem_macaddr), message->modem_macaddr );
	}

	message->vlan = strtoul( (myrow)[2], NULL, 10 );
	if((myrow)[3] != NULL && (myrow)[3][0] != 0)
		strncpy( message->cfname, (myrow)[3], MAX_CONFIG_FILE_NAME -1 );
	message->opt = strtoul( (myrow)[4], NULL, 10 );

	mysql_free_result(res);

	my_findCM_MAC_CACHE( message );

	if (my_Get_Net( message ) ) return 0;

	my_WriteNewCPELease( message );
	return 1;
}
/* ****************************************************************** */
