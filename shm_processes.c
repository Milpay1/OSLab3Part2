#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <stdint.h>  // Include for intptr_t

#define NUM_ITERATIONS 25

void ParentProcess(int *sharedMemory);
void ChildProcess(int *sharedMemory);

int main() {
    int ShmID;
    int *ShmPTR;
    pid_t pid;
    int status;

    // Create shared memory for BankAccount and Turn
    ShmID = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0666);
    if (ShmID < 0) {
        printf("*** shmget error (server) ***\n");
        exit(1);
    }

    ShmPTR = (int *)shmat(ShmID, NULL, 0);
    if ((intptr_t)ShmPTR == -1) {  // Changed to intptr_t to avoid casting warning
        printf("*** shmat error (server) ***\n");
        exit(1);
    }

    // Initialize shared memory values
    ShmPTR[0] = 0; // BankAccount
    ShmPTR[1] = 0; // Turn

    pid = fork();
    if (pid < 0) {
        printf("*** fork error (server) ***\n");
        exit(1);
    } else if (pid == 0) {
        ChildProcess(ShmPTR);
        exit(0);
    } else {
        ParentProcess(ShmPTR);
    }

    wait(&status);

    // Cleanup shared memory
    shmdt((void *)ShmPTR);
    shmctl(ShmID, IPC_RMID, NULL);

    printf("Server exits...\n");
    exit(0);
}

void ParentProcess(int *sharedMemory) {
    int i, account, balance;

    srand(time(NULL));

    for (i = 0; i < NUM_ITERATIONS; i++) {
        sleep(rand() % 6);

        account = sharedMemory[0];

        // Wait for Turn to be 0
        while (sharedMemory[1] != 0);

        if (account <= 100) {
            balance = rand() % 101;
            if (balance % 2 == 0) {
                account += balance;
                printf("Dear old Dad: Deposits $%d / Balance = $%d\n", balance, account);
            } else {
                printf("Dear old Dad: Doesn't have any money to give\n");
            }
        } else {
            printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", account);
        }

        sharedMemory[0] = account;
        sharedMemory[1] = 1; // Set Turn to 1 for Child
    }
}

void ChildProcess(int *sharedMemory) {
    int i, account, balance;

    srand(time(NULL) + 1); // Different seed for child process

    for (i = 0; i < NUM_ITERATIONS; i++) {
        sleep(rand() % 6);

        account = sharedMemory[0];

        // Wait for Turn to be 1
        while (sharedMemory[1] != 1);

        balance = rand() % 51;
        printf("Poor Student needs $%d\n", balance);

        if (balance <= account) {
            account -= balance;
            printf("Poor Student: Withdraws $%d / Balance = $%d\n", balance, account);
        } else {
            printf("Poor Student: Not Enough Cash ($%d)\n", account);
        }

        sharedMemory[0] = account;
        sharedMemory[1] = 0; // Set Turn to 0 for Parent
    }
}
