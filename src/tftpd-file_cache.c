/* file_cache.c
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


extern int      tftp2cfgen[2];
extern int      cfgen2tftp[2];


/* file Cache */
struct file_block {
	u_int32_t	blknum;
	u_int32_t	blksize;

        short   	n_op;
	unsigned short  n_bnum;
	u_int8_t	blk[SEGSIZE +1];

	struct file_block  *next;
};

struct file_cache {
        u_int32_t       filenum;
        char            *fname;
	char		*dyn_fname;
	// u_int32_t	ipaddr;
	u_int32_t	num_blocks;
        u_int32_t       counter;
        time_t          readtime;
        time_t          statcheck;
        time_t          lastaccess;
        struct file_block *thefile;

	struct file_cache *next;
	struct file_cache *prev;
};

/* here is our cache */
struct file_cache	*thecache = NULL;
u_int32_t		file_number = 0;

/* private function prototypes */
u_int32_t  get_new_file( char * fname );
u_int32_t  get_new_dynamic_file( ModemSettings *msc );


/*********************************************************************/
void	delete_file( u_int32_t filenum ) {
	struct file_cache	*fc;
	struct file_block       *fb, *fb2;

	for (fc = thecache; fc; fc = fc->next ) {
		if (fc->filenum == filenum) { break; }
	}
	if (fc == NULL) { return; }

	fb = fc->thefile;
	while( fb != NULL ) {
		fb2 = fb->next;
		free( fb );
		fb = fb2;
	}
	if (fc == thecache) {
		thecache = fc->next;
		if (fc->next != NULL)
			fc->next->prev = NULL;
	} else {
		if (fc->prev != NULL)
			fc->prev->next = fc->next;
		if (fc->next != NULL)
			fc->next->prev = fc->prev;
	}
	if (fc->dyn_fname != NULL) free( fc->dyn_fname );
	if (fc->fname != NULL)     free( fc->fname );
	free( fc );
}


/*********************************************************************/

int	read_static_file( char *fname, u_int32_t *filenum, u_int32_t *num_blocks ) {
	time_t			thetime;
	struct file_cache	*p;
	struct stat		fst;
	u_int32_t		fnum;
	int			res;

	thetime = time(NULL);

		/* If cache is empty, read in requested file */
	if (thecache == NULL) {
		fnum = get_new_file( fname );
		if (fnum == 0) { return -1; }
		*filenum = fnum;
		*num_blocks = thecache->num_blocks;
		return 0;
	}

		/* Search through cache for requested file */
	for ( p = thecache; p; p = p->next ) {
		if (p->fname != NULL && strcmp(fname, p->fname) == 0) {
			p->statcheck = thetime;
			res = stat( p->fname, &fst );
			if (res != 0) {
				my_syslog( MLOG_TFTP, "file %s no longer exists. fnum %d (counter %d)",
					p->fname, p->filenum, p->counter );
				delete_file( p->filenum );
			}
			if (fst.st_mtime > p->readtime) {
				my_syslog( MLOG_TFTP, "updated file %s fnum %d (counter %d)",
					p->fname, p->filenum, p->counter );
				delete_file( p->filenum );
			} else {
				*filenum = p->filenum;
				*num_blocks = p->num_blocks;
				p->counter++;
				return 0;
			}
		}
	}

		/* could not find file in cache, read it in */
	fnum = get_new_file( fname );
	if (fnum == 0) { return -1; }
	*filenum = fnum;
	*num_blocks = 1;
	for ( p = thecache; p; p = p->next ) {
		if (fnum == p->filenum) {
			*num_blocks = p->num_blocks;
			return 0;
		}
	}
	return 0;
}

/*********************************************************************/

int	read_dynamic_file( u_int32_t ipaddr, u_int32_t *filenum, u_int32_t *num_blocks ) {
	time_t			thetime;
	struct file_cache	*p;
	u_int32_t		fnum;
	ModemSettings		*msc;

	thetime = time(NULL);

	msc = my_findModem_Settings( ipaddr );
	if (msc == NULL) return 0;

		/* If cache is empty, read in requested file */
	if (thecache == NULL) {
		my_Check_for_New_Configs();		// prep check_time so we can flag config changes
		fnum = get_new_dynamic_file( msc );
		if (fnum == 0) { return 0; }
		*filenum = fnum;
		*num_blocks = thecache->num_blocks;
		my_TFTP_dynamic_log( msc );
		return 1;
	}

		/* Search through cache for requested file */
	for ( p = thecache; p; p = p->next ) {
		if (p->dyn_fname != NULL && strcmp(msc->dynamic_file, p->dyn_fname) == 0) {
			*filenum = p->filenum;
			*num_blocks = p->num_blocks;
			p->counter++;
			my_TFTP_dynamic_log( msc );
			return 1;
		}
	}

		/* could not find file in cache, read it in */
	fnum = get_new_dynamic_file( msc );
	if (fnum == 0) { return 0; }
	*filenum = fnum;
	*num_blocks = 1;
	my_TFTP_dynamic_log( msc );
	for ( p = thecache; p; p = p->next ) {
		if (fnum == p->filenum) {
			*num_blocks = p->num_blocks;
			return 1;
		}
	}
	return 1;
}

/*********************************************************************/


int	read_block( u_int32_t filenum, u_int32_t blocknum, struct tftphdr **dpp, u_int32_t *bytes ) {
	struct file_cache	*fc;
	struct file_block	*fb;

	for (fc = thecache; fc; fc = fc->next ) {
		if (fc->filenum == filenum) { break; }
	}
	if (fc == NULL) { return -1; }

	fc->lastaccess = time(NULL);

	for (fb = fc->thefile; fb; fb = fb->next ) {
		if (fb->blknum == blocknum) {
			*bytes = fb->blksize;
			*dpp = (struct tftphdr *) &(fb->n_op);
			return 0;
		}
	}

	return -1;
}


/*********************************************************************/
void	free_file_cache( void ) {
	time_t			thetime;
	struct file_cache	*fc;
	struct stat		fst;
	int			res;
	static char		*cachetime = NULL;
	static int		cache_time = 0;
	static char		*dynamtime = NULL;
	static int		dynam_time = 0;

	if (cachetime == NULL) {
		cachetime = GetConfigVar( "tftp-cache-time" );
		if (*cachetime != ' ') {
			cache_time = strtoull( cachetime, NULL, 10 );
		} else {
			cache_time = 3600;	// default to 1 hour
		}
	}

	if (dynamtime == NULL) {
		dynamtime = GetConfigVar( "tftp-dynamic-cache-time" );
		if (*dynamtime != ' ') {
			dynam_time = strtoul( dynamtime, NULL, 10 );
		} else {
			dynam_time = 3600;	// default to 1 hour
		}
	}

	thetime = time(NULL);

	if (thecache == NULL) return;

	if (my_Check_for_New_Configs() == 0) {
		int zz = 0;
		for (fc = thecache; fc != NULL; fc = fc->next ) {
			if (fc->dyn_fname != NULL) {
				zz++;
				my_syslog( MLOG_TFTP, "dynamic file update %s fnum %d (counter %d)",
					fc->dyn_fname, fc->filenum, fc->counter );
				delete_file( fc->filenum );
				fc = thecache;
				if (fc == NULL) return;
			}
		}
		my_syslog( MLOG_TFTP, "dropping %d files because of dynamic update", zz );
	}

	for (fc = thecache; fc != NULL; fc = fc->next ) {
		if (fc->fname != NULL) {
			// if file hasnt been access for 3 minutes
			// check to see if has been updated
			if ( (thetime - fc->statcheck) > 180) {
				fc->statcheck = thetime;
				res = stat( fc->fname, &fst );
				if (res != 0) {
					my_syslog( MLOG_TFTP, "file %s no longer exists. fnum %d (counter %d)",
						fc->fname, fc->filenum, fc->counter );
					delete_file( fc->filenum );
					fc = thecache;
					if (fc == NULL) return;
					continue;
				}
				if (fst.st_mtime > fc->readtime) {
					my_syslog( MLOG_TFTP, "updated file %s fnum %d (counter %d)",
						fc->fname, fc->filenum, fc->counter );
					delete_file( fc->filenum );
					fc = thecache;
					if (fc == NULL) return;
					continue;
				}
			}
			// if file hasnt been accessed since 'cache_time'
			// remove it from the cache
			if ((thetime - fc->lastaccess) > cache_time) {
				my_syslog( MLOG_TFTP, "dropping file %s fnum %d (counter %d)",
					fc->fname, fc->filenum, fc->counter );
				delete_file( fc->filenum );
				fc = thecache;
				if (fc == NULL) return;
			}
		} else {
			// a dynamic file can be cached a max of 'dynam_time'
			// remove it from the cache
			if ((thetime - fc->readtime) > dynam_time) {
				my_syslog( MLOG_TFTP, "dropping file %s fnum %d (counter %d)",
					fc->fname, fc->filenum, fc->counter );
				delete_file( fc->filenum );
				fc = thecache;
				if (fc == NULL) return;
			}
		}
	}
}

/*********************************************************************/

u_int32_t  get_new_file( char * fname ) {
	struct file_cache	*p, *fc;
	FILE			*fin;
	int			bytes, reading, blknum;
	struct file_block	*fb, *fb2;

	p = thecache;
	if (p != NULL) {
		while( p->next != NULL) { p = p->next; }
	}

	file_number++;

	fc = calloc(1, sizeof( struct file_cache ) +2);
	if (fc == NULL) { return 0; }

	fc->filenum = file_number;
	fc->fname = calloc(1, strlen(fname) +1);
	strcpy( fc->fname, fname );
	fc->counter = 1;
	fc->num_blocks = 0;
	fc->readtime = time(NULL);
	fc->statcheck = fc->readtime;
	fc->lastaccess = fc->readtime;
	fc->thefile = NULL;
	fc->next = NULL;
	fc->prev = p;
	fb2 = NULL;

	fin = fopen( fname, "r");
	if (fin == NULL) {  free(fc);  return 0;  }

	blknum = 0;
	reading = 1;
	while (reading) {
		blknum++;
		fb = calloc(1, sizeof( struct file_block ) +2);
		if (fb == NULL) {
			fb = fc->thefile;
			while( fb != NULL ) {
				fb2 = fb->next;
				free( fb );
				fb = fb2;
			}
			free( fc );
			fclose( fin );
			return 0;
		}
		if (fc->thefile == NULL) { fc->thefile = fb; }

		memset( fb->blk, 0, SEGSIZE );
		fb->n_op = htons( (u_short) DATA );
		fb->n_bnum = htons( (u_short) blknum );

		fb->blknum = blknum;
		bytes = fread( fb->blk, 1, SEGSIZE, fin );

		fb->blksize = bytes + 4;
		if (bytes < SEGSIZE) {
			reading = 0;
		}

		if (fb2 != NULL) { fb2->next = fb; }
		fb2 = fb;
	}
	fc->num_blocks = blknum;
	my_syslog( MLOG_TFTP, "file %s fnum %d - blocks read %d", fname, fc->filenum, blknum );
	fclose( fin );
	if (p == NULL) {
		thecache = fc;
	} else {
		p->next = fc;
	}
	return fc->filenum;
}



/*********************************************************************/

u_int32_t  get_new_dynamic_file( ModemSettings *msc ) {
	struct file_cache	*p, *fc;
	int			reading, blknum;
	struct file_block	*fb, *fb2;
	static u_int8_t		*tmp_buf = NULL;
	static char		*dynamic_debug_pref;
	u_int32_t		size, bytes, ipaddr2;
	struct in_addr		s_ip;

	if (dynamic_debug_pref == NULL) {
		dynamic_debug_pref = GetConfigVar( "dynamic-config-debug" );
	}

	p = thecache;
	if (p != NULL) {
		while( p->next != NULL) { p = p->next; }
	}

	file_number++;

	fc = calloc(1, sizeof( struct file_cache ) +2);
	if (fc == NULL) { return 0; }

	fc->filenum = file_number;
	fc->dyn_fname = calloc(1, strlen(msc->dynamic_file) +1 );
	strcpy( fc->dyn_fname, msc->dynamic_file );
	fc->counter = 1;
	fc->num_blocks = 0;
	fc->readtime = time(NULL);
	fc->statcheck = fc->readtime;
	fc->lastaccess = fc->readtime;
	fc->thefile = NULL;
	fc->next = NULL;
	fc->prev = p;
	fb2 = NULL;

	if (tmp_buf == NULL) 
		tmp_buf = (u_int8_t *) calloc(1,20000);
	size = gen_Config_File( msc->ipaddr, tmp_buf, 20000 );
	if (size == 0) return 0;

	if (*dynamic_debug_pref == 'y') {
		int		ofd;
		char		outfile[350];
		sprintf( outfile, "dynamic/dynamic--%s.cfg", msc->dynamic_file );
		if ( (ofd = open( outfile, O_CREAT | O_WRONLY, 0644 ))==-1 )  {
			fprintf(stderr,"Error opening file %s: %s",outfile,strerror(errno));
		}
		write( ofd, tmp_buf, size );
		close( ofd );
	}

	// write( tftp2cfgen[1], &ipaddr, 4 );

	// bytes = read( cfgen2tftp[0], &size, 4 );
	// if (bytes != 4) return 0;
	// if (size == 0)  return 0;

	// buf = alloca( size );
	// bytes = read( cfgen2tftp[0], buf, size );
	// if (bytes != size) return 0;

	bytes = 0;
	blknum = 0;
	reading = 1;
	while (reading) {
		blknum++;
		fb = calloc(1, sizeof( struct file_block ) +2);
		if (fb == NULL) {
			fb = fc->thefile;
			while( fb != NULL ) {
				fb2 = fb->next;
				free( fb );
				fb = fb2;
			}
			free( fc );
			return 0;
		}
		if (fc->thefile == NULL) { fc->thefile = fb; }

		memset( fb->blk, 0, SEGSIZE );
		fb->n_op = htons( (u_short) DATA );
		fb->n_bnum = htons( (u_short) blknum );

		fb->blknum = blknum;
		if ( size - bytes > SEGSIZE ) {
			memcpy( fb->blk, tmp_buf + bytes, SEGSIZE );
			fb->blksize = SEGSIZE + 4;
			bytes += SEGSIZE;
		} else {
			memcpy( fb->blk, tmp_buf + bytes, size - bytes );
			fb->blksize = size - bytes + 4;
			reading = 0;
		}

		if (fb2 != NULL) { fb2->next = fb; }
		fb2 = fb;
	}
	fc->num_blocks = blknum;
	// my_syslog( MLOG_TFTP, "file %s fnum %d - blocks read %d", fc->fname, fc->filenum, blknum );
	if (p == NULL) {
		thecache = fc;
	} else {
		p->next = fc;
	}
	return fc->filenum;
}

