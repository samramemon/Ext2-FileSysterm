/************************************************************************
  Cpt_S 360: Lab 4 - Unix/Linux Copy Function

  Nofal Aamir
  WSU ID 11547300
************************************************************************/
#include <stdio.h>       // for printf()
#include <stdlib.h>      // for exit()
#include <string.h>      // for strcpy(), strcmp(), etc.
#include <libgen.h>      // for basename(), dirname()
#include <fcntl.h>       // for open(), close(), read(), write()
#include <errno.h>

// for stat syscalls
#include <sys/stat.h>
#include <unistd.h>
 
// for opendir, readdir syscalls
#include <sys/types.h>
#include <dirent.h>

#define BLKSIZE 4096

struct stat sb1, sb2, sbd3;  // sbd3 for cpd2d, for entities under directory
char buf[BLKSIZE], *cwd, buff[256];

/*********************FUNCTION DECLARATIONS********************************/
int dbname(char *pathname, char *dname, char *bname);
int samedirs(char *f1, char *f2);
int fileexists(char *f1, struct stat *sb);
int myrcp(char *f1, char *f2);
int cpf2f(char *f1, char *f2);
int cpf2d(char *f1, char *f2);
int cpd2d(char *f1, char *f2);

/*********************FUNCTIONs******************************************/
/*********************************************************/
int main(int argc, char *argv[])
{
         if (argc < 3)
	   {
	       printf("enter filenames\n");
	       return -1;
	   }
	 	 
	 return myrcp(argv[1], argv[2]);
}

/*********************************************************/
int dbname(char *pathname, char *dname, char *bname)
{
         char temp[128]; // dirname(), basename() destroy original pathname
	 strcpy(temp, pathname);
	 strcpy(dname, dirname(temp));
	 strcpy(temp, pathname);
	 strcpy(bname, basename(temp));
}

/*********************************************************/
int fileexists(char *f, struct stat *sb)
{
         if (lstat(f, sb) == -1)
	   {
	     perror("stat");
	     return -1;
	   }
	 printf("File name:                %s\n", f);
}

int samedirs(char *f1, char *f2)
{
	   struct stat sbx, sby;
	   char f2y[100];
	   strcpy(f2y, f2);
	   lstat(f1, &sbx);
	   strcat(f2y, "/..");
	   
	   while(lstat(f2y, &sby) == 0)
	     {
	       if ((sbx.st_dev == sby.st_dev) && (sbx.st_ino == sby.st_ino))
		 {
		   printf("can't copy to same DIR\n");
		   return -1;
		 }
	       else if (sby.st_ino == 2)  // when root is reached
		 break;
	       strcat(f2y, "/..");
	     }
	   return 0;
}

/*********************************************************/
int samefile(struct stat sb1, struct stat sb2)
{
           if ((sb1.st_dev == sb2.st_dev) && (sb1.st_ino == sb2.st_ino))
	     return 0;
	   
	   return -1;
}

/*********************************************************/
// f1=SOURCE f2=DEST
int myrcp(char *f1, char *f2)
{
         /*
	   1. stat f1;   if f1 does not exist ==> exit.
	   f1 exists: reject if f1 is not REG or LNK file
	   2. stat f2;   reject if f2 exists and is not REG or LNK file

	   3. if (f1 is REG){
	   if (f2 non-exist OR exists and is REG)
	   return cpf2f(f1, f2);
	   else // f2 exist and is DIR
	   return cpf2d(f1,f2);
	   }
	   4. if (f1 is DIR){
	   if (f2 exists but not DIR) ==> reject;
	   if (f2 does not exist)     ==> mkdir f2
	   return cpd2d(f1, f2);
	   }
	 */
         char tempf2[10];
	 
         if (fileexists(f1, &sb1) < 0)   exit(EXIT_FAILURE);  // must exit if f1 doesn't exist

	 printf("File type:                ");

	 switch (sb1.st_mode & S_IFMT)
	   {
	   /*********************************************/
	   case S_IFDIR:  printf("directory\n");
	     if (fileexists(f2, &sb2) < 0)
	       {
		 printf("file doesn't exist...making DIR\n");
		 // mkdir and continue
		 if (mkdir(f2, 0766) < 0)
		   {
		     perror("mkdir");
		     break;
		   }
		 lstat(f2, &sb2);
	       }
	     else
	       {
		 strcpy(tempf2, f2);
		 strcat(tempf2, "/");
		 strcat(tempf2, f1);
		 if (fileexists(tempf2, &sb2) < 0)
		   {
		     printf("file doesn't exist...making DIR\n");
		     strcat(f2, "/");
		     strcat(f2, f1);
		     // mkdir and continue
		     if (mkdir(f2, 0766) < 0)
		       {
			 perror("mkdir");
			 break;
		       }
		     lstat(f2, &sb2);
		   }
	       }
	     if (samefile(sb1, sb2) == 0)
	       {
		 printf("same dirs!\n");
		 return 0;
	       }
	     
	     switch (sb2.st_mode & S_IFMT)    // check f2
	       {
	       case S_IFDIR:           // if f2 is DIR
		 {
		   if (samedirs(f1,f2) < 0) break;
		   return cpd2d(f1, f2);
		 }
		 
	       case S_IFREG:           // if f2 is a REG File
		 printf("can't copy DIR to FILE\n");
		 break;

	       case S_IFLNK:
		 break;

	       default:       printf("unknown?\n");
		 break;
	       } 
	     break;
	   /*********************************************/
	   case S_IFLNK:  printf("symlink\n");
	     if (fileexists(f2, &sb2) < 0)
		 printf("symfile doesn't exist...creating it\n");
	     // creat file and continue, cpf2f will run
	     return cpf2f(f1, f2);
	     
	   /*********************************************/
	   case S_IFREG:  printf("regular file\n");
	     if (fileexists(f2, &sb2) < 0)
	       {
		 printf("file doesn't exist...making FILE\n");
		 // creat file and continue, cpf2f will run
		 cpf2f(f1, f2);
	       }
	     
	     switch (sb2.st_mode & S_IFMT)    // check f2
	       {
	       case S_IFDIR:            // if f2 is DIR
		 return cpf2d(f1, f2);
	       
	       case S_IFREG:            // if f2 is REG file
		 return cpf2f(f1, f2);

	       case S_IFLNK:
		 return cpf2f(f1, f2);

	       default:       printf("unknown?\n");
		 break;
	       }
	     break;
	   /*********************************************/
	   default:       printf("unknown?\n");
	     break;
	   }
}

/*********************************************************/
// cp file to file
int cpf2f(char *f1, char *f2)
{
          /*
	    1. reject if f1 and f2 are the SAME file
	    2. if f1 is LNK and f2 exists: reject
	    3. if f1 is LNK and f2 does not exist: create LNK file f2 SAME as f1
	    4:
	    open f1 for READ;
	    open f2 for O_WRONLY|O_CREAT|O_TRUNC, mode=mode_of_f1;
	    copy f1 to f2
	  */
          printf("cpf2f running....\n");
	  cwd = getcwd(buff, 256);
	  printf("cwd = %s\n", cwd);
	  
	  if (samefile(sb1, sb2) == 0)
	    {
	      printf("same file!\n");
	      return 0;
	    }

	  if ((sb1.st_mode & S_IFMT) == S_IFLNK || (sbd3.st_mode & S_IFMT) == S_IFLNK)
	    {
	      printf("f2 doesn't exist...making f2 as LNK to f1\n");
	      if(link(f1, f2))
		perror("link");
	      return 0;
	    }
	  int fd, gd;
	  int n, total=0;

	  fd = open(f1, O_RDONLY);
	  if (fd < 0)
	    {
	      printf("open errno=%d : %s\n", errno, strerror(errno));
	      return 0;
	    }
	  
	  gd = open(f2, O_WRONLY|O_CREAT|O_TRUNC, sb1.st_mode);

	  while (n = read(fd, buf, BLKSIZE))
	    {
	      write(gd, buf, n);
	      total += n;
	    }
	  printf("total=%i\n", total);
	  close(fd); close(gd);
}

/*********************************************************/
int cpf2d(char *f1, char *f2)
{
             /*
	       1. search DIR f2 for basename(f1)
	       (use opendir(), readdir())
	       // x=basename(f1); 
	       // if x not in f2/ ==>        cpf2f(f1, f2/x)
	       // if x already in f2/: 
	       //      if f2/x is a file ==> cpf2f(f1, f2/x)
	       //      if f2/x is a DIR  ==> cpf2d(f1, f2/x)
	     */
           printf("cpf2d running....\n");
	   cwd = getcwd(buff, 256);
	   printf("cwd = %s\n", cwd);
	   
	   char f1dname[64], f1bname[64], f2dname[64], f2bname[64], catf2[64];        // for dirname and basename
	   dbname(f1, f1dname, f1bname);
	   dbname(f2, f2dname, f2bname);

	   strcpy(catf2, f2);
	   strcat(catf2, "/");
	   strcat(catf2, f1bname);    // adding f1 to the path of DIR f2
	   
	   struct dirent *ep;       // use dirent to search directories
	   DIR *dp = opendir(f2);   // open the DIR 'f2'
	   if (!dp)
	     {
	       printf("errno=%d : %s\n", errno, strerror(errno));
	       return 0;
	     }
	   
	   while (ep = readdir(dp))
	     {
	       if (strcmp(f1bname, ep->d_name) == 0)
		 {
		   printf("%s already exists in %s\n", f1, f2);
		   if ((sb2.st_mode & S_IFMT) == S_IFREG)
		     return cpf2f(f1, catf2);
		   else if ((sb2.st_mode & S_IFMT) == S_IFDIR)
		     return cpf2d(f1, catf2);
		 }
	     }
	   closedir(dp);
	   
	   // we now know that f1 doesn't exist in f2
	   printf("%s DOES NOT exists in %s\n", f1, f2);
	   cpf2f(f1, catf2);
}

/*********************************************************/
int cpd2d(char *f1, char *f2)
{
           // recursively cp dir into dir
           printf("cpd2d running....\n");
	   
	   char f1dname[64], f1bname[64], f2dname[64], f2bname[64];
	   char s[128], n[128];
	   int r;

	   dbname(f1, f1dname, f1bname);
	   printf("f1   dname: %s, bname: %s\n", f1dname, f1bname);
	   dbname(f2, f2dname, f2bname);
	   printf("f2   dname: %s, bname: %s\n", f2dname, f2bname);
	   
	   struct dirent *ep;
	   DIR *dp = opendir(f1);
	   if (!dp)
	     {
	       printf("errno=%d : %s\n", errno, strerror(errno));
	       return 0;
	     }
	   
	   while(ep = readdir(dp))
	     {
	       if (strcmp(ep->d_name, "..") == 0)
		 continue;

	       else if (strcmp(ep->d_name, ".") == 0)
		 continue;

	       printf("name=%s \n", ep->d_name);

	       strcpy(n, f1);
	       strcat(n, "/");
	       strcat(n, ep->d_name);
	       if (fileexists(n, &sbd3) < 0)
		 {
		   printf("file doesn't exist, move on...\n");
		   return 0;
		 }
	       
	       strcpy(s, f2);
	       strcat(s, "/");
	       strcat(s, ep->d_name);
	       
	       if ((sbd3.st_mode & S_IFMT) == S_IFDIR)
		 {
		   printf("mkdir = %s\n", s);
		   r = mkdir(s, 0766);
		   if (r < 0)
		     {
		       printf("mkdir errno=%d : %s\n", errno, strerror(errno));
		       return 0;
		     }
		   cpd2d(n, s);
		 }
	       else
		 cpf2d(n, f2);
	     }
}
