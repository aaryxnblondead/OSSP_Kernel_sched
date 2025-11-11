/*
 * test_cpu.c - CPU-bound test program
 * 
 * This program creates CPU-intensive workload to test scheduler behavior
 * with compute-heavy processes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#define DEFAULT_THREADS 4
#define DEFAULT_DURATION 10

volatile int keep_running = 1;
unsigned long long operations = 0;
pthread_mutex_t ops_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * CPU-intensive computation
 */
void* cpu_worker(void *arg) {
    int thread_id = *(int*)arg;
    unsigned long long local_ops = 0;
    unsigned long long result = 0;
    
    printf("[Thread %d] Starting CPU-intensive work (PID: %d, TID: %ld)\n", 
           thread_id, getpid(), pthread_self());
    
    while (keep_running) {
        // Perform CPU-intensive calculations
        for (int i = 0; i < 100000; i++) {
            result += i * i;
            result = result % 1000000;
        }
        local_ops++;
    }
    
    pthread_mutex_lock(&ops_mutex);
    operations += local_ops;
    pthread_mutex_unlock(&ops_mutex);
    
    printf("[Thread %d] Completed %llu operations\n", thread_id, local_ops);
    free(arg);
    return NULL;
}

/*
 * Display statistics
 */
void display_stats(int duration, int num_threads) {
    double ops_per_sec = (double)operations / duration;
    double ops_per_thread = (double)operations / num_threads;
    
    printf("\n=== CPU-Bound Test Results ===\n");
    printf("Duration: %d seconds\n", duration);
    printf("Number of threads: %d\n", num_threads);
    printf("Total operations: %llu\n", operations);
    printf("Operations per second: %.2f\n", ops_per_sec);
    printf("Operations per thread: %.2f\n", ops_per_thread);
    printf("==============================\n");
}

int main(int argc, char *argv[]) {
    int num_threads = DEFAULT_THREADS;
    int duration = DEFAULT_DURATION;
    pthread_t *threads;
    struct timeval start, end;
    
    // Parse arguments
    if (argc > 1) {
        num_threads = atoi(argv[1]);
        if (num_threads < 1 || num_threads > 64) {
            fprintf(stderr, "Number of threads must be between 1 and 64\n");
            return 1;
        }
    }
    if (argc > 2) {
        duration = atoi(argv[2]);
        if (duration < 1 || duration > 300) {
            fprintf(stderr, "Duration must be between 1 and 300 seconds\n");
            return 1;
        }
    }
    
    printf("=== CPU-Bound Test Program ===\n");
    printf("PID: %d\n", getpid());
    printf("Starting %d CPU-intensive threads for %d seconds\n", 
           num_threads, duration);
    printf("Use: cat /proc/sched_stats to monitor scheduler behavior\n\n");
    
    // Allocate thread array
    threads = malloc(num_threads * sizeof(pthread_t));
    if (!threads) {
        perror("Failed to allocate thread array");
        return 1;
    }
    
    gettimeofday(&start, NULL);
    
    // Create worker threads
    for (int i = 0; i < num_threads; i++) {
        int *thread_id = malloc(sizeof(int));
        *thread_id = i;
        
        if (pthread_create(&threads[i], NULL, cpu_worker, thread_id) != 0) {
            perror("Failed to create thread");
            free(thread_id);
        }
    }
    
    // Run for specified duration
    sleep(duration);
    keep_running = 0;
    
    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    gettimeofday(&end, NULL);
    
    int actual_duration = (end.tv_sec - start.tv_sec);
    display_stats(actual_duration, num_threads);
    
    free(threads);
    return 0;
}
