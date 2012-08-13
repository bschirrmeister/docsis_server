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


void cv_mac2num( u_int64_t *b_mac, u_int8_t *chaddr ) {
	u_int64_t	tmpmac;
	int		n;

	tmpmac = 0;
	for(n=0; n<6; n++) {
		tmpmac = tmpmac << 8;
		tmpmac += *(chaddr + n);
	}
	*b_mac = tmpmac;
}

void cv_macaddr( u_int8_t *chaddr, char *macaddr, int numbytes ) {
	int n, n1, n2;
	char	*p = macaddr;

	if (chaddr == NULL) { return; }
	if (macaddr == NULL) { return; }
	if (numbytes == 0) { return; }

	for(n=0; n<numbytes; n++) {
		n1 = (*(chaddr + n) & 0xf0) >> 4;
		n2 = *(chaddr + n) & 0x0f;

		if (n1 >= 10) {
			*p = n1 + 'a' - 10;
		} else {
			*p = n1 + '0';
		}
		p++;

		if (n2 >= 10) {
			*p = n2 + 'a' - 10;
		} else {
			*p = n2 + '0';
		}
		p++;
	}
	*p = 0;
}

void cv_addrmac( u_int8_t *chaddr, char *macaddr, int numbytes ) {
	int 		n, n1, n2, c, er;
	u_int8_t	*p;

	if (chaddr == NULL) { return; }
	if (macaddr == NULL) { return; }
	if (numbytes == 0) { return; }

	n1 = 0;
	er = 0;
	p = chaddr;
	*p = 0;
	for(n=0; n < (numbytes * 2); n++) {
		if (n1 == 0) { n2 = 16; } else { n2 = 1; }

		c = *(macaddr + n);
		if (c >= 48 && c <= 57) {
			*p += n2 * (c - 48);
		} else if (c >= 65 && c <= 70) {
			*p += n2 * (c - 55);
		} else if (c >= 97 && c <= 102) {
			*p += n2 * (c - 87);
		} else if ((c >= 58 && c <= 64) ||
			   (c >= 71 && c <= 96) ||
			   c >= 103 || c <= 47) er++;

		if (er >= 3) {
			p = chaddr;
			p[0]=0; p[1]=0; p[2]=0;
			p[3]=0; p[4]=0; p[5]=0;
			return;
		}
		if (n1 == 0) {
			n1 = 1;
		} else {
			n1 = 0;
			p++;
			*p = 0;
		}
	}
}
