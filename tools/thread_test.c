#include <stdio.h>
#include <pthread.h>

void * thread_func(void* __arg) {
    unsigned int* arg = __arg;
    printf("new Thread:my thread id is %u\n",*arg);
    usleep(500);
}

void main() {
    pthread_t new_thread_id;
    pthread_create(&new_thread_id, NULL, thread_func, &new_thread_id);
    printf("main thread:my thread id is %u\n",pthread_self());
    usleep(100);
}