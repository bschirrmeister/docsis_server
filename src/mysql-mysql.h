/* mysql-mysql.h
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

#ifndef _MY_SQL_H
#define _MY_SQL_H


typedef struct sql_buf {
	char		*qbuf;
	int		len;
	struct sql_buf	*next;
} SQL_buf;

typedef struct modem_settings {
	u_int32_t	ipaddr;
	int		version_flag;
	int		num_dyn;
	u_int32_t	*dynamic;
        int		static_ip;
	int		dynamic_ip;
	char		*config_file;
	char		*dynamic_file;
} ModemSettings;


/**********************************************************************/
/* define a few constants */

/* #define	MAC_NOT_FOUND	0
   #define DOCSIS_MODEM	1
   #define CUST_PC		2
   #define REJECT_DEV	3
*/

#define MLOG_ERROR  1
#define MLOG_DHCP   3
#define MLOG_TFTP   4
#define MLOG_TIME   5
#define MLOG_LOG    6
#define MLOG_SYSLOG 6
#define MLOG_STATS  99

#define NET_TYPE_CM   1
#define NET_TYPE_CPE  2
#define NET_TYPE_MTA  3

#define OPT_IP     1
#define OPT_2IP    2
#define OPT_INT8   3
#define OPT_UINT8  4
#define OPT_INT16  5
#define OPT_UINT16 6
#define OPT_INT32  7
#define OPT_UINT32 8
#define OPT_CHAR   9
#define OPT_SUBOPT 10
#define OPT_HEX    11
#define OPT_MTAIP  12
#define OPT_MTAR   13

#define CHECK_LOAD_NO 0
#define CHECK_LOAD_YES 1

#define FULLFLAG_seton    1
#define FULLFLAG_setoff   2
#define FULLFLAG_clearall 3

#define DHCP_LOG_TYPE		1
#define TFTP_LOG_TYPE		2
#define TFTP_DYNAMIC_LOG_TYPE	3
#define SYS_LOG_TYPE		4

#define Modem_Ver_ANY		1
#define Modem_Ver_v1_0		2
#define Modem_Ver_v1_1		3
#define Modem_Ver_v2_0		4


/*******************************************************************************************/
/*******************************************************************************************/
//	fprintf(stderr,"qv %d %60s\n", strlen(query), query );		\

int my_SQL_Ping( void );
#define SQL_QUERY_RET(query)						\
	if ( my_SQL_Ping() ) return;					\
	if ( mysql_query( my_sock, query ) ) {				\
		fprintf( stderr, "sql error\nq=%s \ne=%s\n", query,	\
			mysql_error( my_sock ) ); return; }

#define SQL_QUERY_RETNULL(query)					\
	if ( my_SQL_Ping() ) return NULL;				\
	if ( mysql_query( my_sock, query ) ) {				\
		fprintf( stderr, "sql error\nq=%s \ne=%s\n", query,	\
			mysql_error( my_sock ) ); return NULL; }

#define SQL_QUERY_RET0(query)						\
	if ( my_SQL_Ping() ) return 0;					\
	if ( mysql_query( my_sock, query ) ) {				\
		fprintf( stderr, "sql error\nq=%s \ne=%s\n", query,	\
			mysql_error( my_sock ) ); return 0; }

#define SQL_QUERY_RET1(query)						\
	if ( my_SQL_Ping() ) return 1;					\
	if ( mysql_query( my_sock, query ) ) {				\
		fprintf( stderr, "sql error\nq=%s \ne=%s\n", query,	\
			mysql_error( my_sock ) ); return 1; }

#define SQL_QUERY_RET1N(query)						\
	if ( my_SQL_Ping() ) return -1;					\
	if ( mysql_query( my_sock, query ) ) {				\
		fprintf( stderr, "sql error\nq=%s \ne=%s\n", query,	\
			mysql_error( my_sock ) ); return -1; }


/*******************************************************************************************/
/*******************************************************************************************/
// Super FLAG
#define SFG				Super_Flag
#define Query_SFG( bit )		SFG & bit
#define SET_SFG( bit )			{ SFG = SFG | bit; }
#define CLR_SFG( bit )			{ SFG = SFG - (SFG & bit); }
#define Primary_SQL_Failure_Flag	Query_SFG( 0x01 )
#define SET_Primary_SQL_Failure_Flag	SET_SFG( 0x01 )
#define CLR_Primary_SQL_Failure_Flag	CLR_SFG( 0x01 )
#define High_Load_Flag			Query_SFG( 0x02 )
#define SET_High_Load_Flag		SET_SFG( 0x02 )
#define CLR_High_Load_Flag		CLR_SFG( 0x02 )


/*******************************************************************************************/
/*******************************************************************************************/
// mysql-cache_dhcp_conf.c
void  my_Clear_MCACHE( void );
void  my_Clear_MCACHE_Entry( dhcp_message *message );
void  my_MCACHE_Load( void );
int  ignore_repeated_customers( dhcp_message *message );
int  my_findMAC_INDEX( u_int64_t b_mac );
int   my_findMAC_CACHE( dhcp_message *message );
void  my_findCM_MAC_CACHE( dhcp_message *message );

/*******************************************************************************************/
// mysql-cache_tftp_bin.c
void  Clear_Config_Cache( void );
int   my_Check_for_New_Configs( void );
void  my_CONFIG_Load( void );
int  add_Config_Item( u_int8_t *dst, u_int32_t idnum, u_int32_t maxsize, u_int8_t modver );

/*******************************************************************************************/
// mysql-cache_tftp_conf.c
void  my_Modem_Settings_Clear( void );
void  my_Modem_Settings_Load( void );
int  my_findModem_INDEX( u_int32_t ipaddr );
ModemSettings  *my_findModem_Settings( u_int32_t ipaddr );

/*******************************************************************************************/
// mysql-config_nets.c
void  my_Clear_Nets( void );
ConfigNets *Get_Net_Opts( int netid );
void  my_Load_Nets( void );
int  my_Get_Net( dhcp_message *message );
int Verify_Vlan( dhcp_message *message );
void Update_Network_Full_Flag( int action_flag, int value );
void  my_CountUnusedIP( int *unused, int *total, u_int16_t cmts_vlan, u_int32_t cmts_ip );

/*******************************************************************************************/
// mysql-config_opts.c
void  my_Clear_Opts( void );
int   Query_Opt_CACHE( u_int16_t opt_id, u_int64_t b_mac );
void  my_Load_Opts( void );
int   Convert_Opt( u_int8_t *outbuf, u_int8_t type, u_int8_t dtype, char *value, u_int64_t b_mac );
int   Query_Opt( ConfigOpts *cfo, u_int16_t opt_id, u_int64_t b_mac );
int   find_Opts_Mac( u_int64_t b_macaddr );
ConfigOpts *my_Get_Opt( u_int16_t opt_id, u_int64_t b_mac );

/*******************************************************************************************/
// mysql-findip.c
int   my_findIP_QUERY( char *qbuf, dhcp_message *message );
int   my_findIP( dhcp_message *message, int find_type );

/*******************************************************************************************/
// mysql-findmac.c
int   my_findMAC( dhcp_message *message );
int   my_findBADMAC( dhcp_message *message );
int   my_findIP_CM( dhcp_message *message );
int   my_findMAC_CM( dhcp_message *message );
int   my_findMAC_CMUNKNOWN( dhcp_message *message );
int   my_findMAC_CPE( dhcp_message *message );
int   my_findMAC_PRECPE( dhcp_message *message );

/*******************************************************************************************/
// mysql-misc_lease.c
void  my_CountOLDIP( u_int16_t cmts_vlan, int *ip45, int *ip30, int *ip15 );
void  my_DeleteOldestLease( u_int16_t cmts_vlan, int days );
void  Print_Log_Entries( int log_type, int num_lines );

/*******************************************************************************************/
// mysql-mysql.c
void  Init_Primary_SQL( void );
void  Init_Backup_SQL( void );
void  Init_PowerDNS_SQL( void );
void  InitSQLconnection();
void  CloseSQLconnection();
//int   my_SQL_Ping( void );
void  my_Check_Load( int high_load );
void  my_LoadChecker( void );

/*******************************************************************************************/
// mysql-newip.c
void my_getNewIP_CM( dhcp_message *message );
void my_getNewIP_CPE( dhcp_message *message );
int my_CheckCPE_IP( u_int32_t ipaddr );
int my_CheckCM_IP( u_int32_t ipaddr );
void my_Count_Allowed_IPs( dhcp_message *message );
void my_Count_Used_IPs( dhcp_message *message );

/*******************************************************************************************/
// mysql-remote_cmds.c
void  Clear_Remote_Commands( void );
void  Check_Remote_Commands( void );

/*******************************************************************************************/
// mysql-summerize.c
void  Log_Summerize( void );

/*******************************************************************************************/
// mysql-syslog.c
void  my_syslog( const int priority, const char * fmt, ... );
void  my_dhcplog( int sending, dhcp_message *message );
void  my_LogSyslog( u_int32_t ipaddr, char * message );

/*******************************************************************************************/
// mysql-tftp_filegen1.c
void  my_Check_New_Configs( char *config_encoder );
void  encode_config_morsel( char *config_encoder, char *id, char *text, char *ver );

/*******************************************************************************************/
// mysql-tftp_filegen2.c
int  gen_Config_File( u_int32_t ipaddr, u_int8_t *dst, u_int32_t maxsize );
void  Config_StressTest( void );

/*******************************************************************************************/
// mysql-tftp_log.c
void my_TFTPlog( char * fname, u_int32_t ipaddr );
void my_TFTP_dynamic_log( ModemSettings *msc );

/*******************************************************************************************/
// mysql-update_leases.c
char  *Lookup_ConfigOptsMacs( dhcp_message *message, int opt_type );
char  *Lookup_ConfigOpts( dhcp_message *message, int opt_type );
void  my_Update_PowerDNS( dhcp_message *message );
void  my_WriteNewCPELease( dhcp_message *message );
void  my_UpdateLease( dhcp_message *message );
void  my_UpdateTime( dhcp_message *message );
void  my_DeleteLease( dhcp_message *message );

/*******************************************************************************************/
// mysql-update_modems.c
void  my_WriteNewCMLease( dhcp_message *message );
void  my_UpdateAgent( dhcp_message *message );
int   my_findAgent( dhcp_message *message );
int  my_findAgent_QUERY( char *qbuf, dhcp_message *message );

/*******************************************************************************************/
// mysql-utilities.c
void  my_buffer_save( char *qbuf );
void  Flush_ALL_SQL_Updates( void );
void  Flush_SQL_Updates( void );
void  Save_SQL_Update( char *pref, char *qbuf, int checkLoad );

/*******************************************************************************************/
/*******************************************************************************************/

#endif /* _MY_SQL_H */

