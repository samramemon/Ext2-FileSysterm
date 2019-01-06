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

//Checks the minode to ensure the dir is empty
//If the dir isnt empty we cannot delete the dir
int isEmptyDir(MINODE *mip)
{
	char *cp, tname[64], buf[BLKSIZE];
	INODE *ip = &mip->INODE;
	DIR *dp;

	//link count greater than 2 has files
	if(ip->i_links_count > 2) {
        printf("dir not empty: link count > 2\n");
		return 0;
	}
	else if(ip->i_links_count == 2) {
		//link count of 2 could still have files, check data blocks for files
		if(ip->i_block[0] == 0) return 1;
        get_block(dev, ip->i_block[0], buf);

        cp = buf;
        dp = (DIR*)buf;

        while(cp < buf + BLKSIZE) {
            // if this doesn't work use dir count, which can't be greater than 2
            strncpy(tname, dp->name, dp->name_len);
            tname[dp->name_len] = 0;
            
            if((strcmp(tname, ".") != 0) && (strcmp(tname, "..") != 0)) {
                printf("dir contains files - 1st file is:%s\n", tname);
                return 0;
            }
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
	}
    //is empty, return 1
	return 1;
}

int rm_child(MINODE *parent, char *name)
{
	int i, n, start, end;
	INODE *pip;
	DIR *dp, *prev_dp, *nxt_dp;
	char buf[BLKSIZE], temp[64];
	char *cp;

    pip = &parent->INODE;
	//going to remove the dir specified
	printf("removing: %s\n", name);
	//checking the parent size for calcuation
	printf("parent size is %d\n", pip->i_size);

	//iterate through blocks this finds the child 
	//go through every block of the parent, hunting down the node to remove
    for(i = 0; i < 12 ; i++) {
		if(pip->i_block[i] == 0)
			return 0;  // consider 'continue'?
        
        get_block(dev, pip->i_block[i], buf);
		cp = buf;
		dp = (DIR*)buf;

        // iterate through parent dir entries
        while(cp < buf + BLKSIZE) {
            //copy the name of this file into temp for all chars
			strncpy(temp, dp->name, dp->name_len);
			//add a null terminating char
			temp[dp->name_len] = 0;
            printf("dp is at %s\n", temp);
            if (strcmp(temp, name) == 0) {
                printf("child found!\n");
                // 1) if LAST entry in block
                if (cp + dp->rec_len == buf + BLKSIZE) {
                    printf("removing LAST entry of current block\n");
					// point to note here is that memory is already allocated as it's whole block
					// so we are not de-allocating memory in this case, we're just removing the dir
					// by making rec len of the prev dir to reach remaining blk size
					// --new stuff will just be written over this dir as it's 'not there'
                    prev_dp->rec_len += dp->rec_len;
					put_block(dev, pip->i_block[i], buf);
                }
                // 2) if (first entry in a data block) 
                // - means cp would point to entry after "." & ".." -> having rec len = 12; 12+12=24
                else if (cp == buf && cp + dp->rec_len == buf + BLKSIZE) {
                    printf("removing FIRST entry of current block\n");
					// need to remove the whole block as nothing's there now
					bdealloc(dev, pip->i_block[i]);
					//decrement the size of the inode variable size by one block
					pip->i_size -= BLKSIZE;
					// in the case we have holes in the data blocks of the INODE
					// because we can remove any dir and we don't know which block it resides in
					// NOTE: this is the only case where a block has to be deallocated
					for (n = 0; n < 12; n++) {
						if(pip->i_block[i] == 0) {
							get_block(dev, pip->i_block[i+1], buf);
							put_block(dev, pip->i_block[i], buf);
						}
					}
                }
                // 3) if in the middle of a block
                else {
                    printf("removing entry somewhere in the MIDDLE of the block\n");
					char *ccp, *st, *end;
					// NOTE: rmdir segfault at memmove() was fixed by changing st and end from int to char*
					DIR *ldp;
					ccp = buf;
					ldp = (DIR *)buf;
					// loop to last entry (ldp) - starting from the beginning
					while(ccp + ldp->rec_len < buf + BLKSIZE) {
						//printf("in loop to last entry\n");
						//strncpy(temp, ldp->name, ldp->name_len);
						//add a null terminating char
						//temp[ldp->name_len] = 0;
						//printf("ndp:%s\n", temp);
						ccp += ldp->rec_len;
						ldp = (DIR *)ccp;
					}
					temp[dp->name_len] = 0;
					printf("to be rmd:%s and last dp:%s\n", dp->name, ldp->name);
					// cp is where current entry resides
					// st or cp + dp->rec_len is where the next entry lies
					// end is total BLOCK size, hence remining space is end - st
					ldp->rec_len += dp->rec_len;
					end = buf + BLKSIZE;
					st = cp + dp->rec_len;
					//printf("cp:%d end:%d st:%d f:%d\n", b, end, st, f);
					memmove(cp, st, end - st);
					put_block(dev, pip->i_block[i], buf);
                }
				return 0;
            }
            // we know 2 iterations will happen for . and ..
            prev_dp = dp;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
	return 0;
}

int remove_dir(char *path)
{
	int i, ino, pino;
	MINODE *mip, *p_mip;
	INODE *ip, *pip;
	char parent[64], child[64];

	//Checks
	if(!path) {
		printf("ERROR: no pathname given\n");
		return 0;
	}
    
    if (path[0] == '/') {
        mip = root;
        dev = root->dev;
    }
    else {
        mip = running->cwd;
        dev = running->cwd->dev;
    }
    // check ownership
    if (running->uid != SUPER_USER)
        if (running->uid != mip->INODE.i_uid) {
			printf("running->uid:%i mip->INODE.i_uid:%i uids don't match\n", running->uid, mip->INODE.i_uid);
			return 0;
		}
    
	//get dirname and basenames of path
	dbname(path, parent, child);

	//get the ino of the child and ensure it exists
	ino = getino(path);
	printf("%s ino is %d\n", path, ino);
	mip = iget(dev, ino);

	//if the pointer is null then not exist
	if(!mip) {
		printf("ERROR: mip does not exist\n");
		return 0;
	}
	if (mip->refCount > 1) {
		printf("other processes are using this dir\n");
		return 0;
	}
	//check if dir
	if(!S_ISDIR(mip->INODE.i_mode)) {
		printf("ERROR: %s is not a directory\n", path);
        iput(mip);
		return 0;
	}
	//check if empty, checks if the child dir has any children besides the . and ..
	if(isEmptyDir(mip) == 0)	{
		printf("ERROR: directory not empty\n");
        iput(mip);
		return 0;
	}

	printf("Starting remove\n");
	//set ip to be the actual inode of the actual dir we want to remove
	ip = &mip->INODE;

	//deallocate blocks
	for(i = 0; i < 12; i++) {
        if (ip->i_block[i]==0)
             continue;
		bdealloc(dev, ip->i_block[i]);
	}
	//deallocate child inode
	idealloc(dev, ino);
    
    //get parent ino
    pino = getino(parent);
	p_mip = iget(dev, pino);
	pip = &p_mip->INODE;

	//remove entry from parent dir
	rm_child(p_mip, child);

	//update parent set link minus one, set touch times, set dirty since just dealloc a node
	pip->i_links_count--;
	pip->i_atime = time(0L);
	pip->i_mtime = time(0L);
	p_mip->dirty = 1;

	//write parent changes to disk
	iput(p_mip);
	//write changes to deleted directory to disk
	//mip->dirty = 1;
	//iput(mip);

	return 1;
}
