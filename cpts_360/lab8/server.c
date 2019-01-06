// This is the echo SERVER server.c
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

#define  MAX 256

// Define variables:
struct dirent *ep;

char command[64], pathname[64], bufc[MAX];
char t1[20] = "xwrxwrxwr-------";
char t2[20] = "----------------";

struct sockaddr_in  server_addr, client_addr, name_addr;
struct hostent *hp;

int  mysock, client_sock;              // socket descriptors
int  serverPort;                     // server port number
int  r, length, n;                   // help variables

// Server initialization code:

int server_init(char *name)
{
   printf("==================== server init ======================\n");   
   // get DOT name and IP address of this host

   printf("1 : get and show server host info\n");
   hp = gethostbyname(name);
   
   if (hp == 0){
      printf("unknown host\n");
      exit(1);
   }
   printf("    hostname=%s IP=(not shown)\n", // IP=%d
               hp->h_name);     // inet_ntoa(*(long *)hp->h_addr))
  
   //  create a TCP socket by socket() syscall
   printf("2 : create a socket\n");
   mysock = socket(AF_INET, SOCK_STREAM, 0);
   if (mysock < 0){
      printf("socket call failed\n");
      exit(2);
   }


   printf("3 : fill server_addr with host IP and PORT# info\n");
   // initialize the server_addr structure
   server_addr.sin_family = AF_INET;                  // for TCP/IP
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   // THIS HOST IP address  
   server_addr.sin_port = 0;   // let kernel assign port

   printf("4 : bind socket to host info\n");
   // bind syscall: bind the socket to server_addr info
   r = bind(mysock,(struct sockaddr *)&server_addr, sizeof(server_addr));
   if (r < 0){
       printf("bind failed\n");
       exit(3);
   }

   printf("5 : find out Kernel assigned PORT# and show it\n");
   // find out socket port number (assigned by kernel)
   length = sizeof(name_addr);
   r = getsockname(mysock, (struct sockaddr *)&name_addr, &length);
   if (r < 0){
      printf("get socketname error\n");
      exit(4);
   }

   // show port number
   serverPort = ntohs(name_addr.sin_port);   // convert to host ushort
   printf("    Port=%d\n", serverPort);

   // listen at port with a max. queue of 5 (waiting clients) 
   printf("5 : server is listening ....\n");
   listen(mysock, 5);
   printf("===================== init done =======================\n");
}

int ls_file(char *fname)
{
	struct stat fstat, *sp;
	int r, i;
	char ftime[64], temp[50];
	sp = &fstat;
	
	if ( (r = lstat(fname, &fstat)) < 0){
		printf("can't stat %s\n", fname);
		exit(1);
	}
	if ((sp->st_mode & 0xF000) == 0x8000) {// if (S_ISREG())
		printf("%c",'-');
      strcat(bufc, "-");
   }
	if ((sp->st_mode & 0xF000) == 0x4000) {// if (S_ISDIR())
		printf("%c",'d');
      strcat(bufc, "d");
   }
	if ((sp->st_mode & 0xF000) == 0xA000) {// if (S_ISLNK())
		printf("%c",'l');
      strcat(bufc, "l");
   }
	for (i=8; i >= 0; i--){
		if (sp->st_mode & (1 << i)) { // print r|w|x
			printf("%c", t1[i]);
         sprintf(temp, "%c", t1[i]); //temp[0] = t1[i];
         strcat(bufc, temp);
      }
		else { // or print -
			printf("%c", t2[i]); 
         sprintf(temp, "%c", t2[i]); //temp[0] = t2[i];
         strcat(bufc, temp);
      }
	}
	printf("%4d ",sp->st_nlink); // link count
   sprintf(temp, "%d", sp->st_nlink);
   strcat(bufc, "    "); strcat(bufc, temp);

	printf("%4d ",sp->st_gid); // gid
   sprintf(temp, "%d", sp->st_gid);
   strcat(bufc, "    "); strcat(bufc, temp);

	printf("%4d ",sp->st_uid); // uid
   sprintf(temp, "%d", sp->st_uid);
   strcat(bufc, "    "); strcat(bufc, temp);

	printf("%8d ",sp->st_size); // file size
   sprintf(temp, "%d", sp->st_size);
   strcat(bufc, "    "); strcat(bufc, temp);
	// print time
	strcpy(ftime, (time_t)ctime(&sp->st_ctime)); // print time in calendar form
	ftime[strlen(ftime)-1] = 0; // kill \n at end
	printf("%s ",ftime); strcat(bufc, "  "); strcat(bufc, ftime);
	// print name
	printf("%s", fname); strcat(bufc, "  "); strcat(bufc, fname);  // print file basename
	// print -> linkname if symbolic file
	//if ((sp->st_mode & 0xF000)== 0xA000){
		// use readlink() to read linkname
		//printf(" -> %s<p>", linkname); // print linked name
	//}
	printf("\n");
   strcat(bufc, "\n");
}

int ls_dir(char *dname)
{
   char temp[50];
   
   if (strcmp(dname, "") == 0)
		strcpy(dname, ".");
   // use opendir(), readdir(); then call ls_file(name)
	DIR *dp = opendir(dname);
	while (ep = readdir(dp)) {
		ls_file(ep->d_name);
      sprintf(temp, "%d", strlen(bufc));
      //n = write(client_sock, temp, MAX);
      n = write(client_sock, bufc, MAX);
      strcpy(bufc, "");
	}
   n = write(client_sock, "$", MAX);
}

int server_get(char *fname)
{
   struct stat sb;
   int fd, size;
   char buf[MAX], temp[10];
   // Check that file exists
   printf("fname:%s\n", fname);
   if (stat(fname, &sb) == 0)
      size = sb.st_size;
   else
      size = 0;

   if(size <= 0)
      return -1;
   // send total bytes to client first
   sprintf(temp, "%d", size);
   n = write(client_sock, temp, MAX);
   // now send to client
   if ((fd = (open(pathname, O_RDONLY))) < 0) {
      printf("can't open %s\n", pathname);
      return 0;
   }
   while (n = read(fd, buf, MAX))
      write(client_sock, buf, n);
   close(fd);
}

int putServer(char* filename)
{
   int size, count, fd, n;
   char buf[MAX] = {0};

   // Get size of the transfer from sender 
   read(client_sock, buf, MAX);
   sscanf(buf, "%d", &size);
   printf("fsize:%i\n", size);
   
   if(size <= 0)
      return -1;

   // Write data from sender into specified file
   count = 0;
   fd = open(filename, O_WRONLY | O_CREAT | O_TMPFILE, 0664);
   printf("fd:%i\n",fd);
   while(count < size)
   {
      n = read(client_sock, buf, MAX);
      write(fd, buf, n);
      count += n;
   }
   close(fd);
   return 0;
}

main(int argc, char *argv[])
{
   char *hostname;
   char line[MAX];
   char cwd[128], buf[MAX], temp[10];

   if (argc < 2)
      hostname = "localhost";
   else
      hostname = argv[1];

   server_init(hostname); 

   // Try to accept a client request
   while(1){
      printf("server: accepting new connection ....\n"); 

      // Try to accept a client connection as descriptor newsock
      length = sizeof(client_addr);
      client_sock = accept(mysock, (struct sockaddr *)&client_addr, &length);
      if (client_sock < 0){
         printf("server: accept error\n");
         exit(1);
      }
      printf("server: accepted a client connection from\n");
      printf("-----------------------------------------------\n");
      printf("        IP=(not shown) port=(not shown))\n"); // inet_ntoa(client_addr.sin_addr.s_addr),  ntohs(client_addr.sin_port)
      printf("-----------------------------------------------\n");

      // Processing loop: newsock <----> client
      while(1){
         n = read(client_sock, line, MAX);
         if (n==0){
            printf("server: client died, server loops\n");
            close(client_sock);
            break;
         }

         // show the line string
         printf("server: read  n=%d bytes; line=[%s]\n", n, line);

         sscanf(line, "%s %s", command, pathname);
         printf("cmd:%s path:%s\n", command, pathname);
         if (strcmp(command, "ls") == 0) 
            ls_dir(pathname);
         if (strcmp(command, "cd") == 0) 
            chdir(pathname);
         if (strcmp(command, "pwd") == 0) {
            getcwd(cwd, 128);
            printf("%s\n", cwd);
            n = write(client_sock, cwd, MAX);
         } 
         if (strcmp(command, "mkdir") == 0) 
            mkdir(pathname, 0755);
         if (strcmp(command, "rmdir") == 0) 
            rmdir(pathname);
         if (strcmp(command, "rm") == 0) 
            unlink(pathname);
         if (strcmp(command, "get") == 0)
            server_get(pathname);
         if (strcmp(command, "put") == 0) 
            putServer(pathname);

         strcat(line, " ECHO");

         // send the echo line to client 
         //n = write(client_sock, line, MAX);

         printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
         printf("server: ready for next request\n");
         strcpy(command, "");
         strcpy(pathname, "");
      }
   }
}
