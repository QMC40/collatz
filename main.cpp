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

int fileOpener(const std::string &myFile, int modeSelector = 1);

std::string nameMaker(pid_t subject, pid_t parent);

void fileClose(int Fd);

void resultDispenser(pid_t children[], int n);

void filePrinter(int Fd);

void stageSetter (struct Numbers *subj);

void stageChecker(struct Numbers *subj);

enum ModeSelector {
    READ, WRITE, READWRITE
};

//created struct purely for decluttering code, instance is created, numbers calculated and instance is deleted
//early in code to prevent children from inheriting and waste resources any more than needed
struct Numbers {
    int n = 1,
    min = 0,
    max = 0,
    leaders = 0,
    bigBite = 0,
    smBite = 0,
    range = 0,
    smShare = 0,
    bigShare = 0,
    followers = 0;

    Numbers(int argc,char *argv[]) {

        n = atoi(argv[1]);
        min = atoi(argv[2]);
        max = atoi(argv[3]);

    }
};
//record keeper for parent process
struct Debrief{
    pid_t pid;
    int status;
};

int main(int argc, char *argv[]) {

    //getting entering args into vars and setting working variables
    if (argc < 4) {
        fprintf(stderr, "missing command line arguments, please try again.\n");
        return 1;
    }
    auto *args = new Numbers(argc,argv);
    if (args->n <= 0 || args->min < 0 || args->max <= 0) {
        printf("command line arguments must be positive integers, please try again.\n");
        return 1;
    } else if (args->n > 10) {
        printf("number of processes cannot exceed 10, please try again.\n");
        return 1;
    }
    else if (args->max < args->min) {
        printf("minimum of range should be less than maximum, please try again");
        return 1;
    }

    //send numbers to stageSetter to do the math
    stageSetter(args);
    //put the results in local vars
    int n = args->n, min = args->min, max = args->max, leaders = args->leaders, bigBite = args->bigBite,
    smBite = args->smBite, myNumber = 0;
    //delete the struct before forking
    delete(args);

    //creates / opens temp file for number pool
    char filename[] = "numberPool_temp_file_XXXXXX";
    int numberPool = 0;
    numberPool = mkstemp(filename);

    //write range into file to be accessed by child processes
    for (int i = min; i <= max; i++) {
        if (write(numberPool, &i, 4) == -1) {
            perror("number pool write failed");
            exit(EXIT_FAILURE);
        }
    }

    //put number pool file in read only mode and rewind it back to start
    if (fcntl(numberPool, F_SETFL, O_RDONLY) == -1) {
        perror("unable to set file to read only mode\n");
        exit(EXIT_FAILURE);
    }
    if (lseek(numberPool, 0, SEEK_SET) == -1) {
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
        //creates / opens file for sequence results from child process
        int record = fileOpener(nameMaker((long)getpid(), (long)getppid()),
                                READWRITE);

        //set task list for each child process
        int stop = 0, low = max, high = min;
        //segregate first 50% of processes and assign stop # as appropriate
        if (myNumber <= leaders) {
            stop = (bigBite > 0) ? bigBite : 1;
        } else {
            stop = smBite;
        }

        //children do their assigned number of sequences
        int last = -1;
        for (int i = 0; i < stop; i++) {
            int num;
            //get next number on the list
            num = getNum(numberPool);
            //set low point of processes assigned range
            low = (num < low) ? num : low;
            //set high point
            high = (high < num) ? num : high;
            //escape hatch for situations where the task distribution leaves
            //odd last distribution and prevents duplicate runs of sequence
               if(num != last) {
                    collatz(num, record);
                    last = num;
                }

            //adjust low to 1 if range entered as zero
            low = (low == 0) ? 1 : low;

        }
        fileClose(record);
        printf("I am child number %ld, my parent is %ld, "
               "I computed the Collatz sequence for numbers from %d to %d\n",
               (long) getpid(), (long) getppid(), low, high);

        //close and release temp file used for issuing numbers to children
        fileClose(numberPool);
        unlink(filename);

    } else {
        //parent waits for children to finish storing results until all child processes exit
        //array to collect exit status as children exit to allow printout when all are completed
        auto *collection = new struct Debrief[n];
        for (int i = 0; i < n; i++) {
            collection[i].pid = waitpid(children[i], &collection[i].status, 0);
        }

        printf("\nProcesses completing: \n");

        for(int i = 0; i < n; i++) {
            printf("Process that has PID %ld exited with status 0x%x.\n", (long) collection[i].pid,
                   collection[i].status);
        }

        printf("\nResults:\n");

        resultDispenser(children, n);

        //delete dynamic storage
        delete[](collection);
        delete[](children);
    }
    return 0;
}

//conducts collatz sequence with supplied seed value and writes it to file
void collatz(int num, int Fd) {
    bool go = true;
    do {
        if (write(Fd, &num, 4) == -1) {
            perror("write result file failed");
            exit(EXIT_FAILURE);
        }
        if(num != 1)
            { num = (num % 2 == 0)? (num / 2) : (3 * (num) + 1); }
        else
            { go = false; }
    }while (go);
}

//collects a seed number off the script file and returns it
int getNum(int Fd) {
    int buff;
    if (read(Fd, &buff, 4) == -1) {
        perror("can't read number pool file!");
        exit(EXIT_FAILURE);
    }
    return buff;
}

//make name file out of pid info supplied
std::string nameMaker(pid_t subject, pid_t parent) {
    return "results_" + std::to_string(subject) + "_" +
           std::to_string(parent);
}

//opens file and checks for error
int fileOpener(const std::string &myFile, int modeSelector) {
    int Fd, openFlags, filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                                   S_IROTH | S_IWOTH; /* rw-rw-rw- */

    switch (modeSelector) {
        case READWRITE: {
            openFlags = O_RDWR | O_CREAT | O_TRUNC;
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
            //no default behavior defined
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

//cycles through children's results files printing them
void resultDispenser(pid_t children[], int n) {
    for (int i = 0; i < n; i++) {
        std::string name = nameMaker(children[i], getpid());
        int childFile = fileOpener(name, READ);
        filePrinter(childFile);
        fileClose(childFile);
    }
}

//utility for printing out file contents for testing
void filePrinter(int Fd) {
    int buff = 0;
    ssize_t numRead = 0;
    while ((numRead = read(Fd, &buff, 4)) > 0) {

        printf("%d ", buff);
        if (buff == 1) {
            printf("\n");
        }
    }
    if (numRead == -1) {
        perror("can't read result file!");
        exit(EXIT_FAILURE);
    }
}

//utility takes args and calcs stopping points for processes
void stageSetter (struct Numbers *subj) {

    //collecting job numbers for individual processes
    //check if single process requested, otherwise do the math
    int temp;
    if (subj->n > 1) {
        //adjust min if 0 to correct short calculation of range for sequences
        subj->min = ((subj->min == 0) ? 1 : subj->min),
        subj->range = (subj->max + 1 - subj->min),
        //calculate population of first 50% of processes, rounding up
        subj->leaders = (int) (round((double) subj->n / 2)),
        //if a process isn't a leader it's a follower
        subj->followers = subj->n - subj->leaders,
        //share to be done by followers
        subj->smShare = subj->range / 3,
        //share to be done by leaders
        subj->bigShare = subj->range - subj->smShare,
        subj->bigBite = (int) (round( (double) subj->bigShare / subj->leaders) ),
        temp = subj->smShare / subj->followers;
        subj->smBite = (temp < 1)? 1 : temp;
    } else {
        subj->min = ((subj->min == 0) ? 1 : subj->min),
        subj->range = (subj->max + 1 - subj->min),
        subj->leaders = 1, subj->bigBite = subj->range;
    }
    //calls print out of stats for run for testing
//    stageChecker(subj);
}

//utility prints calculations for stopping points and vars
void stageChecker(struct Numbers * subj) {
    //testing output to check math
    printf("n: %d min: %d max: %d\n", subj->n, subj->min, subj->max);
    printf("range: %d leaders: %d \n"
           "bigShare: %d smShare: %d\n"
           "bigBite: %d smBite: %d total: %d\n\n",
           subj->range, subj->leaders, subj->bigShare, subj->smShare,
           subj->bigBite, subj->smBite, (subj->leaders * subj->bigBite) + (subj->followers * subj->smBite));
}