/*************************************************************************//**
 *****************************************************************************
 * @file   misc.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   2008
 *****************************************************************************
 *****************************************************************************/

/* Orange'S FS */

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"
#include "hd.h"
#include "fs.h"

/*****************************************************************************
 *                                do_stat
 *************************************************************************//**
 * Perform the stat() syscall.
 * 
 * @return  On success, zero is returned. On error, -1 is returned.
 *****************************************************************************/
PUBLIC int do_stat()
{
	char pathname[MAX_PATH]; /* parameter from the caller */
	char filename[MAX_PATH]; /* directory has been stipped */

	/* get parameters from the message */
	int name_len = fs_msg.NAME_LEN;	/* length of filename */
	int src = fs_msg.source;	/* caller proc nr. */
	assert(name_len < MAX_PATH);
	phys_copy((void*)va2la(TASK_FS, pathname),    /* to   */
		  (void*)va2la(src, fs_msg.PATHNAME), /* from */
		  name_len);
	pathname[name_len] = 0;	/* terminate the string */

	int inode_nr = search_file(pathname);
	if (inode_nr == INVALID_INODE) {	/* file not found */
		printl("{FS} FS::do_stat():: search_file() returns "
		       "invalid inode: %s\n", pathname);
		return -1;
	}

	struct inode * pin = 0;

	struct inode * dir_inode;
	if (strip_path_dir(filename, pathname, &dir_inode) != 0) {
		/* theoretically never fail here
		 * (it would have failed earlier when
		 *  search_file() was called)
		 */
		assert(0);
	}
	pin = get_inode(dir_inode->i_dev, inode_nr);

	struct stat s;		/* the thing requested */
	s.st_dev = pin->i_dev;
	s.st_ino = pin->i_num;
	s.st_mode= pin->i_mode;
	s.st_rdev= is_special(pin->i_mode) ? pin->i_start_sect : NO_DEV;
	s.st_size= pin->i_size;

	put_inode(pin);

	phys_copy((void*)va2la(src, fs_msg.BUF), /* to   */
		  (void*)va2la(TASK_FS, &s),	 /* from */
		  sizeof(struct stat));

	return 0;
}

/*****************************************************************************
 *                                search_file
 *****************************************************************************/
/**
 * Search the file and return the inode_nr.
 *
 * @param[in] path The full path of the file to search.
 * @return         Ptr to the i-node of the file if successful, otherwise zero.
 * 
 * @see open()
 * @see do_open()
 *****************************************************************************/
PUBLIC int search_file(char * path)
{
	int i, j;
	char filename[MAX_PATH];
	memset(filename, 0, MAX_FILENAME_LEN);
	struct inode * dir_inode;
	//printl("path:%s\n",path);
	if (strip_path_dir(filename, path, &dir_inode) != 0)
		return 0;
	//printl("search filename is %s\n",filename);
	//printl("father:%d\n", dir_inode->i_num);
	//printl("root:%d\n", root_inode->i_num);
	if (filename[0] == 0)	/* path: "/" */
		return dir_inode->i_num;

	/**
	 * Search the dir for the file.
	 */
	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
	int nr_dir_entries =
	  dir_inode->i_size / DIR_ENTRY_SIZE; /**
					       * including unused slots
					       * (the file has been deleted
					       * but the slot is still there)
					       */
	int m = 0;
	struct dir_entry * pde;
	for (i = 0; i < nr_dir_blks; i++) {
		RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
			//printl("searched name is:%s\n",pde->name);
			if (memcmp(filename, pde->name, MAX_FILENAME_LEN) == 0)
			{
				//printl("find file name is :%s\n",pde->name);
				return pde->inode_nr;
			}
			if (++m > nr_dir_entries)
				break;
		}
		if (m > nr_dir_entries) /* all entries have been iterated */
			break;
	}

	/* file not found */
	return 0;
}

/*****************************************************************************
 *                                strip_path
 *****************************************************************************/
/**
 * Get the basename from the fullpath.
 *
 * In Orange'S FS v1.0, all files are stored in the root directory.
 * There is no sub-folder thing.
 *
 * This routine should be called at the very beginning of file operations
 * such as open(), read() and write(). It accepts the full path and returns
 * two things: the basename and a ptr of the root dir's i-node.
 *
 * e.g. After stip_path(filename, "/blah", ppinode) finishes, we get:
 *      - filename: "blah"
 *      - *ppinode: root_inode
 *      - ret val:  0 (successful)
 *
 * Currently an acceptable pathname should begin with at most one `/'
 * preceding a filename.
 *
 * Filenames may contain any character except '/' and '\\0'.
 *
 * @param[out] filename The string for the result.
 * @param[in]  pathname The full pathname.
 * @param[out] ppinode  The ptr of the dir's inode will be stored here.
 * 
 * @return Zero if success, otherwise the pathname is not valid.
 *****************************************************************************/
/*PUBLIC int strip_path(char * filename, const char * pathname,
		      struct inode** ppinode)
{
	const char * s = pathname;
	char * t = filename;

	if (s == 0)
		return -1;

	if (*s == '/')
		s++;

	while (*s) {		
		if (*s == '/')
			return -1;
		*t++ = *s++;
		
		if (t - filename >= MAX_FILENAME_LEN)
			break;
	}
	*t = 0;

	*ppinode = root_inode;

	return 0;
}*/


PUBLIC int strip_path_dir(char * filename, const char * pathname,
		      struct inode** ppinode)
{
	char  dirnames[5][12];
	int numofname=0;
	memset(dirnames[numofname], 0, 12);
	const char * s = pathname;
	//printl("pathname is %s\n",s);
	char * t = filename;
	int j=0;
	if (s == 0)
	{
		//printl("here");
		return -1;
	}

	if (*s == '/')
		s++;
	int k=0;
	while (*s) {		/* check each character */
		if (*s == '/')
		{
			numofname++;
			k=0;
			memset(dirnames[numofname], 0, 12);
			s++;
			continue;
		}
		else
		{
			//printl("each s is %c\n",*s);
			dirnames[numofname][k++] = *s++;

			if (dirnames[numofname] - filename >= MAX_FILENAME_LEN)
			break;
			/* if filename is too long, just truncate it */
			/*if (k - filename >= MAX_FILENAME_LEN)
				break;*/
		}
	}

	/*for(j=0;j<10;j++)
	{
		printl("each name is %s\n",dirnames[numofname]);

	}*/
	//printl("name len  is %d\n",strlen(dirnames[numofname]));
	
	for(j=0;j<strlen(dirnames[numofname]);j++)
	{
		*t++=dirnames[numofname][j];

	}
	*t = 0;
	//printl("filename is %s\n",filename);
    *ppinode = root_inode;
    int i=0;
	for(int k=0;k<numofname;k++)
	{
		int dir_blk0_nr = (*ppinode)->i_start_sect;
		int nr_dir_blks = ((*ppinode)->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
		int nr_dir_entries =
	  	(*ppinode)->i_size / DIR_ENTRY_SIZE; /**
					       * including unused slots
					       * (the file has been deleted
					       * but the slot is still there)
					       */
		int m = 0;
		struct dir_entry * pde;
		for (i = 0; i < nr_dir_blks; i++) {
			RD_SECT((*ppinode)->i_dev, dir_blk0_nr + i);
			pde = (struct dir_entry *)fsbuf;
			for (int j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
				if (memcmp(dirnames[i], pde->name, MAX_FILENAME_LEN) == 0)	
				{
					int idev=(*ppinode)->i_dev;
					if(*ppinode!=root_inode)
						put_inode(*ppinode);
					*ppinode=get_inode(idev,pde->inode_nr);
				}
				if (++m > nr_dir_entries)
					break;
			}
			if (m > nr_dir_entries) /* all entries have been iterated */
				break;
		}
	} 
	

	return 0;
}


