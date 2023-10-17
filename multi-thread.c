#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

// Function to be executed by the first thread
void *thread1_function(void *arg) {
    srand(time(NULL)); // Initialize random seed
    for (int i = 1; i <= 5; i++) {
        printf("Thread 1: Message %d\n", i);
        fflush(stdout); // Flush the output buffer to ensure immediate printing
        usleep(rand() % 1000000); // Sleep for a random period of up to 1 second
    }
    return NULL;
}

// Function to be executed by the second thread
void *thread2_function(void *arg) {
    srand(time(NULL)); // Initialize random seed
    for (int i = 1; i <= 5; i++) {
        printf("Thread 2: Message %d\n", i);
        fflush(stdout); // Flush the output buffer to ensure immediate printing
        usleep(rand() % 1000000); // Sleep for a random period of up to 1 second
    }
    return NULL;
}

int main() {
    pthread_t thread1, thread2;

    // Create two threads
    pthread_create(&thread1, NULL, thread1_function, NULL);
    pthread_create(&thread2, NULL, thread2_function, NULL);

    // Wait for both threads to finish
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return 0;
}
