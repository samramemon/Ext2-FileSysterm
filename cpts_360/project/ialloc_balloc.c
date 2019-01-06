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
int tst_bit(char *buf, int bit)
{
    int i, j;
    i = bit/8; j=bit%8;
    if (buf[i] & (1 << j))
        return 1;
    return 0;
}

int set_bit(char *buf, int bit)
{
    int i, j;
    i = bit/8; j=bit%8;
    buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
    int i, j;
    i = bit/8; j=bit%8;
    buf[i] &= ~(1 << j);
}

int incFreeInodes(int dev)
/*decrement number of free inodes*/
{
    char buf[BLKSIZE];

    // dec free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count++;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count++;
    put_block(dev, 2, buf);
}

int decFreeInodes(int dev)
/*decrement number of free inodes*/
{
    char buf[BLKSIZE];

    // dec free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}

int idealloc(int dev, int ino)
{
	char buf[BLKSIZE];

	get_block(dev, imap, buf);
	clr_bit(buf, ino-1);
	put_block(dev, imap, buf);
    incFreeInodes(dev);
}

int ialloc(int dev)
{
    int  i;
    char buf[BLKSIZE];

    // read inode_bitmap block
    get_block(dev, imap, buf);

    for (i=0; i < ninodes; i++){
        if (tst_bit(buf, i)==0){
        set_bit(buf,i);
        decFreeInodes(dev);

        put_block(dev, imap, buf);

        return i+1;
        }
    }
    printf("ialloc(): no more free inodes\n");
    return 0;
}

int pimap()
{
    int  i;
    char buf[BLKSIZE];

    printf("imap = %d\n", imap);

    // read inode_bitmap block
    get_block(fd, imap, buf);

    for (i=0; i < ninodes; i++){
        (tst_bit(buf, i)) ?	putchar('1') : putchar('0');
        if (i && (i % 8)==0)
        printf(" ");
    }
    printf("\n");
}

int incFreeBlocks(int dev)
/*decrement number of free blocks*/
{
    char buf[BLKSIZE];

    // dec free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count++;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);
}

int decFreeBlocks(int dev)
/*decrement number of free blocks*/
{
    char buf[BLKSIZE];

    // dec free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count--;
    put_block(dev, 2, buf);
}

int bdealloc(int dev, int bno)
{
	char buf[BLKSIZE];

	get_block(dev, bmap, buf);
	clr_bit(buf, bno-1);
	put_block(dev, bmap, buf);
    incFreeBlocks(dev);
}

int balloc(int dev)
{
    int  b;
    char buf[BLKSIZE];

    // read inode_bitmap block
    get_block(dev, bmap, buf);

    for (b=0; b < nblocks; b++){
        if (tst_bit(buf, b)==0){
            set_bit(buf,b);
            decFreeBlocks(dev);

            put_block(dev, bmap, buf);

            return b+1;
        }
    }
    printf("balloc(): no more free blocks\n");
    return 0;
}

int pbmap()
{
    int i;
    char buf[BLKSIZE];

    printf("bmap = %d\n", bmap);
    // read block_bitmap block
    get_block(fd, bmap, buf);
    
    for (i=0; i < nblocks; i++){
        (tst_bit(buf, i)) ?	putchar('1') : putchar('0');
        if (i && (i % 8)==0)
        printf(" ");
    }
    printf("\n");
}