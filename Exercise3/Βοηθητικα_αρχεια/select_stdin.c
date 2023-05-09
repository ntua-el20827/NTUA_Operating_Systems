#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <sys/select.h>
#include <stdbool.h>
#include <ctype.h>


bool isNumber(char number[]){
    int i = 0;
    //checking for negative numbers
    if (number[0] == '-')
        i = 1;
    for (; number[i] != 0; i++){
        //if (number[i] > '9' || number[i] < '0')
        if (!isdigit(number[i]))
            return false;
    }
    return true;
}
int main() {
    fd_set readfds;

    while (1) {
        // initialize the file descriptor set
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        // wait for input on stdin
        int ready = select(STDIN_FILENO+1, &readfds, NULL, NULL, NULL);
        if (ready == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        } else if (ready > 0) {
            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                char buf[1024];
                int n = read(STDIN_FILENO, buf, sizeof(buf));
                //buf[n] = '\0';
                printf(buf);
                buf[n-1] = '\0';
                if (strcmp(buf, "help") == 0) {
                    printf("Type a number to send job to a child!\n");
                }
            }
        }
    }
    exit(EXIT_SUCCESS);
}