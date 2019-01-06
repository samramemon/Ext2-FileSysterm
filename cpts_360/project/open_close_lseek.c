/**** globals defined in main.c file ****/
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include "type.h"

extern MINODE   minode[NMINODE];
extern MINODE   *root;
extern OFT      oftp[NFD];
extern PROC   proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;
extern char pathname[256];

int fdalloc(PROC *proc)   // allocate a free fd
{
    int i;
    for (i = 0; i<NFD; i++){
        OFT *of = proc->fd[i];
        // refCount for all FDs are set to 0 in init()
        // they're set to 1 when opened
        if (of->refCount == 0){
            return i; // return the lowest available index
        }
    }
    printf("FS panic: out of FDs\n");
    return 0;
}

int my_open(char *pathname, int mode)
{
    int ino, i;
    MINODE *mip;
    INODE *ip;
    //OFT oftp;

    if (pathname[0] == '/')  dev = root->dev;
    else                     dev = running->cwd->dev;

    ino = getino(pathname);
    if (ino == 0)  {// if no file present then create it!
        creat_file(pathname);
        ino = getino(pathname);
    }
    mip = iget(dev, ino); // get the file's MINODE

    ip = &mip->INODE;
    //printf("imode:%x\n", ip->i_mode);
    if (S_ISREG(ip->i_mode) && //check if it's a REG file
        ((ip->i_mode & S_IRWXU) == S_IRUSR) ||  // if File Owner has READ permissions
        ((ip->i_mode & S_IRWXG) == S_IRGRP))   { // if Group has READ permissions

        for (i = 0; i < NFD; i++) {
            // checking if the pathname minode can be found in the current list
            // of open fds in the running PROC - so we can check if file is already open
            if (mip == running->fd[i]->mptr) {
                printf("file already open\n");
                return 0;
            }
        }

        int ifd = fdalloc(running);
        oftp[ifd].mode = mode;      // mode = 0|1|2|3 for R|W|RW|APPEND 
        oftp[ifd].refCount = 1;
        oftp[ifd].mptr = mip;  // point at the file's minode[]

        switch(mode){
            case 0 : oftp[ifd].offset = 0;     // R: offset = 0
                    break;
            case 1 : truncate(mip);        // W: truncate file to 0 size
                    oftp[ifd].offset = 0;
                    break;
            case 2 : oftp[ifd].offset = 0;     // RW: do NOT truncate file
                    break;
            case 3 : oftp[ifd].offset =  mip->INODE.i_size;  // APPEND mode
                    break;
            default: printf("invalid mode\n");
                    return(-1);
        }

        running->fd[ifd] = &oftp[ifd];  // Let running->fd[i] point at the OFT entry

        switch(mode){
            case 0: ip->i_atime = time(0L);
                    break;
            case 1: ip->i_atime = time(0L);
                    ip->i_mtime = time(0L);
                    break;
            case 2: ip->i_atime = time(0L);
                    ip->i_mtime = time(0L);
                    break;
            case 3: ip->i_atime = time(0L);
                    ip->i_mtime = time(0L);
                    break;
            default: printf("invalid mode\n");
                    return(-1);
        }
        oftp[ifd].mptr->dirty = 1;
        //iput(mip); IMP-->release MINODE when closing the file 

        return ifd; // return file descriptor
    }
    else {
        printf("Not a REG file or no permissions\n");
        return 0;   
    }
}

int close_file(int fd)
{
    if (fd < 0 || fd > NFD) {
        printf("not a valid fd\n");
        return -1;
    }

    if (running->fd[fd]) {  // if it exists
        OFT *oftp = running->fd[fd];
        //running->fd[fd] = 0;
        oftp->refCount--;
        if (oftp->refCount > 0) return 0;
        // last user of this OFT entry - clear everything
        oftp->mode = 0;
        oftp->offset = 0;
        oftp->refCount = 0; // it should already be zero - just makin sure
        // last user of this OFT entry ==> dispose of the Minode[]
        MINODE *mip = oftp->mptr;
        iput(mip);
        oftp->mptr = 0;

        return 0; 
    }
}

int my_lseek(int fd, int position)
{   /*
    From fd, find the OFT entry. 

    change OFT entry's offset to position but make sure NOT to over run either end
    of the file.

    return originalPosition
    */
    int origPos;
    OFT *oftp;
    MINODE *mip;

    oftp = running->fd[fd];
    mip = oftp->mptr;
    origPos = oftp->offset;

    if (position < origPos || position > mip->INODE.i_size) {
        printf("positions is invalid\n");
        return -1;
    }
    oftp->offset = position;

    return origPos;
}

int pfd()
{
    /* This function displays the currently opened files as follows:

            fd     mode    offset    INODE
        ----    ----    ------   --------
            0     READ    1234   [dev, ino]  
            1     WRITE      0   [dev, ino]
        --------------------------------------
    to help the user know what files has been opened.  */
    int i;
    MINODE *mip;
    OFT *opft;
    printf("    fd    mode     offset    INODE\n");
    printf("----------------------------------\n");
    for (i = 0; i < NFD; i++) {
        opft = running->fd[i];
        mip = opft->mptr;
        if (opft->refCount > 0) 
            printf("%4d %4d %4d    (%d,%d)\n", 
                    i, opft->mode, opft->offset, mip->dev, mip->ino);
    }
    printf("----------------------------------\n");
}

/*
dup(int fd): 
{
  verify fd is an opened descriptor;
  duplicates (copy) fd[fd] into FIRST empty fd[ ] slot;
  increment OFT's refCount by 1;
}

dup2(int fd, int gd):
{
  CLOSE gd fisrt if it's already opened;
  duplicates fd[fd] into fd[gd]; 
}
*/
