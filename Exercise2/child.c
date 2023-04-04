#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

char f = 'f';
char t = 't';
bool state;
bool alarm_condition;
int gate;
time_t time_start, time_end;

void sigalrm_handler();
void print_state();
void sigusr1_handler();
void sigusr2_handler();
void sigterm_handler();
void chose_handler(int num);

int main(int argc, char **argv){
    int length = strlen(argv[1]); 
    gate = *argv[2]-'0'; // converting char to int
    // allos tropos gate = atoi(argv[2]);
    state = 0;
    if (argv[1][gate]=='t'){ // apofasi gia to state tou paidiou
        state = 1;
    }

    //assigning signals to functions
    struct sigaction act; // dimiourgoume thn domi
    act.sa_handler = chose_handler; // orizoume se ayth thn domh poia synarthsh ua eiani o handler
    sigaction(SIGALRM,&act,NULL); // Antistoixizoyme ta shmata ston handler
    sigaction(SIGTERM,&act,NULL);
    sigaction(SIGUSR1,&act,NULL);
    sigaction(SIGUSR2,&act,NULL); 

    // Edv exoyme mia synarthsh handler h opoia, opvw faientai kai pio kato, apofasizei telika poio shma ua erthei

    time_start = time(NULL); // arxikos xronos me vasi ton opoio upologizo kathe xrono
    alarm_condition = true;
    raise(SIGALRM); // stelnei epitopou SIGALRM sthn diergasia poy vrisketai
    while(true){ // while true gia na stelno SIGALRM kathe 15 deyterolepta
        if(alarm_condition){
            alarm(15);
            alarm_condition = false;
        }
    }
    // deyteros tropos me pause kai alarm
}

void print_state(){ // prints the state of the gate
    time_end = time(NULL);
    printf("[GATE=%d/PID=%d/TIME=%lds] The gates are ",gate,getpid(),(time_end-time_start));
    if (state==1){
        printf("open! \n");
    }
    else{
        printf("closed! \n");
    }
}

void sigalrm_handler(){ // handles the SIGALRM signals
    print_state();
    alarm_condition = true; 
}
void sigusr1_handler(){ // changes the state of the gate
    state = !state;
    print_state();

}
void sigusr2_handler(){ // prints the state of the gate
    print_state();
}
void sigterm_handler(){
    exit(0);
}
void chose_handler(int num){
    if (num==15) sigterm_handler();
    if (num==10 || num==30 || num==16) sigusr1_handler();
    if (num==12 || num==31 || num==17) sigusr2_handler();
    if (num==14) sigalrm_handler();
}