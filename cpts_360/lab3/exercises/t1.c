/****************** t1.c **************************/
#include <stdio.h>
#include <stdlib.h>

main()
{
    int pid;
    printf("THIS IS %d  MY PARENT=%d\n", getpid(), getppid()); 

    pid = fork();   // fork syscall; parent returns child pid, 
    //               child returns 0

    if (pid < 0){   // fork() may fail. e.g. no more PROC in Kernel
      perror("fork faild");
      exit(1);
    }
           
    if (pid){ // PARENT EXECUTES THIS PART 
      printf("THIS IS %d  MY PARENT=%d\n", getpid(), getppid());
    } 
    else{ // child executes this part
      printf("I am    %d  my parent=%d\n", getpid(), getppid());
    }
}
