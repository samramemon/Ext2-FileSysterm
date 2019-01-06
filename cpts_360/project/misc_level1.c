/**** globals defined in main.c file ****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include <sys/stat.h>
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

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

int my_stat(char *pathname)
{
    /*1. stat filename: 
        struct stat myst;
        get INODE of filename into memory:
            int ino = getino(pathname);
            MINODE *mip = iget(dev, ino);
            copy dev, ino to myst.st_dev, myst.st_ino;
            copy mip->INODE fields to myst fields;
        iput(mip);*/  
    struct stat myst;
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);
    
    myst.st_dev = dev;
    myst.st_ino = ino;
    myst.st_ctime = mip->INODE.i_ctime;
    myst.st_uid = mip->INODE.i_uid;
    myst.st_size = mip->INODE.i_size;
    myst.st_gid = mip->INODE.i_gid;
    myst.st_blocks = mip->INODE.i_blocks;
    myst.st_mode = mip->INODE.i_mode;
    myst.st_nlink = mip->INODE.i_links_count;

    printf("mode=0x%4x\n", myst.st_mode);
    printf("uid=%d  gid=%d\n", myst.st_uid, myst.st_gid);
    printf("size=%d\n", myst.st_size);
    printf("time=%s", ctime(&myst.st_ctime));
    printf("link=%d\n", myst.st_nlink);
    printf("no of blocks=%d\n", myst.st_blocks);

    iput(mip);
}