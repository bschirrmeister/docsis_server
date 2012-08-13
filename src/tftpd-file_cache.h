/* file_cache.h
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

#ifndef _FILE_CACHE_H
#define _FILE_CACHE_H


int	read_static_file( char *fname, u_int32_t *filenum, u_int32_t *num_blocks );
int	read_dynamic_file( u_int32_t ipaddr, u_int32_t *filenum, u_int32_t *num_blocks );

int	read_block( u_int32_t filenum, u_int32_t blocknum, struct tftphdr **dpp, u_int32_t *bytes );
void	free_file_cache( void );


#endif /* _FILE_CACHE_H */

