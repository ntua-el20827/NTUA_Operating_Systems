#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>

int main(int argc, char *argv[]){
    if (argc!=2){ 
        printf("Usage: %s filename \n",argv[0]);
        return 1;
    }
    char help[] = "--help";
    int result = strcmp(help,argv[1]);
    //https://www.ibm.com/docs/en/zos/2.4.0?topic=functions-strcmp-compare-strings
    if (result==0){
        printf("Usage: %s filename\n", argv[0]);
        return 0;
    }
    struct stat buffer;
    if (stat(argv[1],&buffer)==0){
        printf("Error: %s already exists \n", argv[1]);
        return 1;
    }
    // https://www.learnc.net/c-tutorial/c-file-exists/
    
    int fd = open(argv[1],O_CREAT|O_APPEND|O_WRONLY,0644);
        // O_WRONLY -> Open for writing only.
        // O_APPEND -> Causes all write actions to happen at the end of the file
        // O_CREAT -> Creates file if it doesn't exists
        // 0644 -> The 'mode' argument. When creating new file it specifies the protection mode:  I can read and write it; everyone else can only read it." Most commonly used
            //The file's owner can read and write (6)
            //Users in the same group as the file's owner can read (first 4)
            //All users can read (second 4)
    if (fd == -1){
        perror("open");
        return 1;
    }
    int status;
    pid_t child;
    child = fork();
    
    if (child<0){
        perror("fork failed");
        return 1;
    }
    if (child==0){
        //child's code
        pid_t pid_child = getpid();
        pid_t pid_parent = getppid();

        char buf[50];
        sprintf(buf,"[CHILD] getpid()=%d, getppid()=%d \n" , pid_child , pid_parent);


        // write for child
        if (write(fd,buf,strlen(buf))<strlen(buf)) {
            perror("write");
            return 1;
        }
        close(fd);
        exit(0);
    }
    else {
        // father's code
        //sleep(1); -> 2nd way: sleep now for 1 sec, until child finisses, then write and at the end wait(without if) for the child just in case it hasn't yet been killed
        pid_t i = wait(&status);
        //printf("%d \n",i);
        if (i==-1){
            perror("wait");
            return 1;
        }
        pid_t pid_child = getpid();
        pid_t pid_parent = getppid();
        char buf[50];
        sprintf(buf,"[PARENT] getpid()=%d, getppid()=%d \n" , pid_child , pid_parent);


        // write for father
        if (write(fd,buf,strlen(buf))<strlen(buf)) {
            perror("write");
            return 1;
        }
        close(fd);
        exit(0);
    }
    return 0;
}
