#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/fcntl.h>
#include <string>
#include <cstring>
#include <cmath>

void collatz(int num, int Fd);

int getNum(int Fd);

int fileOpener(const std::string& myFile, int modeSelector = 1);

std::string nameMaker(pid_t subject,pid_t parent);

void fileClose(int Fd);

void filePrinter(int Fd);

enum ModeSelector {
    READ, WRITE, READWRITE
};

int main(int argc, char *argv[]) {

    int n = 0, min = 0, max = 0, myNumber = 0;

    pid_t parentPid = getpid();

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
    } else if (n > 10) {
        printf("number of processes cannot exceed 10, please try again.\n");
    }

    //collecting job numbers for individual processes
    int range = (max+1 - min),
            leaders = (int) (ceil((double) n / 2)),
            followers = n - leaders,
            smShare = range / 3,
            bigShare = range - smShare,
            bigBite = (int)(ceil((double)bigShare / leaders)),
            smBite = smShare / followers;

    //testing output to check math
//    printf("n: %d min: %d max: %d\n", n, min, max);
//    printf("range: %d leaders: %d \n"
//           "smShare: %d bigShare: %d \n"
//           "bigBite: %d smBite: %d total: %d\n",
//           range, leaders, smShare, bigShare,
//           bigBite, smBite, (leaders * bigBite) + (followers * smBite));

    //creates / opens file for number pool / results and populated it
    int numberPool = 0;
    numberPool = fileOpener("number_Pool.dat",READWRITE);

    //write range into file to be accessed by child processes
    for (int i = min; i <= max; i++) {
        if (write(numberPool, &i, 4) == -1) {
            perror("write failed");
            exit(EXIT_FAILURE);
        }
    }

    //put number pool file in read only mode and rewind it back to start
    if(fcntl(numberPool,F_SETFL,O_RDONLY) == -1) {
        perror("unable to set file to read only mode\n");
        exit(EXIT_FAILURE);
    }    if(lseek(numberPool,0,SEEK_SET) == -1) {
        perror("unable to set file offset back to start of file\n");
        exit(EXIT_FAILURE);
    }





    printf("Creating %d processes:\n", n);

    //fork child processes to do sequences
    auto *children = new pid_t[n];   //dynamic array to hold pids of children forked off
    for (int i = 0; i < n; i++) {
        if ((children[i] = fork()) <= 0) {
            if (children[i] == -1) {
                perror("fork failed!");
                exit(EXIT_FAILURE);
            } else {
                myNumber = i + 1;
            }
            break;
        }
    }


    //screen children out and set their task count "stop"
    if (myNumber > 0) {
        //creates / opens file for sequence results from child processes
        int record = fileOpener(nameMaker((long)getpid(),(long)getppid()),
                                READWRITE);

        //set task list
        int stop = 0, low = max, high = min;
        if (myNumber <= leaders) {
            stop = bigBite;
        } else {
            stop = smBite;
        }

        //children do their assigned number of sequences
        for (int i = 0; i < stop; i++) {
            int num;
            num = getNum(numberPool);
            low = (num < low) ? num : low;
            high = (high < num) ? num : high;
            if (num > 1 && num <= max) {
                collatz(num, record);
            }

        }
        fileClose(record);
        printf("I am child number %ld, my parent is %ld, "
               "I computed the Collatz sequence for numbers from %d to %d\n",
               (long) getpid(), (long) getppid(), low, high);
    }

    //close file used for issuing numbers to children
    fileClose(numberPool);

    //cutoff for children
    if (myNumber > 0) { exit(0); }

    //parent waits for children to finish
    while (wait(nullptr) >= 0) {}

    if (getpid() == parentPid) {
        for (int i = 0; i < n; i++) {
            std::string name = nameMaker(children[i],getpid());
            int childFile = fileOpener(name,READ);
            filePrinter(childFile);
            fileClose(childFile);
        }
    }
    return 0;
}

//conducts collatz sequence with supplied seed value and writes it to file / console
void collatz(int num, int Fd) {
//    printf("%d ", num);
    write(Fd,&num,4);
    while (num > 1) {
        if (num % 2 == 0) {
            num = num / 2;
        } else if (num % 2 == 1) {
            num = 3 * (num) + 1;
        }
        write(Fd, &num, 4);
        write(Fd," ",1);
//        printf("%d ", num);
    }
    write(Fd,"\0",1);
//    printf("\n");
}

//collects a seed number off the script file and returns it
int getNum(int Fd) {
    int buff;
    read(Fd, &buff, 4);
    return buff;
}

//make name file out of pid supplied
std::string nameMaker(pid_t subject,pid_t parent) {
    return "results_"+std::to_string(subject)+"_"+
                         std::to_string(parent);
}

//opens file and checks for error
int fileOpener(const std::string& myFile,int modeSelector) {
    int Fd, openFlags, filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                                   S_IROTH | S_IWOTH; /* rw-rw-rw- */

    switch (modeSelector) {
        case READWRITE: {
            openFlags =  O_RDWR | O_CREAT | O_TRUNC;
            break;
        }
        case WRITE: {
            openFlags = O_WRONLY;
            break;
        }
        case READ: {
            openFlags = O_RDONLY;
            break;
        }
        default: {
            openFlags = O_RDONLY;
            break;
        }
    }
    if ((Fd = open(myFile.c_str(), openFlags, filePerms)) == -1) {
        perror("data file failed to open");
        printf("File: %s", myFile.c_str());
        exit(EXIT_FAILURE);
    }
    return Fd;
}

//closes file and checks for error
void fileClose(int Fd) {
    if ((close(Fd)) == -1) {
        perror("data file failed to close");
        exit(EXIT_FAILURE);
    }
}

//utility for printing out file contents for testing
void filePrinter(int Fd) {
    int buff = 0;
    ssize_t  numRead = 0;
    while((numRead = read(Fd, &buff, 4)) > 0) {
        if(write(STDOUT_FILENO,&buff,numRead) != numRead) {
            perror("unable to write whole buffer");
        }
        printf("buff = %d\n", buff);
        if(numRead == -1) { perror("file read error"); }
    }
}