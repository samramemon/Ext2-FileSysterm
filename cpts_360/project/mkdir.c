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
int make_dir(char *pathname)
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
    //2. Let  
    // parent = dirname(pathname);   parent= "/a/b" OR "a/b"
    // child  = basename(pathname);  child = "c"
    dbname(pathname, parent, child);

    //3. Get the In_MEMORY minode of parent:
    //     pino  = getino(parent);
    //     pip   = iget(dev, pino); 
    //Verify : (1). parent INODE is a DIR (HOW?)   AND
    //         (2). child does NOT exists in the parent directory (HOW?);
    pino = getino(parent);
    pip = iget(dev, pino);
    printf("parent path mode is %d: HEX %04X\n", pip->INODE.i_mode, pip->INODE.i_mode);
    if (pip->INODE.i_mode != DIR_MODE)
    {
        printf("not a DIR\n");
        return 0;
    }

    if (search(pip, child) != 0){
        printf("%s already exists in cwd:%s\n", child, parent);
        return 0;
    }
    //4. call mymkdir(pip, child);
    mymkdir(pip, child);
    //5. inc parent inodes's link count by 1; 
    // touch its atime and mark it DIRTY
    pip->INODE.i_links_count++;
    pip->INODE.i_atime = time(0L);
    //6. iput(pip);
    iput(pip);
}

int mymkdir(MINODE *pip, char *name)
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

    ip->i_mode = DIR_MODE;		// OR 040755: DIR type and permissions
    ip->i_uid  = running->uid;	// Owner uid 
    ip->i_gid  = running->gid;	// Group Id
    ip->i_size = BLKSIZE;		// Size in bytes 
    ip->i_links_count = 2;	        // Links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
    ip->i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks 
    ip->i_block[0] = bno;             // new DIR has one data block   
    for (i=1; i<15; i++)
        ip->i_block[i] = 0;

    mip->dirty = 1;               // mark minode dirty
    iput(mip);                    // write INODE to disk

    get_block(dev, ip->i_block[0], buf);
    dp = (DIR *)buf;
    cp = buf;

    printf("populate new dir with .\n");   // current directory info
    dp->inode = mip->ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    strncpy(dp->name, ".", dp->name_len);
    cp += dp->rec_len;
    dp = (DIR *)cp;

    printf("populate new dir with ..\n");  // parent directory info
    dp->inode = pip->ino;
    dp->rec_len = 1012;
    dp->name_len = 2;
    strncpy(dp->name, "..", dp->name_len);

    put_block(dev, ip->i_block[0], buf);

    enter_name(pip, ino, name);
}

int enter_name(MINODE *pip, int myino, char *myname)
{
    char *cp, buf[BLKSIZE];
    int i, need_length, remaining;
    INODE *pinode;
    DIR *dp;

    pinode = &pip->INODE;
    for (i=0; i<12; i++){
        if (pinode->i_block[i] == 0) break;

        need_length = 4*( (8 + strlen(myname) + 3)/4 );  // a multiple of 4

        // step to LAST entry in block: int blk = parent->INODE.i_block[i];
        // which is equivalent to pinode->i_block[i];
        get_block(dev, pinode->i_block[i], buf);  // get disk block
        dp = (DIR *)buf;
        cp = buf;

        printf("step to LAST entry in data block %d\n", pinode->i_block[i]);
        // loop through to reach the last entry
        while (cp + dp->rec_len < buf + BLKSIZE){ 
            /****** Technique for printing, compare, etc.******/
            char c;
            c = dp->name[dp->name_len];
            dp->name[dp->name_len] = 0;
            printf("%s ", dp->name);
            dp->name[dp->name_len] = c;
            /***********************************/
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        printf("\n");
        // dp NOW points at last entry in block
        // remain = LAST entry's rec_len - last entry IDEAL_LENGTH;
        int ideal_len = 4*( (8 + dp->name_len + 3)/4 );
        // remember how the rec len of last entry has the whole length remaining in the block
        remaining = dp->rec_len - ideal_len;

        if (remaining > need_length) { // we have more than what we need - so we good
            dp->rec_len = ideal_len;    // update rec_len of last entry - will become 2nd to last now
            cp += dp->rec_len;
            dp = (DIR *)cp;

            dp->inode = myino;
            dp->rec_len = remaining;    // remining bytes are assigned to added entry (now last)
            //dp->name_len = need_length; //Prof said this is wrong
            dp->name_len = strlen(myname);
            strncpy(dp->name, myname, dp->name_len);
        }
        else {  // need more than what we have - get new block
            //#5
            int nbno;
            dp->rec_len = ideal_len;    // update rec_len of last entry - will become 2nd to last now
            pinode->i_size += BLKSIZE;  // because allocating new data block to contain new dir

            nbno = balloc(dev);
            pinode->i_block[i+1] = nbno;
            get_block(dev, pinode->i_block[i+1], buf);  // get next disk block
            dp = (DIR *)buf;
            dp->inode = myino;
            dp->rec_len = BLKSIZE;
            dp->name_len = need_length;
            strncpy(dp->name, myname, dp->name_len);
            put_block(dev, pinode->i_block[i], buf);
            return 1;  // go back cuz the dir has been added and we don't need 
                       // to iterate to the next block
        }
        put_block(dev, pinode->i_block[i], buf);
    }
}