/************** t2.c ************************/
     #include <stdio.h>
     #include <stdlib.h>

     main()
     {
       int pid, status;
       pid = fork();
       if (pid){ // PARENT:
           printf("PARENT %d WAITS FOR CHILD %d TO DIE\n", getpid(),pid);
           pid=wait(&status);
           printf("DEAD CHILD=%d, HOW=%04x\n", pid, status);
       }
       else{// child:
           printf("child %d dies by exit(VALUE)\n", getpid());
EXIT:      exit(100);  //OR {int a,b; a=b/0;} ==> see how does it die
       }
     }
