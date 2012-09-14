/* util_configfile.c
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


/* Global Variables */
typedef struct config_list {
        char    *key;
        char    *data;

        struct config_list  *next;
} ConfigList;
ConfigList	my_list;


/*  Private Function Prototypes */
int  get_next_line( FILE *fin, char buf[] );
int  get_line( FILE *fin, char buf2[] );
void lc( char * );

/* ***********************************************************************
   *********************************************************************** */

/* Read the docsis config file and parse all the key-value items */
void	ReadConfigFile( int config_print_flag ) {
	FILE	*fin;
	char	line_buf[512];
	char	*ck, *cd, *tc;
	ConfigList *xlist, *oldx;

	if ((fin = fopen(CONFDIR "/docsis-server3.conf", "r")) == NULL) {
		if ((fin = fopen(CONFDIR "/docsis-server.conf", "r")) == NULL) {
			fprintf(stderr, "Cannot open " CONFDIR "/docsis-server.conf file!\n" );
			exit(1);
		}
	}

	xlist = &my_list;
	while( get_next_line(fin, line_buf) != EOF ) {
		ck = line_buf;
		tc = line_buf;
		while( *tc != 32 && *tc != 9 && *tc != 0 ) tc++;
		*tc = 0; tc++;
		while ( *tc == 32 || *tc == 9 ) tc++;

		cd = tc;
		xlist->key = calloc(1, strlen( ck ) + 2 );
		lc( ck );
		strcpy( xlist->key, ck );
		xlist->data = calloc(1, strlen( cd ) + 2 );
		strcpy( xlist->data, cd );
		lc( cd );
		if (!strcmp(cd,"yes")) strcpy( xlist->data, cd );
		if (!strcmp(cd,"y"))   strcpy( xlist->data, cd );
		if (!strcmp(cd,"no"))  strcpy( xlist->data, cd );
		if (!strcmp(cd,"n"))   strcpy( xlist->data, cd );
		if (config_print_flag)
			fprintf(stderr, "Config Item '%s' = '%s'\n", xlist->key, xlist->data );
		xlist->next = calloc(1, sizeof( ConfigList ) +2);
		oldx = xlist;
		xlist = xlist->next;
	}
	if (xlist != &my_list) {
		oldx->next = 0;
		free( xlist );
	}
	fclose( fin );
}

/* The user passes a key and we pass back the return value or a space */
char * GetConfigVar( char *key ) {
	ConfigList	*xlist;
	char		lckey[1024];
	static char	*tmpc = NULL;

	strncpy( lckey, key, 1023 );
	lc( lckey );

	for( xlist = &my_list; xlist; xlist = xlist->next ) {
		if (strcmp( xlist->key, lckey ) == 0) {
			return( xlist->data );
		}
	}

	if (tmpc == NULL) {
		tmpc = (char *) calloc(1,5);
		strcpy( tmpc, " " );
	}
	return( tmpc );
}


int * my_GetModemDefault( void ) {
	char	*retval;
	retval = GetConfigVar( "ModemDefault" );
	if (*retval == 'y') return 1;
	return 0;
}

char * my_GetModemDefaultFile( void ) {
	return( GetConfigVar( "ModemDefaultFile" ) );
}

char * my_GetTFTPdir( void ) {
	return( GetConfigVar( "tftp-dir" ) );
}

char * my_GetDHCPint( void ) {
	return( GetConfigVar( "dhcp-interface" ) );
}

int my_CheckActiveService( char *service ) {
	char	*retval, sname[100];

	strcpy( sname, "activate-" );
	strcat( sname, service );
	retval = GetConfigVar( sname );
	if (*retval == 'y') return 1;
	return 0;
}

/* ***********************************************************************
   *********************************************************************** */
/* Private Functions */

int	get_next_line( FILE *fin, char buf[] ) {
	int c;

	while ((c = get_line(fin, buf)) != EOF &&
		(buf[0] == '#' || buf[0] == '\0'));

	return( c );
}

int	get_line( FILE *fin, char buf2[] ) {
	int c, i;

	i = 0;
	while ((c=fgetc(fin)) != EOF && c != '\n' && i < 500)
		buf2[i++] = c;

	buf2[i] = '\0';
	return( c );
}

/* Change buffer to lower case */
void   lc( char * buf ) {
	int c;

	for(c = 0; c < strlen(buf); c++ )
		if (buf[c] >= 65 && buf[c] <= 90) buf[c] += 32;
}

