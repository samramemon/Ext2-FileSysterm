/********* bmap.c code ***************/
#include <stdio.h>
#include <stdlib.h>
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
char bufgp[BLKSIZE];
int fd;

int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit / 8;  j = bit % 8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
}

int bmap()
{
  int i, bmapblk, blkcount;
  
  // read Group Descriptor 0
  get_block(fd, 2, buf);
  gp = (GD *)buf;

  bmapblk = gp->bg_block_bitmap;
  printf("bg_block_bitmap = %d\n", bmapblk);
  blkcount = gp->bg_free_blocks_count;
  printf("bg_block_bitmap = %d\n", blkcount);

  // read block_bitmap block
  get_block(fd, bmapblk, buf);
  
  for (i=0; i < blkcount; i++){
    (tst_bit(buf, i)) ?	putchar('1') : putchar('0');
    if (i && (i % 8)==0)
       printf(" ");
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
  bmap();
}
