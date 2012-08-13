/***************************************************************************
 *   Copyright (C) 2006 by Docsis Guy                                      *
 *   docsis_guy@accesscomm.ca                                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "docsis_server.h"


void get_date_time( char **the_date, char **the_time ) {
	time_t		t1;
	struct tm	*t2;
	static char	*date_str = NULL;
	static char	*time_str = NULL;
	static int	day_of_month = 0;

	if (date_str == NULL)
		date_str = (char *) calloc(1,80);
	if (time_str == NULL)
		time_str = (char *) calloc(1,80);

	t1 = time(NULL);
	t2 = localtime( &t1 );
	
	if (t2->tm_mday != day_of_month) {
		day_of_month = t2->tm_mday;
		sprintf(date_str,"%d-%d-%d",
			(1900 + t2->tm_year), (1 + t2->tm_mon), t2->tm_mday );
	}

	sprintf(time_str,"%d:%d:%d", t2->tm_hour, t2->tm_min, t2->tm_sec );

	*the_date = date_str;
	*the_time = time_str;
}


