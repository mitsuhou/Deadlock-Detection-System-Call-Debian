#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define SYS_register_resource 548
#define SYS_request_resource 549
#define SYS_release_resource 550
#define SYS_detect_deadlock 551

int main()
{
    srand(time(NULL) ^ getpid());
    int R = rand() % 10000 + 1000;

    // Register resource
    if (syscall(SYS_register_resource, R) < 0)
    {
        perror("Error registering R");
        exit(1);
    }
    printf("Sucessfully registering R.\n\n");
    fflush(stdout);

    // Process A
    pid_t pid1 = fork();
    if (pid1 == 0)
    {
        printf("[Process A:%d] Requesting R...\n", getpid());
        if (syscall(SYS_request_resource, R) < 0)
        {
            perror("Error Requesting R");
            exit(1);
        }
        printf("[Process A:%d] Acquired R\n", getpid());

        sleep(3);

        exit(0);
    }

    // Process B
    pid_t pid2 = fork();
    if (pid2 == 0)
    {
        printf("[Process B:%d] Requesting R...\n", getpid());
        if (syscall(SYS_request_resource, R) < 0)
        {
            perror("Error Requesting R");
            exit(1);
        }
        printf("[Process B:%d] Acquired R\n", getpid());

        sleep(3);

        exit(0);
    }

    sleep(5); 

    long is_deadlocked = syscall(SYS_detect_deadlock);

    if (is_deadlocked < 0)
    {
        perror("Syscall is malfunctioned\n");
    }
    else if (is_deadlocked == 1)
    {
        printf("Deadlock occurs!\n");
    }
    else
    {
        printf("The system is safe\n");
    }

    kill(pid1, SIGKILL);
    kill(pid2, SIGKILL);
    wait(NULL);
    wait(NULL);
    return 0;
}