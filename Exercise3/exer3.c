#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024

pid_t* child;
int** in_pipes;
int** out_pipes;
int number_of_children;

void kill_all_children(int number_of_children);

bool isNumber(char number[]){
   for (int i = 0; i < strlen(number); i++)
      if (isdigit(number[i]) == false)
      return false; //when one non numeric value is found, return false   
   return true;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))


int main(int argc,char **argv){
    // Ελεγχος αρχικών ορισμάτων 
    if  (argc!=2 && argc!=3){
        printf("Usage: %s <nChildren> [--random][--round-robin]\n", argv[0]);
        return 1;
    }
    if (strcmp("--help",argv[1])==0){
        printf("Usage: %s <nChildren> [--random][--round-robin]\n", argv[0]);
        return 0;
    }
    if (!isNumber(argv[1])){
        printf("Usage: %s <nChildren> [--random][--round-robin]\n", argv[0]);
        return 1;
    }

    // Ελεγχος ποιο mode θα ακολουθήσουμε: round-robin ή random
    // round-robin = 0 / random = 1
    int mode = 0; // default ο round-robin
    if (argc==3){ // Αν μας λέει ποιο mode θελει
        if (strcmp("--random",argv[2])==0) mode=1;
        else if (strcmp("--round-robin",argv[2])==0) mode=0;
        else {
            printf("Usage: %s <nChildren> [--random][--round-robin]\n", argv[0]);
            return 1;
        }
    }
    printf("mode = %d\n",mode);

    // Δημιουργία των pipes
    number_of_children = atoi(argv[1]);
    // Δημιουργούμε 2 set απο pipes 
    // in_pipes εκει που γραφει ο πατέρας και διαβάζουν τα παιδιά
    // out_pipes εκεί που γράφουν τα παιδιά και διαβάζει ο πατέρας
    int **in_pipes = malloc(number_of_children * sizeof(int *)); // Τα δημιουργούμε δυναμικά με χρήση της malloc
    int **out_pipes = malloc(number_of_children * sizeof(int *));
    for (int i = 0; i < number_of_children; i++) {
        in_pipes[i] = malloc(2 * sizeof(int));
        out_pipes[i] = malloc(2 * sizeof(int));
        if (pipe(in_pipes[i]) == -1 || pipe(out_pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
    /* 3) Using pointer to a pointer 
            We can create an array of pointers also dynamically using a double 
            pointer. Once we have an array pointers allocated dynamically, 
            we can dynamically allocate memory and for every row
    */
    // Δημιουργία των παιδιών
    child = (pid_t*)malloc(number_of_children);
    int status;
    for (int i = 0; i < number_of_children; i++){
        if ((child[i]=fork())==-1){
            perror("fork");
            return 1;
        }
        else if (child[i]==0){
            // Ο κώδικας των παιδιών
            printf("Created child %d with pid = %d \n",i,getpid());
            //printf("hi");
            int val;
            close(in_pipes[i][1]); // Close write-end of input pipe
            close(out_pipes[i][0]); // Close read-end of output pipe 
            while(1) {
                //printf("in while hi");
                read(in_pipes[i][0], &val, sizeof(int));
                printf("[Child %d] [%d] Child received %d!\n",i,getpid(), val);
                val++;
                //printf("hi after val++");
                sleep(5);
                write(out_pipes[i][1], &val, sizeof(int));
                printf("[Child %d] [%d] Child Finished hard work, writing back %d\n",i,getpid(), val);
            }
            exit(0); 
        }
        
    }

    // Ο κώδικας του πατέρα

    // Αρχικά ο πατέρας πρέπει να κλείσει:
    for (int j = 0; j < number_of_children; j++) {
        close(in_pipes[j][0]);   // το read-end του pipe  στο οποίο αυτος γράφει
        close(out_pipes[j][1]);  // το write-end του pipe απο το οποίο διαβάζει
    }
    int counter=0; // Σε ποια διεργασία θα αναθέσω την δουλεια που ερχεται
    // Βασικό while loop του πατέρα
    while (1) {
        fd_set readfds;
        int maxfd;
        FD_ZERO(&readfds);               //αρχικοποίηση πριν απο κάθε κλήση συστήματος select!
        FD_SET(STDIN_FILENO, &readfds);   //Ενώνουμε την select με το stdin 
        
        for (int k=0; k<number_of_children; k++){
            FD_SET(out_pipes[k][0], &readfds);          // Βάζουμε στο set που θα ελέγχει η select ολα τα fd των pipes!
        }
        // Θέλουμε η select να κάνει monitor τα file descriptors που τους αντιστοιχίζει integer μικρότερος απο τον maxfd
        maxfd=STDIN_FILENO;
        for (int j=0; j<number_of_children; j++){
            maxfd = MAX(maxfd,out_pipes[j][0]);
        }
        maxfd ++;
        // wait until any of the input file descriptors are ready to receive
        int ready_fds = select(maxfd, &readfds, NULL, NULL, NULL);

        if (ready_fds <= 0) {
            perror("select");
            continue; // Επιλέγουμε το πρόγραμμα να μην τερματίζει
        }

        // Η select περασε, ελέγχουμε τις περιπτώσεις οπου μπορει να έχουμε εγγραφη: 1. Terminal 2. Pipe καποιου παιδιου

        // 1. Περίπτωση απο terminal!
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char buffer[BUFFER_SIZE];
            int n_read = read(STDIN_FILENO, buffer, 100);   // error checking!
            // Επιστρεφει τον αριθμό των bytes που διάβασε
            //printf("bytes = %d \n",n_read);
            if (n_read == -1) {
                perror("read");
                exit(-1);
            }
            // buffer[n_read] = '\0';
            // New-line is also read from the stream, discard it.
            if (n_read > 0 && buffer[n_read-1] == '\n') {
                buffer[n_read-1] = '\0';
            } 
            // Περιίπτωση οπου στο terminal δώθηκε "exit"
            if (n_read ==5 && strcmp(buffer, "exit") == 0) { // 5 γιατι μετράει και το <enter>
                 // user typed 'exit', kill child and exit properly
                kill_all_children(number_of_children);
                for (int k = 0; k < number_of_children; k++){
                    close(out_pipes[k][0]); //father also closes the port from where he read from the children 
                    close(in_pipes[k][1]); //father also closes the ports from where he wrote to his chilfren 
                }          
                printf("[PARENT/PID=%d] All children exited, terminating as well \n",getpid());          
                exit(0);
            }
            // Περιίπτωση οπου στο terminal δώθηκε αριθμός!
            else if (n_read >= 1 && isNumber(buffer)){  
                int x=atoi(buffer);
                int rr_child=counter%number_of_children;
                if (mode==0){ // περίπτωση round-robin
                    int n_write=write(in_pipes[rr_child][1], &x, sizeof(int));
                    if ( n_write == -1){
                        perror("write");
                        exit(-1);
                    }
                    printf("[Parent] Assigned %d to child %d.\n", x,rr_child);
                    counter++;
                }
                else if (mode==1){ // περίπτωση random
                    int random_child=rand()%number_of_children; //returns a pseudo-random number in the range of 0 to RAND_MAX.
                    //https://www.tutorialspoint.com/c_standard_library/c_function_rand.htm
                    int n_write =write(in_pipes[random_child][1],&x,sizeof(int));
                    if (n_write == -1){
                        perror("write");
                        exit(-1);
                    }
                    printf("[Parent] Assigned %d to child %d.\n", x,random_child);
                }
            }
            // Περιίπτωση οπου στο terminal δώθηκε οτιδήποτε αλλο (μαζί και το help!)
            else if (n_read >= 1){ 
                printf("Type a number to send job to a child!\n"); 
            }
        }
        // Περίπτωση όπου έγραψε το παιδί σε ένα pipe!
        // someone has written bytes to the pipe, we can read without blocking
        for (int m = 0; m < number_of_children; ++m){
            if (FD_ISSET(out_pipes[m][0], &readfds)) {
                int numb;
                int Rbytes=read(out_pipes[m][0], &numb, sizeof(int)); 
                if (Rbytes == -1){
                    perror("read");
                    exit(-1);
                }
             printf("[Parent] Received result form child %d ----> %d.\n", m,numb);
            }

        }
    } // Τελος του while loop του πατέρα
    return 0;
}

void kill_all_children(int number_of_children){
    int wait_for = number_of_children;
    for (int i = 0; i < number_of_children; ++i){
        printf("Waiting for %d children \n",wait_for);
        int w = waitpid(child[i], NULL, WNOHANG);
        if (w==0){
            printf("[Father process: %d] Will terminate (SIGTERM) child process %d: %d\n",getpid(),i,child[i] );
            kill(child[i],SIGTERM);
            wait(NULL);
        }
        wait_for--;
    }
}