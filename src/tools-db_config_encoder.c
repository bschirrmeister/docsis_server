/* db_config_encoder.c
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


// Global Variables


// Prototypes


int main( void ) {
	char	*encoder_pref = NULL;

	ReadConfigFile( 0 );
	InitSQLconnection();

	encoder_pref = GetConfigVar( "config-encoder" );
	if (CheckExecutable( encoder_pref ) == 0) {
		printf("could not execute '%s'", encoder_pref );
		exit(1);
	}

	my_Check_New_Configs( encoder_pref );
	return 0;
}


