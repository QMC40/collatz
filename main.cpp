#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/fcntl.h>
#include <string>

void kidz();

void collatz(int num);

int getNum();

int min, max;


int main(int argc, char *argv[]) {

    int n, record;

    //getting entering args into vars
    if (argc < 3) {
        fprintf(stderr, "missing command line arguments, please try again.\n");
        return 1;
    }
    n = atoi(argv[1]);
    min = atoi(argv[2]);
    max = atoi(argv[3]);
    if (n <= 0 || min <= 0 || max <= 0) {
        printf("command line arguments must be positive integers, please try again.\n");
        return 1;
    }
    printf("n: %d min: %d max: %d\n", n, min, max);

    pid_t parentPid = getpid(), childPid = 0;

    printf("parent is: %d\n", parentPid);

    //open file to write results too
    if ((record = open("data.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
        perror("data file failed to open");
    }

    //write range into file to be accessed by child processes
    int buff;
    for (int i = min; i <= max; i++) {
        write(record, &i, 4);
        read(record, &buff, 4);
    }
    //rewind offset to first byte of file, could also just close and reopen file...
    lseek(record, 0, SEEK_SET);

//    for (int i = min; i <= max; i++) {
//        read(record, &buff, 4);
//        printf("buff = %d\n", buff);
//    }


    for (int i = 0; i < n; i++) {
        if ((childPid = fork()) <= 0) {
            if (childPid == -1) {
                printf("fork failed!\n");
                return 1;
            } else {
//                kidz();
                break;
            }
        }
    }
//    dup2(oldOut,STDOUT_FILENO);
//    while (wait(nullptr) >= 0) {}
//
//    if (getpid() == parentPid) {
//        printf("Parent %d process is completed.\n", getpid());
//    }

    if ((record = close(record)) == -1) {
        perror("data file failed to close");
    }

    return 0;
}

void kidz() {
    pid_t myID = getpid();
    //segregate parent from children
    if (myID != 0) {
        printf("Child %d is working...\n", myID);
        collatz(getNum());
        printf("\n");
        printf("Child %d process is completed.\n", myID);
    }
}

void collatz(int num) {
    /*this is for check and print all positive
      numbers will finally reach 1*/
    printf("collatz\n");
    while (num != 1) {
        if (num % 2 == 0) {
            num = num / 2;
        } else if (num % 2 == 1) {
            num = 3 * (num) + 1;
        }
        printf("%d ", num);
    }
}

int getNum() {
    printf("getNum min: %d\n", min);
    while (min <= max) { return min++; }
    return -1;
}