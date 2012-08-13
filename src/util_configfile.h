/* util_configfile.h
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

#ifndef _UTIL_CONFIGFILE_H
#define _UTIL_CONFIGFILE_H


/* Read the docsis_server config file */
void    ReadConfigFile( int config_print_flag );

/* Check for a Config Key */
char * GetConfigVar( char *key );

/* Check config file to see if service is active */
int my_CheckActiveService( char *service );

/* Find out what interface the user wants the dhcp to listen to */
char * my_GetDHCPint( void );

/* Get the directory with all the tftp files */
char * my_GetTFTPdir( void );

#endif /* _UTIL_CONFIGFILE_H */

