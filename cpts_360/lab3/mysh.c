/************************************************************************
Cpt_S 360: Systems Programming C/C++ - Lab3

Nofal Aamir
WSU ID 11547300

************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>  // contains O_RDONLY, O_WRONLY,O_APPEND, etc

#define true  1
#define false 0

/**********************GLOBALS*****************************************/
char passedin[128];                // for getting user input line
char home[64];                     // for paths to executable and home path
char command[64];                  // for command strings
char *name_paths[15];              // token string pointers
int  no_paths = 0;
char *myargs[100];                 // arguments
int  myargc;                       // number of arguments
char *head[20], *tail[20];         // arguments for different parts of pipe
int  headc, tailc;                 // no. of arguments for parts of pipe
int  pipeflag;
int  fd;                           // file descriptor
char buf[256];                     // for reading from file descriptor

/**********************FUNCTIONS*****************************************/
int resetAll()
{
     memset(command, 0, 64);
     memset(passedin, 0, 128);

     for (int i = 0; i < myargc; i++)
       {
	 myargs[i] = 0;   // make all pointers NULL
       }
     
     myargc = 0;

     for (int i = 0; i < headc; i++)
       {
	 head[i] = 0;   // make all pointers NULL
       }
     
     headc = 0;

     for (int i = 0; i < tailc; i++)
       {
	 tail[i] = 0;   // make all pointers NULL
       }
     
     tailc = 0;
}

int getCommand(int pindex, char *args[])
// gets the command and concatenates with respective PATH given by pindex
{
     char temp[64];

     strcpy(temp, name_paths[pindex]);

     if (strcmp(command, args[0]) != 0)
         strcpy(command, args[0]);  // place first argument in as the command if not already present

     strcat(temp, "/");               // concatenate PATH with '/'
     strcat(temp, command);           // concatenate PATH with command
     
     strcpy(command, temp);         // copy everything to command
     printf("trying cmd: %s\n", command);
}

int tokArguments(char *mystr)
{
     char *s;
     char temp[100];
     int i = 0;

     strcpy(temp, mystr);
  
     s = strtok(temp, " ");
     while(s)
       {
	 if (i == 0)  myargs[i] = (char *)malloc(64*sizeof(char));
	 else         myargs[i] = (char *)malloc(strlen(s)*sizeof(char));
	 strcpy(myargs[i], s);
	 s = strtok(0, " ");
	 i++;
       }
     return i;
}

int getPaths(char *mystr)
{
     char *s;
     char temp[200];
     int i = 0;
  
     strcpy(temp, mystr);  //make copy of mystr entered
  
     s = strtok(temp, "=");
     s = strtok(0, ":");
  
     while (s)
       {
	   name_paths[i] = (char *)malloc(strlen(s)*sizeof(char));
	   strcpy(name_paths[i], s);   //store pointer to token in name[] array
	   s = strtok(0, ":");
	   i++;
       }
     return i;
}

void getHome(char *mystr)
{
     char *s;
     char temp[50];
  
     strcpy(temp, mystr);  //make copy of mystr entered
  
     s = strtok(temp, "=");
     s = strtok(0, "=");

     strcpy(mystr, s);
}

int getheadtail(int n)
{
     for (int c = 0; c < n; c++)   // populate head with whatever is before '|'
       {
	   head[c] = myargs[c];
	   headc = c;
       }

     for (int c = n+1, i = 0; c < myargc; c++)  // populate tail with whatever is after '|'
       {
	   tail[i] = myargs[c];
	   tailc = i;
	   i++;
       }
}

void parseArgs()
{     
     for (int n = 1; n < myargc; n++)
       {
	   if (strcmp(myargs[n], "<") == 0)
	       {
		   printf("Redirect Input < infile\n");  // take inputs from infile
		   fd = open(myargs[n+1], O_RDONLY);  // open filename for READ, which
		                                      // will replace fd 0
		   dup2(fd, 0);    // system call to close file descriptor 0
	       }
	   else if (strcmp(myargs[n], ">") == 0)
	       {
		   printf("Redirect Output > outfile\n");  // send outputs to outfile
		   fd = open(myargs[n+1], O_WRONLY|O_CREAT, 0644); // open filename for WRITE, which
		                                                   // will replace fd 0
		   dup2(fd, 1);    // system call to close file descriptor 1
		   close(fd);
	       }
	   else if (strcmp(myargs[n], ">>") == 0)
	       {
		   printf("Redirect Output >> outfile APPEND\n");  // APPEND outputs to outfile
		   fd = open(myargs[n+1], O_WRONLY|O_CREAT|O_APPEND, 0644); // open filename for WRITE, which
		                                                   // will replace fd 0
		   dup2(fd, 1);    // system call to close file descriptor 0
		   close(fd);
	       }
	   else if (strcmp(myargs[n], "|") == 0)
	       {
		   printf("has | let the pipin beginnnn\n");  // pipe dat shiz
		   pipeflag = true;
		   getheadtail(n);		   
	       }
       }
}

int bepipin(char *env[])
{
      int pd[2], pid, r = 0;

      pipe(pd);        // creates a PIPE; pd[0] for READ  from the pipe, 
      //                 pd[1] for WRITE to   the pipe.

      pid = fork();    // fork a child process
      // child also has the same pd[0] and pd[1]

      if (pid){        // parent as pipe WRITER
	   close(pd[0]); // WRITER MUST close pd[0]
	   close(1);     // close 1
	   dup(pd[1]);   // replace 1 with pd[1]
	   close(pd[1]); // close pd[1] since it has replaced 1
	   
	   for (int c = 0; c < no_paths; c++)
	     {
	          getCommand(c, head);
		  r = execve(command, head, env);   // change image to cmd1
	     }
	   if (r < 0)    printf("errno=%d : %s\n", errno, strerror(errno));

	   exit(1);  // exit pipe WRITER
      }
      else{            // child as pipe READER
	   close(pd[1]); // READER MUST close pd[1]
	   close(0);  
	   dup(pd[0]);   // replace 0 with pd[0]
	   close(pd[0]); // close pd[0] since it has replaced 0
	   
	   for (int c = 0; c < no_paths; c++)
	     {
	          getCommand(c, tail);
		  r = execve(command, tail, env);   // change image to cmd2
	     }
	   if (r < 0)    printf("errno=%d : %s\n", errno, strerror(errno));

	   exit(1);  // exit pipe READER
      }
}

int runchild(char *env[])
{
      int r = 0;
      for (int c = 0; c < no_paths; c++)
	{
	  getCommand(c, myargs);
	  printf("********program exec********\n");

	  r = execve(command, myargs, env);
	}
      
      if (r < 0)    printf("errno=%d : %s\n", errno, strerror(errno));

      exit(1);  // exit child
}

void sysm(char *env[])
{
     int pid, status;
     
     // check exceptions
     if (strcmp(myargs[0], "cd") == 0 && myargs[1] == NULL)
       {
	   if (chdir(home) < 0)    printf("chdir unsuccessful\n");
	   return ;
       }
     else if (strcmp(myargs[0], "cd") == 0)
       {
	   if (chdir(myargs[1]) < 0)    printf("chdir unsuccessful\n");
	   return ;
       }
     else if (strcmp(myargs[0], "exit") == 0)
       {
	   exit(1);
	   return ;
       }
  
     printf("\nTHIS IS %d  MY PARENT=%d\n", getpid(), getppid()); 
     pid = fork();   // fork syscall; parent returns child pid, child returns 0
	   
     if (pid < 0) // fork() may fail. e.g. no more PROC in Kernel
       {  
	 perror("fork failed");
	 exit(1);
       }
      
     if (pid == 0) // child
       {
	   parseArgs();
	   if (pipeflag)
	       bepipin(env);
	   runchild(env);
       }
     else  // parent 
       {
	 printf("PARENT %d WAITS FOR CHILD %d TO DIE\n", getpid(), pid);
	 pid = wait(status);
	 printf("DEAD CHILD=%d, HOW=%04x\n", pid, status);
       }
}

/**********************MAIN*****************************************/
void main(int argc, char *argv[], char *env[])
{
      int index = 0;
      char paths[200];

      memset(paths, 0, 250*sizeof(char));
      
      printf("***********Welcome to nash yo...sup?!*************\n");
      
      // find PATH in environment
      printf("1. show PATH:\n");
      while(strncmp(env[index], "PATH", 4) != 0)
	{
	  index++;
	}
      strcpy(paths, env[index]);
      printf("%s len:%li\n", paths, strlen(paths));


      // find HOME in environment
      index = 0;
      printf("2. show HOME:\n");
      while(strncmp(env[index], "HOME", 4) != 0)
	{
	  index++;
	}
      strcpy(home, env[index]);
      getHome(home);
      printf("%s\n", home);

      
      // decompose PATH
      //printf("strlen_paths: %d", strlen(paths));
      no_paths = getPaths(paths);
      printf("3. decomposed paths:");
      for (index = 0; index < no_paths; index++)
	{
	  printf("%s, ", name_paths[index]);
	}
      printf("\nnumber of paths: %i\n", no_paths);



      
      // main loop
      printf("4.*********loopin nash*******\n");
      while(1)
	{
	  resetAll();
	  
	  printf("nash % : ");
	  fgets(passedin,128,stdin);
	  passedin[strlen(passedin)-1] = 0;
	  
	  if (passedin[0] == '\0')
	    continue;
	  
	  myargc = tokArguments(passedin);
	  printf("arguments:%i\n values: ", myargc);
	  for(int n=0; n<myargc; n++)
	    {
	      printf("%s,", myargs[n]);
	    }
	  printf("\n");

	  sysm(env);
	}
}

