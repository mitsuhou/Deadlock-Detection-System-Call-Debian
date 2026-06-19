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
    int R1 = rand() % 10000 + 1000;
    int R2 = rand() % 10000 + 10000;

    // Register resource
    if (syscall(SYS_register_resource, R1) < 0)
    {
        perror("Error registering R1");
        exit(1);
    }
    if (syscall(SYS_register_resource, R2) < 0)
    {
        perror("Error registering R2");
        exit(1);
    }
    printf("Sucessfully registering R1 and R2.\n\n");
    fflush(stdout);

    // Process A
    pid_t pid1 = fork();
    if (pid1 == 0)
    {
        printf("[Process A:%d] Requesting R1...\n", getpid());
        if (syscall(SYS_request_resource, R1) < 0)
        {
            perror("Error Requesting R1");
            exit(1);
        }
        printf("[Process A:%d] Acquired R1\n", getpid());

        sleep(3);

        printf("[Process A:%d] Requesting R2...\n", getpid());
        if (syscall(SYS_request_resource, R2) < 0)
        {
            perror("Error Requesting R2");
            exit(1);
        }
        printf("[Process A:%d] Acquired R2\n", getpid());
        exit(0);
    }

    // Process B
    pid_t pid2 = fork();
    if (pid2 == 0)
    {
        printf("[Process B:%d] Requesting R2...\n", getpid());
        if (syscall(SYS_request_resource, R2) < 0)
        {
            perror("Error Requesting R2");
            exit(1);
        }
        printf("[Process B:%d] Acquired R2\n", getpid());

        sleep(3);

        printf("[Process B:%d] Requesting R1...\n", getpid());
        if (syscall(SYS_request_resource, R1) < 0)
        {
            perror("Error Requesting R1");
            exit(1);
        }
        printf("[Process B:%d] Acquired R1\n", getpid());
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