#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <ctype.h>
#include <sys/select.h>  
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define BUFFER_SIZE 512
bool isNumber(char number[]){
   for (int i = 0; i < strlen(number); i++)
        if (isdigit(number[i]) == false){
            if (number[i]==' '){
                continue;
            }
            return false; //when one non numeric value is found, return false  
        } 
   return true;
}

int main(int argc, char** argv){
    bool debug = false;
    // Default Δεδομένα Host
    char host[] = "iot.dslab.pub.ds.open-cloud.xyz";
    int port = 18080;
    // Ελεγχοι Εισόδου
    for(int i=1; i<argc; i++) {
        if (strcmp("--debug",argv[i])==0){
            debug = true;
            printf("[DEBUG] Debug mode on\n");
        }
        if (strcmp("--host",argv[i])==0){
            strncpy(host, argv[++i],sizeof(argv[i]));
            continue;
        }
        if (strcmp("--port",argv[i])==0){
            port = atoi(argv[++i]);
            continue;
        }
    }

    // Ορισμός Socket
    int CL_SOCKET=socket(AF_INET,SOCK_STREAM,0);

    if (CL_SOCKET<0){
        perror("socket");
        return 1;
    }
    printf("socket ok\n");

    // Ορισμός Socket-Port  / BIND για τον CLIENT
    // Δεν ειναι αναγκαστικό για τον client!!
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(0);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(CL_SOCKET, (struct sockaddr *)&sin, sizeof(sin)) < 0){
        perror("bind");
        return 1;    
    }
    //printf("bind oke \n");
    
    // Connect με τον HOST
    struct sockaddr_in sin2;
    memset(&sin2, '\0', sizeof(sin2)); // zero structure out 
    sin2.sin_family = AF_INET;
    sin2.sin_port = htons(port);
    struct hostent *host_address;
    if((host_address = gethostbyname(host))==NULL){
        perror("host");
        return 1;
    }
    memcpy((char *) &sin2.sin_addr.s_addr, host_address->h_addr_list[0], host_address->h_length); // copy the address 
    if (connect(CL_SOCKET, (struct sockaddr *)&sin2, sizeof(sin2)) < 0){
        perror("connect");
        return 1;
    }
    printf("Connecting!\n");
    printf("Connected to iot.dslab.pub.ds.open-cloud.xyz:8080\n");

    char buffer[BUFFER_SIZE];

    // Input απο τον χρήστη
    while(1){
        memset(buffer, 0, sizeof(buffer));
        // Initialize fd set
        fd_set in_set;
        int maxfd;

        FD_ZERO(&in_set);                
        FD_SET(STDIN_FILENO, &in_set);   
        FD_SET(CL_SOCKET, &in_set); // ορίζω τον file descriptor του socket στο οποίο συνδεθήκαμε
        maxfd = MAX(STDIN_FILENO,CL_SOCKET) + 1;
        // Select όπου περιμένουμε
        int ready_fds = select(maxfd, &in_set, NULL, NULL, NULL);
        if (ready_fds <= 0) {
            perror("select");
            continue;
        }
        // Ηρθε κατι απο τον χρήστη
        if (FD_ISSET(STDIN_FILENO, &in_set)){
            int read_bytes = read(STDERR_FILENO,buffer,BUFFER_SIZE);
            if (read_bytes > 0 && buffer[read_bytes] == '\n') {
                buffer[read_bytes-1] = '\0';
            }
            // Ελεγχος για help
            if (read_bytes >= 4 && strncmp(buffer, "help", 4) == 0){
                printf("Please type:\n");
                printf("get / 'N name surname reason' / exit \n");
                continue;
            }
            // Ελεγχος για exit
            if (read_bytes >= 4 && strncmp(buffer, "exit", 4) == 0){
                if (shutdown(CL_SOCKET,2)<0){
                    perror("shutdown");
                    return 1;
                }
                close(CL_SOCKET);
                exit(0);
            }
            // Ελεγχος για get
            if (read_bytes >= 3 && strncmp(buffer, "get", 3)==0){
                int write_bytes = write(CL_SOCKET,buffer,3);
                if (write_bytes<0){
                    perror("write");
                    continue;
                }
                if (debug){
                    printf("[DEBUG] sent 'get' \n");
                    continue;
                }
            }
            // Άδεια εξόδου στην καραντίνα / Ολες οι υπολοιπες περιπτωσεις
            else {
                int write_bytes = write(CL_SOCKET,buffer,read_bytes);
                if (write_bytes<0){
                    perror("write");
                    continue;
                }
                if (debug){
                    printf("[DEBUG] sent %s",buffer);
                    continue;
                }
            }
        }
        // Ηρθε κατι απο τον server
        if (FD_ISSET(CL_SOCKET, &in_set)){
            int read_bytes = read(CL_SOCKET,buffer,BUFFER_SIZE);
            if (read_bytes > 0 && buffer[read_bytes-1] == '\n') {
                buffer[read_bytes-1] = '\0';
            }
            //  Try aggain 
            if (read_bytes >= 1 && strncmp(buffer,"try again",strlen("try again")-1 )==0){
                printf("%s \n",buffer);
                continue;
            }
            // invalid code 
            if (read_bytes >= 1 && strncmp(buffer,"invalid code",strlen("invalid code")-1 )==0){
                printf("%s \n",buffer);
                continue;
            }
            // result of get
            else if(read_bytes >= 1 && isNumber(buffer)){
                if (debug){
                    printf("[DEBUG] read '%s'\n",buffer);
                }
                
                char *original_name;
                for(int i=0;i<20;++i){
                    printf("-");
                }
                printf("\n");

                char interval [40];
                char temperature [4];
                char light [40];
                char time [40];
                
                original_name=buffer; // pointer στο πρωτο στοιχείο οτυ πίνακα ωστε να έω μια αναφορά!!
                strncpy(interval,original_name,1);
                if (atoi(interval)==0){
                printf("boot (0)\n");

                }
                if (atoi(interval)==1){
                printf("setup(1)\n");

                }
                if (atoi(interval)==2){
                printf("interval (2)\n");

                }
                if (atoi(interval)==3){
                printf("button (3)\n");

                }
                if (atoi(interval)==4){
                printf("motion (4)\n");

                }
                strncpy(light,original_name +2,3);
                printf("Light level is: %d\n",atoi(light) );

                strncpy(temperature,original_name +6,4);
                printf("temperature is : %.2f\n",atof(temperature)/100.0);//precision -->2 dekadika psifia

                strncpy(time,original_name+11,10);
                time_t rawtime1=atoi(time); //apo char se long_int 
                struct tm *info;
            
                info = localtime( &rawtime1 );
                printf("Current local time and date: %s", asctime(info));
            }
            // ACK
            else if (read_bytes >= 1 && strncmp(buffer,"ACK",strlen("ACK")-1 )==0){
                buffer[read_bytes-2] = '\0'; 
                if(debug){
                    printf("[DEBUG] read '%s'\n",buffer);
                }
                printf("Response: %s\n",buffer);
                //memset(buffer, 0, sizeof(buffer)); //clear bufferserver
                continue;
            }
            // Verification Code 
            else{
                if(debug){
                    printf("[DEBUG] read '%s' \n",buffer);
                }
                printf("Send verification code: '%s' \n",buffer);
                //memset(buffer, 0, sizeof(buffer)); //clear bufferserver

                // Αν θελουμε να στελνετε πισω το verification code αυτοματα!
                /* int write_bytes = write(CL_SOCKET,buffer,read_bytes);
                if (write_bytes<0){
                    perror("write");
                    continue;
                }
                if (debug){
                    printf("[DEBUG] sent %s \n",buffer);
                    continue;
                } */

            }
        }
    }
    close(CL_SOCKET);
    return 0;
}