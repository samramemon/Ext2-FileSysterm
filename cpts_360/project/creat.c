/**** globals defined in main.c file ****/
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include "type.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;
extern char line[256], cmd[32], pathname[256];

/*************************FUNCTIONS*******************************/
int creat_file(char *pathname)
{
    int pino;
    MINODE *mip, *pip;
    char parent[64], child[64];

    //1. pahtname = "/a/b/c" start mip = root;         dev = root->dev;
    //          =  "a/b/c" start mip = running->cwd; dev = running->cwd->dev;
    if (pathname[0] == '/') {
        mip = root;
        dev = root->dev;
    }
    else {
        mip = running->cwd;
        dev = running->cwd->dev;
    } 
    //   parent = dirname(pathname);   parent= "/a/b" OR "a/b"
    // child  = basename(pathname);  child = "c"

    dbname(pathname, parent, child);

    //3. Get the In_MEMORY minode of parent:
   //Verify : (1). parent INODE is a DIR (HOW?)   AND
     //       (2). child does NOT exists in the parent directory (HOW?);
    pino = getino(parent);
    pip = iget(dev, pino);
    printf("parent path mode is %d: HEX %04X\n", pip->INODE.i_mode, pip->INODE.i_mode);
    if (pip->INODE.i_mode != DIR_MODE)    {
        printf("not a DIR\n");
        return 0;
    }

    if (search(pip, child) != 0){
        printf("%s already exists in cwd:%s\n", child, parent);
        return 0;
    }
    //4. call my_creat(pip, child);
    my_creat(pip, child);
    //5. inc parent inodes's link count by 1; 
    // touch its atime and mark it DIRTY
    pip->INODE.i_links_count = 1;
    pip->INODE.i_atime = time(0L);
    //6. iput(pip);
    iput(pip);
}

int my_creat(MINODE *pip, char *name)
{
    MINODE *mip;
    int ino, bno, i;
    char *cp, buf[BLKSIZE];
    DIR *dp;

    ino = ialloc(dev);  // get new inode num
    bno = balloc(dev);  // get new block

    // to load the inode into a minode[] (in order to write contents to the INODE in memory)
    mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    //Use ip-> to acess the INODE fields:

    ip->i_mode = FILE_MODE;		// FILE type and permissions
    ip->i_uid  = running->uid;	// Owner uid 
    ip->i_gid  = running->gid;	// Group Id
    ip->i_size = 0;		// Size in bytes - empty file
    ip->i_links_count = 1;	        // Links count=1 because it's a file
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
    ip->i_blocks = 0;                	// LINUX: Blocks count in 512-byte chunks 
    ip->i_block[0] = bno;             // new FILE has one data block   
    for (i=1; i<15; i++)
        ip->i_block[i] = 0;

    mip->dirty = 1;               // mark minode dirty
    iput(mip);                    // write INODE to disk

    enter_name(pip, ino, name);
}