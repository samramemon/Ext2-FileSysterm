// The echo client client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX 256

// Define variables
struct dirent *ep;

char command[64], pathname[64];
char t1[20] = "xwrxwrxwr-------";
char t2[20] = "----------------";

struct hostent *hp;              
struct sockaddr_in  server_addr; 

int server_sock, r;
int SERVER_IP, SERVER_PORT; 


// clinet initialization code

int client_init(char *argv[])
{
   printf("======= clinet init ==========\n");

   printf("1 : get server info\n");
   hp = gethostbyname(argv[1]);
   if (hp==0){
      printf("unknown host %s\n", argv[1]);
      exit(1);
   }

   SERVER_IP   = *(long *)hp->h_addr_list[0];
   SERVER_PORT = atoi(argv[2]);

   printf("2 : create a TCP socket\n");
   server_sock = socket(AF_INET, SOCK_STREAM, 0);
   if (server_sock<0){
      printf("socket call failed\n");
      exit(2);
   }

   printf("3 : fill server_addr with server's IP and PORT#\n");
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = SERVER_IP;
   server_addr.sin_port = htons(SERVER_PORT);

   // Connect to server
   printf("4 : connecting to server ....\n");
   r = connect(server_sock,(struct sockaddr *)&server_addr, sizeof(server_addr));
   if (r < 0){
      printf("connect failed\n");
      exit(1);
   }

   printf("5 : connected OK to \007\n"); 
   printf("---------------------------------------------------------\n");
   printf("hostname=%s  IP=(not working)  PORT=%d\n", 
            hp->h_name, SERVER_PORT); // inet_ntoa(SERVER_IP)
   printf("---------------------------------------------------------\n");

   printf("========= init done ==========\n");
}

int ls_file(char *fname)
{
	struct stat fstat, *sp;
	int r, i;
	char ftime[64], temp[10];
	sp = &fstat;
	
	if ( (r = lstat(fname, &fstat)) < 0){
		printf("can't stat %s\n", fname);
		exit(1);
	}
	if ((sp->st_mode & 0xF000) == 0x8000)// if (S_ISREG())
		printf("%c",'-');
	if ((sp->st_mode & 0xF000) == 0x4000)// if (S_ISDIR())
		printf("%c",'d');
	if ((sp->st_mode & 0xF000) == 0xA000)// if (S_ISLNK())
		printf("%c",'l');
	for (i=8; i >= 0; i--) {
		if (sp->st_mode & (1 << i)) // print r|w|x
			printf("%c", t1[i]);
		else // or print -
			printf("%c", t2[i]);
	}
	printf("%4d ",sp->st_nlink); // link count
	printf("%4d ",sp->st_gid); // gid
	printf("%4d ",sp->st_uid); // uid
	printf("%8d ",sp->st_size); // file size
	// print time
	strcpy(ftime, (time_t)ctime(&sp->st_ctime)); // print time in calendar form
	ftime[strlen(ftime)-1] = 0; // kill \n at end
	printf("%s ",ftime);
	// print name
	printf("%s", fname); // print file basename
	// print -> linkname if symbolic file
	//if ((sp->st_mode & 0xF000)== 0xA000){
		// use readlink() to read linkname
		//printf(" -> %s<p>", linkname); // print linked name
	//}
	printf("\n");
}

int ls_dir(char *dname)
{
   if (strcmp(dname, "") == 0)
		strcpy(dname, ".");
   // use opendir(), readdir(); then call ls_file(name)
	DIR *dp = opendir(dname);
	while (ep = readdir(dp)) {
		ls_file(ep->d_name);
	}
}

int getClient(char* filename)
{
   int size, count, fd, n;
   char buf[MAX] = {0};

   // Get size of the transfer from sender 
   read(server_sock, buf, MAX);
   sscanf(buf, "%d", &size);
   printf("fsize:%i\n", size);

   if(size <= 0)
      return -1;

   // Write data from sender into specified file
   count = 0;
   fd = open(filename, O_WRONLY | O_CREAT, 0664);
   while(count < size)
   {
      n = read(server_sock, buf, MAX);
      write(fd, buf, n);
      count += n;
   }
   close(fd);
   return 0;
}

int client_put(char *fname)
{
   struct stat sb;
   int fd, size, n;
   char buf[MAX], temp[10];
   // Check that file exists
   printf("fname:%s\n", fname);
   if (stat(fname, &sb) == 0){
      size = sb.st_size;
      printf("sizeput:%i\n",size);
   }
   else
      size = 0;

   if(size < 0)
      return -1;
   // send total bytes to client first
   if (size == 0) strcpy(temp, "\n");
   else           sprintf(temp, "%d", size);
   n = write(server_sock, temp, MAX);
   // now send to client
   if ((fd = (open(pathname, O_RDONLY))) < 0) {
      printf("can't open %s\n", pathname);
      return 0;
   }
   while (n = read(fd, buf, MAX)) 
      write(server_sock, buf, n);

   close(fd);
}

main(int argc, char *argv[ ])
{
   int n;
   char line[MAX], ans[MAX];
   char *endptr;
   char cwd[128];

   if (argc < 3){
      printf("Usage : client ServerName SeverPort\n");
      exit(1);
   }

   client_init(argv);
   // sock <---> server
   printf("********  processing loop  *********\n");
   while (1){
      printf("input a command \n");
      printf("[get put ls mkdir rmdir rm cd pwd]] \n");
      printf("[lls lmkdir lrmdir lrm lcd lpwd]] \n");
      bzero(line, MAX);                // zero out line[ ]
      fgets(line, MAX, stdin);         // get a line (end with \n) from stdin

      sscanf(line, "%s %s", command, pathname);
      line[strlen(line)-1] = 0;        // kill \n at end
      if (line[0]==0)                  // exit if NULL line
         exit(0);

      if (strcmp(command, "lls") == 0) 
         ls_dir(pathname);
      if (strcmp(command, "lcd") == 0) 
         chdir(pathname);
      if (strcmp(command, "lpwd") == 0) {
         getcwd(cwd, 128);
         printf("%s\n", cwd);
      } 
      if (strcmp(command, "lmkdir") == 0) 
         mkdir(pathname, 0755);
      if (strcmp(command, "lrmdir") == 0) 
         rmdir(pathname);
      if (strcmp(command, "lrm") == 0) 
         unlink(pathname);


      if (strcmp(command, "ls") == 0) {
         // Send ENTIRE line to server
         n = write(server_sock, line, MAX);
         printf("client: wrote n=%d bytes; line=(%s)\n", n, line);

         // Read a line from sock and show it
         while (n = read(server_sock, ans, MAX)) {
            printf("%s", ans);
            if (strcmp(ans, "$") == 0) break; 
         }
      }
      if (strcmp(command, "pwd") == 0) {
         // Send ENTIRE line to server
         n = write(server_sock, line, MAX);
         printf("client: wrote n=%d bytes; line=(%s)\n", n, line);

         // Read a line from sock and show it
         n = read(server_sock, ans, MAX);
         printf("%s\n", ans);
      }
      if (strcmp(command, "cd") == 0) {
         // Send ENTIRE line to server
         n = write(server_sock, line, MAX);
         printf("client: wrote n=%d bytes; line=(%s)\n", n, line);

      }
      if (strcmp(command, "mkdir") == 0) {
         // Send ENTIRE line to server
         n = write(server_sock, line, MAX);
         printf("client: wrote n=%d bytes; line=(%s)\n", n, line);

      }
      if (strcmp(command, "rmdir") == 0) {
         // Send ENTIRE line to server
         n = write(server_sock, line, MAX);
         printf("client: wrote n=%d bytes; line=(%s)\n", n, line);

      }
      if (strcmp(command, "rm") == 0) {
         // Send ENTIRE line to server
         n = write(server_sock, line, MAX);
         printf("client: wrote n=%d bytes; line=(%s)\n", n, line);

      }
      if (strcmp(command, "get") == 0) {
         n = write(server_sock, line, MAX);
         printf("client: wrote n=%d bytes; line=(%s)\n", n, line);

         getClient(pathname);
      }
      if (strcmp(command, "put") == 0) {
         n = write(server_sock, line, MAX);
         printf("client: wrote n=%d bytes; line=(%s)\n", n, line);

         client_put(pathname);
      }
      strcpy(command, "");
      strcpy(pathname, "");
      printf("\n");
   }
}

