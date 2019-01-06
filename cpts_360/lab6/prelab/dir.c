/********* dir.c code ***************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

#define BLKSIZE 1024

char buf[BLKSIZE];
int fd;

/******************* in <ext2fs/ext2_fs.h>*******************************

**********************************************************************/

int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

int search(INODE *ip, char *name)
{
  
}

int dirp()
{
  int i, iblock, dblock, dcount;
  char *cp, dbuf[BLKSIZE], temp[256];
  
  // read Group Descriptor 0
  get_block(fd, 2, buf);
  gp = (GD *)buf;

  dcount = gp->bg_used_dirs_count;
  printf("used_dirs_count=%d\n", dcount);
  
  iblock = gp->bg_inode_table;   // get inode start block#
  // get inode start block     
  get_block(fd, iblock, buf);

  ip = (INODE *)buf + 1; // get the 2nd INODE (or root INODE)
  
  for (i=0; i < 12; i++){  // assuming only 12 entries
    if (ip->i_block[i] == 0)
      break;
    get_block(fd, ip->i_block[i], dbuf);
    //if ((ip->i_mode >> 12) != 4)
    //continue;
    dp = (DIR *)dbuf;
    cp = dbuf;
    
    while (cp < dbuf + BLKSIZE){
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      printf("%4d %4d %4d %s\n", 
	     dp->inode, dp->rec_len, dp->name_len, temp);

      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }
  printf("\n");
}

char *disk = "mydisk";

int main(int argc, char *argv[ ])
{
  if (argc > 1)
     disk = argv[1];
  fd = open(disk, O_RDONLY);
  if (fd < 0){
    printf("open failed\n");
    exit(1);
  }
  dirp();
}
