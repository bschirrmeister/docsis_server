/* options.h
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

#ifndef _OPTIONS_H
#define _OPTIONS_H


void  initOpt( dhcp_message *message );
void  addOpt( unsigned char code, unsigned char datalength, char *data );
void  addBigOpt( ConfigOpts *opt );

void DecodeOptions( dhcp_message *message );

// int Check_Requested_Options ( dhcp_message *message );

#endif /* _OPTIONS_H */

