#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/fcntl.h>
#include <string>
#include <semaphore.h>

void kidz();
void collatz(int num);
int getNum();

int min,max;
sem_t next;     //semaphore for picking numbers


int main(int argc, char *argv[]) {

    int n, record, index, p, semid;

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
    if ((record = open("data.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) { perror
    ("data file failed to open"); }

//    int oldOut = STDOUT_FILENO;
//    dup2(record,STDOUT_FILENO);
    //write range into file to be accessed by child processes
    for(int i = min ; i <= max ; i++) {
        write(record,&i,sizeof(int));
    }
    for(int i = min ; i <= max ; i++) {
        int buff;
        read(record,&buff, sizeof(int));
        printf("%d\n",buff);
    }


    sem_init(&next,3,1);

    for (int i = 0; i < n; i++) {
        if ((childPid = fork()) <= 0) {
            if (childPid == -1) {
                printf("fork failed!\n");
                return 1;
            } else {
                kidz();
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

    sem_destroy(&next);
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
    int queue;
    printf("in getNum: %d \n",sem_getvalue(&next,&queue));
    sem_wait(&next);
    printf("getNum min: %d\n",min);
    while(min <= max) { return min++; }
    sem_post(&next);
    return -1;
}