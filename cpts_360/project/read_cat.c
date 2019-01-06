/**** globals defined in main.c file ****/
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include "type.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern OFT oftp[NFD];
extern PROC   proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;
extern char pathname[256];

int myread(int fd, char *buf, int nbytes)
{
    char *cp, *cq;
    unsigned int i12, i13, *i_dbl, *di_db1, *di_db2;
    char indbuf[BLKSIZE/4], dindbuf1[BLKSIZE/4], dindbuf2[BLKSIZE/4], readbuf[BLKSIZE];
    int pblk, lblk, start, remain, avail, id, count = 0;
    OFT *oftp;
    MINODE *fmip;

    cq = buf;
    // assign the openned file in running Proc to the local ptr oftp
    oftp = running->fd[fd];
    avail = oftp->mptr->INODE.i_size - oftp->offset; // filesize - offset
    
    // get the file's MINODE ptr
    fmip = oftp->mptr;
    if (oftp->mode != R && oftp->mode != RW) {
        printf("do not have read permissions\n");
        return 0;
    }
    
    while (nbytes && avail) {
        // logical block will tell us which block our updated offset resides in
        // NOTE: offset is just which exact byte no. we want to read or start reading
        lblk = oftp->offset / BLKSIZE; // note: offset is 0 when new file
        start = oftp->offset % BLKSIZE;

        if (lblk < 12){                     // lbk is a direct block
            pblk = fmip->INODE.i_block[lblk]; // map LOGICAL lbk to PHYSICAL blk
        }
        else if (lblk >= 12 && lblk < 256 + 12) { 
            // indirect blocks
            i12 = fmip->INODE.i_block[12];
            get_block(dev, i12, indbuf);
            i_dbl = (unsigned int *)indbuf;

            pblk = i_dbl[lblk-12];
        }
        else { 
            // double indirect blocks
            i13 = fmip->INODE.i_block[13];
            get_block(dev, i13, dindbuf1);
            di_db1 = (unsigned int *)dindbuf1;
            lblk -= (256 + 12);
            di_db2 = (unsigned int *)di_db1[lblk/256];
            get_block(dev, di_db2, dindbuf2);
            pblk = dindbuf2[(lblk-12) % 256];
            //for (id = 0; id < 256; id++) {
                //get_block(dev, di_db1[id], dindbuf2);
                //di_db2 = (unsigned int *)dindbuf2;

                //pblk = di_db2[lblk - 268];
            //}
        }

        // get the data block into readbuf[BLKSIZE]
        get_block(fmip->dev, pblk, readbuf);

        cp = readbuf + start;  // start address to read in disk
        remain = BLKSIZE - start;   // number of bytes that remain in readbuf[]
        //printf("nbytes:%i, avail:%i, remain:%i\n", nbytes, avail, remain);
        while (remain > 0 && avail > 0) {
            if (avail < nbytes) {
                memcpy(cq, cp, avail);
                oftp->offset += avail;
                count += avail;
                avail -= avail;
                nbytes -= avail;
                remain -= avail;
            }
            else { // if nbytes are greater than available bytes then read only as much are in the current block
                memcpy(cq, cp, remain);
                oftp->offset += remain;
                count += remain;
                avail -= remain;
                nbytes -= remain;
                remain -= remain;
            }
            //printf("nbytes:%i, avail:%i\n", nbytes, avail);
            if (nbytes <= 0 || avail <= 0) // this condition will also break the outter loop
                break;
        }
        //getchar();
    }
    printf("myread: read %d char from file descriptor %d\n", count, fd);  
    return count;   // count is the actual number of bytes read
}

int my_cat(char *pathname)
{
    char buf[BLKSIZE];
    int n, fd;
    fd = my_open(pathname, RW);
    pfd();
    while(n = myread(fd, buf, BLKSIZE)){
       buf[n] = 0;             // as a null terminated string
       printf("%s", buf);  // <=== THIS works but not good
       //spit out chars from mybuf[ ] but handle \n properly;
   }     
    close_file(fd);
}