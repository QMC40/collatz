#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>

int main(int argc, char *argv[]) {

    int n, min, max;
    //getting entering args into vars
    if (argc < 3) {
        printf("missing command line arguments, please try again.\n");
        return 1;
    } else {
        long conv1 = strtol(argv[1], nullptr, 10);
        long conv2 = strtol(argv[2], nullptr, 10);
        long conv3 = strtol(argv[3], nullptr, 10);
        if (conv1 <= 0 || conv2 <= 0 || conv3 <= 0) {
            printf("command line arguments must be positive integers, please try again.\n");
            return 1;
        }
        n = (int) conv1, min = (int) conv2, max = (int) conv3;
    }
//    printf("n: %d min: %d max: %d\n", n, min, max);

    pid_t P = getpid();
    pid_t p;
    while (n) {
//        printf("max currently: %d\n", max);
        p = fork();
        if (p == -1) {
            printf("fork failed!\n");
            return 1;
        }
        n--, max--;

    }

    //segregate parent from children
    if (p == 0) {
        printf("Child %d is working...\n", getpid());
        /*this is for check and print all positive
        numbers will finally reach 1*/
        int num = max;
        while (num != 1) {
            if (num % 2 == 0) {
                num = num / 2;
            } else if (num % 2 == 1) {
                num = 3 * (num) + 1;
            }
            printf("%d ", num);
        }
        printf("\n");
        printf("Child %d process is completed.\n", getpid());
    } else {
//        printf("Parents process is waiting on child process...\n");
        //call the waiting function
        wait(nullptr);

    }
    if (getpid()==P) {
        printf("Parent %d process is completed.\n", getpid());
    }
    return 0;
}