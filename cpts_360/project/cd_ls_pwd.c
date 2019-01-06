/****************************************************************************
*                   cd, ls, pwd Program                                      *
*****************************************************************************/
/**** globals defined in main.c file ****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include <time.h>
#include "type.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap;
extern char pathname[256];
// void printFile(MINODE *mip, char *namebuf)
// {
// 	char *Time;
// 	unsigned short mode;
// 	int type;

// 	mode = mip->INODE.i_mode;
// 	// sscanf(mip->INODE.i_mode, "%o", &mode);
// 	// print out info in the file in same format as ls -l in linux
// 	// print the file type
// 	if((mode & 0120000) == 0120000)
// 	{
// 		printf("l");
// 		type = LINK;
// 	}
// 	else if((mode & 0040000) == 0040000)
// 	{
// 		printf("d");
// 		type = DIRECTORY;
// 	}
// 	else if((mode & 0100000) == 0100000)
// 	{
// 		printf("-");
// 		type = FILE;
// 	}

// 	// print the permissions
// 	if((mode & (1 << 8)))
// 		printf("r");
// 	else
// 		printf("-");
// 	if((mode & (1 << 7)) )
// 		printf("w");
// 	else
// 		printf("-");
// 	if((mode & (1 << 6)) )
// 		printf("x");
// 	else
// 		printf("-");

// 	if((mode & (1 << 5)) )
// 		printf("r");
// 	else
// 		printf("-");
// 	if((mode & ( 1 << 4)) )
// 		printf("w");
// 	else
// 		printf("-");
// 	if((mode & (1 << 3)) )
// 		printf("x");
// 	else
// 		printf("-");

// 	if((mode & (1 << 2)) )
// 		printf("r");
// 	else
// 		printf("-");
// 	if((mode & (1 << 1)) )
// 		printf("w");
// 	else
// 		printf("-");
// 	if(mode & 1)
// 		printf("x");
// 	else
// printf("-");

// 	// print the file info
// printf(" %d %d %d %d", mip->INODE.i_links_count, mip->INODE.i_uid,mip->INODE.i_gid, mip->INODE.i_size);
// 	// Time = ctime(&(mip->INODE.i_mtime));
// 	// Time[strlen(Time) -1 ] = 0;
	

// 	// if this is a symlink file, show the file it points to
// 	if((mode & 0120000) == 0120000)
// 		printf(" => %s\n",(char *)(mip->INODE.i_block));
// 	else
// 		printf("\n");

// 	iput(mip); // cleanup
// } 

int print(MINODE *mip)
{
	char buf[1024], temp[256];
	int i;
	DIR *dp;
	char *cp;

	INODE *ip = &mip->INODE;
	for (i = 0; i < 12; i++)
	{
		if (ip->i_block[i] == 0)
			return 0;
		get_block(dev, ip->i_block[i], buf);

		dp = (DIR *)buf;
		cp = buf;

		printf("inode    rec len    name len    name\n");
		while (cp < buf + 1024)
		{
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0; // removing end of str delimiter '/0', why tho?
			// make dp->name a string in temp[ ]
			printf("%2d %12d %12d %12s\n", dp->inode, dp->rec_len, dp->name_len, temp);
			// printFile(mip,dp->name);
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
}

int ls(char *pathname)
{
	int ino, n;
	MINODE *mip;

	if (strcmp(pathname, "") != 0)
	{
		ino = getino(pathname);
		mip = iget(dev, ino);
		print(mip);
		return 0;
	}
	print(running->cwd);
}

int chdir(char *pathname)
{
	int ino, n;
	MINODE *mip;

	if (strcmp(pathname, "") == 0)
		running->cwd = root;
	else {
		ino = getino(pathname);
		mip = iget(dev, ino);
		printf("path mode is %d: HEX %04X\n", mip->INODE.i_mode, mip->INODE.i_mode);
		if (mip->INODE.i_mode != DIR_MODE) {
			printf("not a DIR\n");
			return 0;
		}
		iput(running->cwd);
		running->cwd = mip;
	}
}

int quit()
{
	int i;
	MINODE *mip;
	for (i = 0; i < NMINODE; i++)
	{
		mip = &minode[i];
		if (mip->refCount > 0)
			iput(mip);
	}
	exit(0);
}

int pwd(MINODE *wd)
{
	if (wd == root)
		printf("/");
	else
		rpwd(wd);

	printf("\n");
}

int rpwd(MINODE *wd)
{
	/*
        if (wd==root) return;
	from i_block[0] of wd->INODE: get my_ino of . parent_ino of ..
	  pip = iget(dev, parent_ino);
	from pip->INODE.i_block[0]: get my_name string as LOCAL

	  rpwd(pip);  // recursive call rpwd() with pip

	print "/%s", my_name;
	*/
	if (wd == root)
		return 0;

	u32 my_ino, parent_ino;
	char *cp, my_name[EXT2_NAME_LEN], buf[BLKSIZE];
	DIR *dp;
	MINODE *pip;
	/*
	my_ino = search(&wd->INODE, ".");
	parent_ino = search(&wd->INODE, "..");
	*/
	my_ino = search(wd, ".");
	parent_ino = search(wd, "..");

	pip = iget(dev, parent_ino);
	get_block(dev, pip->INODE.i_block[0], buf);
	dp = (DIR *)buf;
	cp = buf; // let cp point to the start of the buffer
	while (cp < buf + BLKSIZE)
	{
		cp += dp->rec_len;
		dp = (DIR *)cp;

		if (dp->inode == my_ino)
			break;
	}
	strncpy(my_name, dp->name, dp->name_len);
	my_name[dp->name_len] = 0;
	rpwd(pip);

	printf("/%s", my_name);
}
