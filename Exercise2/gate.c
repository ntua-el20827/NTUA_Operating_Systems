// Georgakopoulos Georgios 03120827

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/resource.h>

char f = 'f';
char t = 't';
int length;
bool kill_everything;
pid_t* child;
pid_t wpid;
char *buf;
char *letters;
void sigusr1_handler();
void sigusr2_handler();
void sigterm_handler();
void sigchld_handler();

int main(int argc, char **argv){
    kill_everything = false;
    if  (argc!=2){
        printf("Usage: %s string_of_f_and_t // Example: %s fftft\n", argv[0],argv[0]);
        return 1;
    }
    char help[] = "--help";
    int result = strcmp(help,argv[1]);
    if (result==0){
        printf("Usage: %s string_of_f_and_t  // Example: %s fftft\n", argv[0],argv[0]);
        return 0;
    }

    length = strlen(argv[1]); // posa paidia tha exo

    int gates[length];
    for (int j=0;j<length;j++){ // checks if the string contains only f or t 
        if (argv[1][j]!=f && argv[1][j]!=t){
            printf("String has to contain only 'f' and 't' characters \n");
            return 1;
        }
    }
    
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);
    signal(SIGTERM, sigterm_handler);
    signal(SIGCHLD, sigchld_handler);
 
    child = (pid_t*)malloc(length); // allocate memory for child array
    buf = (char *)malloc(length);
    letters = (char *)malloc(length);

    int status;
    for (int i=0;i<length;i++){
        letters[i] = argv[1][i];
        sprintf(buf,"%d",i);
        //printf("buf is: %c \n", buf[0]);
        child[i] = fork();
        //printf("%d",child[i]);
        if (child[i]>0){
            // fathers code -> everything here
            printf("[PARENT/PID=%d] Created child %d (PID=%d) and initial state '%c' \n",getpid(),i,child[i],argv[1][i]);
            //wait(&status);
        }
        if (child[i]==0){
            // code for children -> different file -> child.c
            char *const args[] = {"./child",argv[1],buf,NULL}; //not child.c / I want the executable / It has to be compiled / den xreiazetai anagkastika na einai argv
            int exec_status = execv("./child", args);
                // h execv den afinei na treksei gia to paidi apo edo kai kato
            /* on success, execution will never reach this line */
            if (exec_status==-1){
                perror("Execv");
                return 1;
            }
            exit(0); // xwris ayto bgazei n!+1 apotelesmata
        }
        if (child[i]<0){
            perror("Fork: ");
        }
    }

    // while loop to opoio perimenei ola ta paidia na pethanoun
    while ((wpid = wait(&status)) > 0);
}

void sigusr1_handler(){
    for(int i=0;i<length;i++){
        if(kill(child[i],SIGUSR1)==-1){ //kill returns 0 on succes / -1 on error
            perror("Kill Failed: ");
            exit(1);
        }
    }
}
void sigusr2_handler(){
    for(int i=0;i<length;i++){
        if(kill(child[i],SIGUSR2)==-1){ //kill returns 0 on succes / -1 on error
            perror("Kill Failed: ");
            exit(1);
        }
    }
}
void sigterm_handler(){ // termatismos olwn twv diergasiwn
    kill_everything = true;
    int j = length;
    int status;
    for(int i=0;i<length;i++){
        printf("[PARENT/PID=%d] Waiting for %d to exit \n",getpid(),j);
        if(kill(child[i],SIGTERM)==-1){ //kill returns 0 on succes / -1 on error
            perror("Kill Failed: ");
            exit(1);
        }
        pid_t dead_kid_pid = waitpid(-1,&status,0); // accepts a SIGCHLD signal //returns pid of child who died // in case of error returns -1
        if (WIFEXITED(status)) { // This macro returns a nonzero value if the child process terminated normally with exit or _exit.
            printf("[PARENT/PID=%d] Child %d with PID=%d terminated successfully with exit status code %d!\n", getpid(), i, child[i], WEXITSTATUS(status));
        } else {
            printf("[PARENT/PID=%d] Child %d with PID=%d terminated unsuccessfully with exit status code %d!\n", getpid(), i, child[i], WEXITSTATUS(status));
        }
        //https://www.gnu.org/software/libc/manual/html_node/Process-Completion-Status.html
        j--;
    }
    printf("[PARENT/PID=%d] All children exited, terminating as well \n",getpid());
    exit(0);
}

void sigchld_handler(){ // αν πεθανει καποιο παιδι πρεπει να το ξανα δημιουργησει // 
    if (kill_everything) return;
    int status;
    
    // for loop , elegxei an kapoio paidi exei pethanei, an nai ftiaxnei ena kainoyrgio gia to gate pou exei meinei keno
    for(int i=0;i<length;i++){
        if (waitpid(child[i],&status,WNOHANG)==0) continue; // epistrefei kateutheian an exei kanei exit kapoio paidi. An nai epistrefei -1, An oxi epitrefei 0

        // εδω χρειαζεται κωδικας που τυπωνει το μνμα οτι πεθανε το παιδι
        printf("[PARENT/PID=%d] Child %d with PID=%d exited \n",getpid(),i,child[i]);

        // O upoloipos kodikas ftiaxnei ksana to paidi pou pethane
        sprintf(buf,"%d",i);
        child[i] = fork();
        if (child[i]>0){
            // fathers code -> everything here
            printf("[PARENT/PID=%d] Created child %d (PID=%d) and initial state '%c' \n",getpid(),i,child[i],letters[i]);
        }
        if (child[i]==0){
            // code for children -> different file -> child.c
            char *const args[] = {"./child",letters,buf,NULL}; //not child.c / I want the executable / It has to be compiled / den xreiazetai anagkastika na einai argv
            int exec_status = execv("./child", args);

            /* on success, execution will never reach this line */
            if (exec_status==-1){
                perror("Execv");
                exit(1);
            }
            exit(0); 
        }
        if (child[i]<0){
            perror("Fork: ");
        }
    }

    // elegxos gia an exei stamathsei kapoio paidi, an nai tote to synexizei
    pid_t stoped_kid = waitpid(-1, &status, WUNTRACED|WNOHANG); // an den exeis stamathsei kapoio paidi epistrefei 0
    if (WIFSTOPPED(status)) { // This macro returns a nonzero value if the child process is stopped.
        int i;
        for(i=0;i<length;i++){
            if (child[i]==stoped_kid) break;
        }
        printf("[PARENT/PID=%d] Child %d (PID=%d) stopped by signal %s\n", getpid(),i,stoped_kid, strsignal(WSTOPSIG(status)));
        kill(stoped_kid, SIGCONT);
    }
    pid_t continued_kid = waitpid(-1,&status,WCONTINUED|WNOHANG); // xoris to WCONTINUED den trexei sosta / to WCONTINUED leei oti ena process exei synexisei eno prin eixe stamathsei 
    if (continued_kid){
        if (WIFCONTINUED(status)) {
            int i;
            for(i=0;i<length;i++){
                if (child[i]==stoped_kid) break;
            }
            printf("[PARENT/PID=%d] Child %d (PID=%d) resumed \n", getpid(),i,continued_kid);
        }
    }
} 