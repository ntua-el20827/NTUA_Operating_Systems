#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/select.h>

int main(){
    int pd[2],nbytes;
    pid_t childpid;
    int numb = 3;
    char readbuffer[80];
    pipe(pd);
    int status;

    if ((childpid=fork())==-1){
        perror("fork");
        return 1;
    }
    if (childpid==0){
        int val;
        read(pd[0],&val,sizeof(int));
        printf("[child] Received number: %d\n",val);
        val++;
        sleep(3);
        write(pd[1],&val,sizeof(int));
        return 0;
    }
    else {
        write(pd[1],&numb,sizeof(int));
        printf("[parent] Sent Number: %d\n",numb);
        //sleep(1);
        read(pd[0],&numb,sizeof(int));
        printf("[parent] Received number: %d\n",numb);
        wait(&status);
    }
    return 0;
}