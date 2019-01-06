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
extern char pathname[256];

/*************************FUNCTIONS*******************************/
int get_block(int dev, int blk, char *buf)
{
    lseek(dev, (long)blk*BLKSIZE, 0);
    int n = read(dev, buf, BLKSIZE);
    if (n < 0) printf("get_block: [%d %d] error\n", dev, blk);
}

int put_block(int dev, int blk, char *buf)
{
    lseek(dev, (long)blk*BLKSIZE, 0);
    int n = write(dev, buf, BLKSIZE);
    if (n != BLKSIZE) printf("put_block: [%d %d] error\n", dev, blk);
}

MINODE* mialloc()   // allocate a free inode
{
    int i;
    for (i = 0; i<NMINODE; i++){
        MINODE *mp = &minode[i];
        if (mp->refCount == 0){
            mp->refCount = 1;
            return mp;
        }
    }
    printf("FS panic: out of minodes\n");
    return 0;
}

int midalloc(MINODE *mip)   // release a used minode
{
    mip->refCount = 0;  // just resetting the refCount is enough
                        // to reset as that memory location is not
                        // locked anymore
}

// this function returns a pointer to the in-memory minode containing
// the INODE of (dev, ino). If NOT found then allocate a new minode
MINODE* iget(int dev, int ino)
{
    //MTABLE *mp;
    INODE *ip;
    int i, block, offset;
    char buf[BLKSIZE];

    // search in-memory minodes first
    for (i = 0; i<NMINODE; i++){
        MINODE *mip = &minode[i];
        if (mip->refCount  && (mip->dev == dev)  && (mip->ino == ino)){
            mip->refCount++;
            return mip;
           }
    }

    MINODE *mip;
    // needed INODE=(dev, ino) not in memory
    mip = mialloc();          // allocate a free minode
    mip->dev = dev; mip->ino = ino;  // assign to (dev, ino)

    block = (ino - 1) / 8 + iblk;
    offset = (ino - 1) % 8;
    get_block(dev, block, buf);
    
    ip = (INODE*) buf + offset;
    mip->INODE = *ip;    // this is where inode on disk is placed in memory
    
    // initialize minode
    mip->refCount = 1;
    mip->mounted = 0;
    mip->dirty = 0;
    mip->mptr = 0;
    return mip;
}

// This function releases a used minode pointed by mip. Each minode
// has a refCount, which represents the number of users that are
// using the minode, iput() decrements refCount by 1. If caller is
// last user (i.e. refCount 0 after decrement) AND modified (dirty)
// the INODE is written back to disk
int iput(MINODE *mip)
{
    INODE *ip;
    int i, block, offset;
    char buf[BLKSIZE];

    if (mip == 0) return 0;
    mip->refCount--;   // dec refCount by 1
    if (mip->refCount > 0) return 0;  // still has a user
    if (mip->dirty == 0) return 0;    // no need to write back
                                    // as no changes have been made
    //write INODE back to disk
    block = (mip->ino - 1) / 8 + iblk;
    offset = (mip->ino - 1) % 8;

    // get block containing this inode
    get_block(mip->dev, block, buf);
    ip = (INODE  *)buf + offset;      // ip now points to INODE of mip
    *ip = mip->INODE;           // copy INODE to inode in block
    put_block(mip->dev, block, buf);  // write back to disk
    midalloc(mip);                  // mip->refCount = 0;
}

int tokenize(char *path)
{
    int i = 0;
    char *s;

    strcpy(gpath, path);
    s = strtok(gpath, "/");
    while (s != NULL)
	{
        name[i] = s;    // store pointer to token in *name[] array  
        s = strtok(0, "/");
        i++;
	}
    return i;
}

int search(MINODE *mip, char *name)
// which searches the DIRectory's data blocks for a name string; 
// return its inode number if found; 0 if not.
{
    char *cp, dbuf[BLKSIZE], temp[256];
    int i;     
    for (i=0; i < 12; i++){  // assuming only 12 entries
        if (mip->INODE.i_block[i] == 0)
            return 0;

        get_block(dev, mip->INODE.i_block[i], dbuf);  // get disk block
        dp = (DIR *)dbuf;
        cp = dbuf;
        
        printf("inode       rec len       name len       name\n");
        while (cp < dbuf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            printf("%10d %10d %10d %5s\n", 
                dp->inode, dp->rec_len, dp->name_len, temp);
            if (strcmp(name, temp) == 0){
                printf("found %s : inumber = %d\n", name, dp->inode);
                return dp->inode;
            }
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    printf("\n");
    return 0;
}

int getino(char *pathname)
{
    MINODE *mip;
    int i, n, ino;
    char ibuf[BLKSIZE];
    
    if (strcmp(pathname, "/") == 0)
        return 2;    // return ino of root, i.e. 2
    if (pathname[0] == '/'){      
        mip = root;
        printf("root dev:%d ino:%d\n", mip->dev, mip->ino);
    }
    else{
        mip = running->cwd;
        printf("cwd dev:%d ino:%d\n", mip->dev, mip->ino);
    }

    n = tokenize(pathname);  // tokenize pathname and store in *tkn[]

    for (i = 0; i < n; i++){  // iterate for all the names in the path, i.e. /a/b/c -> n = 3
        if (!S_ISDIR(mip->INODE.i_mode)){
            printf("%s is not a directory\n", name[i]);
            iput(mip);
            return 0;
        }
        ino = search(mip, name[i]);
        if (!ino){
            printf("can't find %s\n", name[i]); 
            return 0;
        }
        //iput(mip);          // release current inode ->LINE COMMENTED but was present in book
        mip = iget(dev, ino);  // switch to new minode
    }
    //iput(mip);   ->LINE COMMENTED but was present in book
    return ino;
}

int truncate(MINODE *mip)
{
    // deallocates all data blocks 
    int i, id, id1, id2;
    unsigned int i12, i13, *i_dbl, *di_db1, *di_db2;
    char indbuf[BLKSIZE/4], dindbuf1[BLKSIZE/4], dindbuf2[BLKSIZE/4];
    INODE *ip;

    ip = &mip->INODE;

    // if unlinking a file the inode deallocate is dealt with there
    for (i = 0; i < 12; i++) {
        if (ip->i_block[i] == 0) continue;
        printf("iblock num:%i\n", ip->i_block[i]);
        bdealloc(dev, ip->i_block[i]);
        mip->dirty = 1;
        ip->i_block[i] = 0;
    }
        // clear double indirect blocks, if any
    i12 = ip->i_block[12];
    printf("12th block:%i\n", i12);
    if (i12 == 0){
        ip->i_size = 0;
        return 0;
    }

    get_block(dev, i12, indbuf);
    i_dbl = (unsigned int *)indbuf;

    for (id = 0; id < 256; id++) {
        if (i_dbl[id] == 0) continue;
        printf("indbuf1 %4d\n", i_dbl[id]);
        bdealloc(dev, i_dbl[id]);
        mip->dirty = 1;
        i_dbl[id] = 0;
    }
    
    i13 = ip->i_block[13];
    printf("13th block:%i\n", i13);
    if (i13 == 0) {
        ip->i_size = 0;
        return 0;
    } 
    get_block(fd, i13, dindbuf1);
    di_db1 = (unsigned int *)dindbuf1;
    for (id1 = 0; id1 < 256; id1++) {
        get_block(fd, di_db1[id1], dindbuf2);
        di_db2 = (unsigned int *)dindbuf2;
        printf("id1:%i\n", id1);
        //getchar();
        for (id2 = 0; id2 < 256; id2++) {
            if (di_db2[id2] == 0) continue;
            printf("indbuf2 %4d\n", di_db2[id2]);
            bdealloc(dev, i_dbl[id]);
            mip->dirty = 1;
            di_db2[id2] = 0;
        }
    }
    ip->i_size = 0;
}

int dbname(char *pathname, char *dname, char *bname)
{
    char temp[128]; // dirname(), basename() destroy original pathname
    strcpy(temp, pathname);
    strcpy(dname, dirname(temp));
    strcpy(temp, pathname);
    strcpy(bname, basename(temp));
}