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

//Checks the minode to ensure the dir is empty
//If the dir isnt empty we cannot delete the dir
int isEmptyDir(MINODE *mip)
{
	char buf[1024];
	INODE *ip = &mip->INODE;
	char *cp;
	char name[64];
	DIR *dp;

	//link count greater than 2 has files
	if(ip->i_links_count > 2)
	{
		return 1;
	}
	else if(ip->i_links_count == 2)
	{
		//link count of 2 could still have files, check data blocks
		if(ip->i_block[1])
		{
			get_block(dev, ip->i_block[1], buf);

			cp = buf;
			dp = (DIR*)buf;

			while(cp < buf + 1024)
			{
				strncpy(name, dp->name, dp->name_len);
				name[dp->name_len] = 0;

				if(strcmp(name, ".") != 0 && strcmp(name, "..") != 0)
				{
					//not empty!
					return 1;
				}
			}
		}
	}
	else
		return 0;//is empty, return 0
}

//this function now removes the ready child of the entry, passing in the parent MINODE and the name of the dir to remove
//this function should search for the child inode block in the parent structure and then be freed
//cleans up parent dir after dealloc an inode
void rm_child(MINODE *parent, char *name)
{
	int i;
	INODE *p_ip = &parent->INODE;
	DIR *dp;
	DIR *prev_dp;
	DIR *last_dp;
	char buf[1024];
	char *cp;
	char temp[64];
	char *last_cp;
	int start, end;

	//going to remove the dir specified
	printf("going to remove %s\n", name);
	//checking the parent size for calcuation
	printf("parent size is %d\n", p_ip->i_size);

	//iterate through blocks
	//this finds the child 
	//go through every block of the parent, hunting down the node to remove
	for(i = 0; i < 12 ; i++)
	{
		//if the parents block # is null we have an issue, prevents seg faults here
		if(p_ip->i_block[i] == 0)
			return;

		//get the first block in the node and store in buf
		get_block(dev, p_ip->i_block[i], buf);
		//set pointer to buf for ref
		cp = buf;
		//cast buf as a dir pointer to see its contents
		dp = (DIR*)buf;

		printf("dp at %s\n", dp->name);

		//while we are not at the end of the direct blocks for the chosen inode
		while(cp < buf + 1024)
		{
			//copy the name of this file into temp for all chars
			strncpy(temp, dp->name, dp->name_len);
			//add a null terminating char
			temp[dp->name_len] = 0;

			//print the name of the file we are scanning
			printf("dp is at %s\n", temp);

			//if temp and name are equal, boom found the target
			if(!strcmp(temp, name))					//0 means equal
			{
				printf("child found!\n"); //time to check block locations
				//check to see if we are at the end of the block list for this inode
				if(cp == buf && cp + dp->rec_len == buf + 1024)
				{
					//it's the first and only entry, need to delete entire block
					//free the malloced memory
					free(buf);
					//deallocate the block from our block management structure, bmap
					bdealloc(dev, ip->i_block[i]);

					//decrement the size of the inode variable size by one block
					p_ip->i_size -= 1024;

					//shift all blocks on the right to the left.
					//while block number and current block are less than 12
					while(p_ip->i_block[i + 1] && i + 1 < 12)
					{
						//get the next block and slide it back one slot
						i++;
						get_block(dev, p_ip->i_block[i], buf);
						put_block(dev, p_ip->i_block[i - 1], buf);
					}
				}
				else if(cp + dp->rec_len == buf + 1024) //single instance, just have to remove last entry
				{
					//just have to remove the last entry
					printf("removing last entry\n");
					prev_dp->rec_len += dp->rec_len; //set dp rec length to += prior and then put the block away
					put_block(dev, p_ip->i_block[i], buf);
				}
				else
				{
					//not last entry, this is where we have problems
					//list the name of the item somewhere before the one we're after
					printf("Before dp is %s\n", dp->name);

					//set last_dp equal to the pointer before we move forward
					last_dp = (DIR*)buf;
					//same for old cp
					last_cp = buf;

					//step into last entry, this will allow us to progress through the inode block strucutre
					//until we find the last element 
					while(last_cp + last_dp->rec_len < buf + BLKSIZE)
					{
						//continue moving the pointers by dir sized chunks until we reach the end
						printf("last_dp at %s\n", last_dp->name);
						last_cp += last_dp->rec_len;
						last_dp = (DIR*)last_cp;
					}

					printf("%s and %s\n", dp->name, last_dp->name);
					//move the rec len to += previous length
					last_dp->rec_len += dp->rec_len;
					
					//START point for the memory remove
					start = cp + dp->rec_len;
					//END point for the memory remove
					end = buf + 1024;

					//
					memmove(cp, start, end - start);//built in function. move memory left
					//put the block back
					put_block(dev, p_ip->i_block[i], buf);

				}
				//set the parent dirty
				parent->dirty = 1;
				//put the parent node back 
				iput(parent);
				return;
			}//end of child found

			//hit the else if or the intial state so we are here
			//keep parsing the blocks searching for the child name
			//advance the pointers and recast.
			prev_dp = dp;
			cp += dp->rec_len;
			dp = (DIR*)cp;
			
			//now advance to the next block if not found
		}
		//advance to the next inode
	}

	return;
}

//accepts the path of the dir to remove
void remove_dir(char *path)
{
	int i;
	int ino, parent_ino;
	MINODE *mip;
	MINODE *p_mip;
	INODE *ip;
	INODE *p_ip;
	char temp[64], child[64];

	//Checks
	if(!path)
	{
		printf("ERROR: no pathname given\n");
		return;
	}

	//copy the path so as not to destroy it
	strcpy(temp, path);
	//basename is the name of the directory the remove
	strcpy(child, basename(temp));

	//get the ino of the child and ensure it exists
	//getino will return zero if cant find the dir, otherwise returns the inumber of the directory, also tokenizes
	ino = getino(path);
	printf("%s ino is %d\n", path, ino);
	//get the inode of the child and point to it with mip
	mip = iget(dev, ino);

	//if the pointer is null then not exist
	if(!mip){
		printf("ERROR: mip does not exist\n");
		return;
	}
	if (mip->refCount > 1) {
		printf("other processes are using this dir\n");
		return;
	}
	//check if dir
	if(!S_ISDIR(mip->INODE.i_mode)){
		printf("ERROR: %s is not a directory\n", path);
		return;
	}
	//check if empty, checks if the child dir has any children besides the . and ..
	if(isEmptyDir(mip)){
		printf("ERROR: directory not empty\n");
		return;
	}

	printf("Starting remove\n");

	//set ip to be the actual inode of the child dir we want to remove
	ip = &mip->INODE;

	//get parent ino from 
	//find ino will use mip to find the ino from . and .. of the dir to remove and assign them accordingly
	findino(mip, &ino, &parent_ino);
	printf("ino is %d\nparent ino is%d\n", ino, parent_ino);
	//set p_mip to point at the parent minode
	p_mip = iget(dev, parent_ino);
	//p_ip points directly at the parent inode for removal tasks
	p_ip = &p_mip->INODE;

	//go through child blocks deallocating them

	//deallocate blocks
	for(i = 0; i < 15 && ip->i_block[i] != 0; i++)
	{
		bdealloc(dev, ip->i_block[i]);
	}
	//blocks for the dir to remove have now been deallocated
	//deallocate child inode
	idealloc(dev, ino);

	//remove entry from parent dir
	rm_child(p_mip, child);

	//update parent set link minus one, set touch times, set dirty since just dealloc a node
	p_ip->i_links_count--;
	p_ip->i_atime = time(0L);
	p_ip->i_mtime = time(0L);
	p_mip->dirty = 1;

	//write parent changes to disk
	iput(p_mip);
	//write changes to deleted directory to disk
	mip->dirty = 1;
	iput(mip);

	return;
}