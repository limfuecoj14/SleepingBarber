#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// The maximum number of customer threads.
#define MAX_CUSTOMERS 25

//prototypes
void *customer(void *num);
void *barber(void *);
void randwait(int secs);

// define the semaphores respectively: # of customers, the mutex chair, sleep pillow, and finally the wait
sem_t waitingRoom, barberChair, barberPillow, belt;
// the condition variable
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;
// flag to stop the barber thread when all customers have been serviced.
int allDone = 0;


int main(int argc, char *argv[]) {
    pthread_t btid;
    pthread_t tid[MAX_CUSTOMERS];
    long RandSeed;
    int i, numCustomers, numChairs;
    int Number[MAX_CUSTOMERS];


    // check number of command line arguments.
    if (argc != 4) {
        printf("Use: SleepBarber <Num Customers> <Num Chairs> <rand seed>\n");
        exit(-1);
    }
    // convert command Line into ints
    numCustomers = atoi(argv[1]);
    numChairs = atoi(argv[2]);
    RandSeed = atol(argv[3]);

    // Ensure # of threads is less than the customers.
    if (numCustomers > MAX_CUSTOMERS) {
        printf("The maximum number of Customers is %d.\n", MAX_CUSTOMERS);
        exit(-1);
    }
    printf("A solution to the sleeping barber problem using semaphores and condition variables.\n");

    // rng w/new seed.
    srand48(RandSeed);

    // numbers array.
    for (i=0; i<MAX_CUSTOMERS; i++) {
        Number[i] = i;
    }

    sem_init(&waitingRoom, 0, numChairs);
    sem_init(&barberChair, 0, 1);
    sem_init(&barberPillow, 0, 0);
    sem_init(&belt, 0, 0);

    // barber and customer threads
    pthread_create(&btid, NULL, barber, NULL);
    for (i=0; i<numCustomers; i++) {
        pthread_create(&tid[i], NULL, customer, (void *)&Number[i]);
    }

    for (i=0; i<numCustomers; i++) {
        pthread_join(tid[i],NULL);
    }

    // "kill" the barber when done.
    allDone = 1;
    sem_post(&barberPillow);  // Wake the barber so he will exit.
    pthread_join(btid,NULL);
}

void *customer(void *number) {
    int num = *(int *)number;

    // go to shop and take some random amount of time to arrive.
    printf("Customer %d leaving for barber shop.\n", num);
    randwait(5);
    printf("Customer %d arrived at barber shop.\n", num);

    // open up in the waiting room...
    sem_wait(&waitingRoom);
    printf("Customer %d entering waiting room.\n", num);

    // wat for chair to be free.
    sem_wait(&barberChair);

    // chair is free == give up spot in waiting room.
    sem_post(&waitingRoom);

    // wake barber...
    printf("Customer %d waking the barber.\n", num);
    pthread_mutex_lock(&m);
    sem_post(&barberPillow);
    pthread_cond_wait(&c,&m);

    // wait to finish cutting your hair.
    sem_wait(&belt);

    // give up chair.
    sem_post(&barberChair);
    printf("Customer %d leaving barber shop.\n", num);
    pthread_mutex_unlock(&m);
    return NULL;
}

void *barber(void *arg) {
    while (!allDone) {

        // Sleep until someone arrives and wakes you..
        printf("The barber is sleeping\n");
        sem_wait(&barberPillow);

        // Skip this stuff at the end...
        if (!allDone) {

            // Take a random amount of time to cut the
            // customer's hair.

            printf("The barber is cutting hair\n");
            randwait(3);
            printf("The barber has finished cutting hair.\n");

            // release customer
            pthread_cond_signal(&c);
            sem_post(&belt);
        }
        else {
            printf("The barber is going home for the day.\n");
        }
    }
    return NULL;
}

// rng waiting
void randwait(int secs) {
    int len;

    // Generate a random number...
    len = (int) ((drand48() * secs) + 1);
    sleep(len);
}