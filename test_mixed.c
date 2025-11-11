/*
 * test_mixed.c - Mixed workload test program
 * 
 * This program creates a mixed workload with both CPU-intensive and 
 * I/O-intensive threads to test scheduler behavior under realistic conditions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>

#define DEFAULT_DURATION 10
#define BUFFER_SIZE 4096

volatile int keep_running = 1;

// Statistics
typedef struct {
    unsigned long long cpu_operations;
    unsigned long long io_reads;
    unsigned long long io_writes;
    pthread_mutex_t mutex;
} stats_t;

stats_t global_stats = {0, 0, 0, PTHREAD_MUTEX_INITIALIZER};

/*
 * CPU-intensive worker thread
 */
void* cpu_worker(void *arg) {
    int thread_id = *(int*)arg;
    unsigned long long local_ops = 0;
    unsigned long long result = 0;
    
    printf("[CPU Thread %d] Starting (PID: %d, TID: %ld)\n", 
           thread_id, getpid(), pthread_self());
    
    while (keep_running) {
        // CPU-intensive computation
        for (int i = 0; i < 50000; i++) {
            result += i * i;
            result = result % 1000000;
        }
        local_ops++;
    }
    
    pthread_mutex_lock(&global_stats.mutex);
    global_stats.cpu_operations += local_ops;
    pthread_mutex_unlock(&global_stats.mutex);
    
    printf("[CPU Thread %d] Completed %llu operations\n", thread_id, local_ops);
    free(arg);
    return NULL;
}

/*
 * I/O-intensive worker thread
 */
void* io_worker(void *arg) {
    int thread_id = *(int*)arg;
    char filename[256];
    char buffer[BUFFER_SIZE];
    int fd;
    unsigned long long local_reads = 0;
    unsigned long long local_writes = 0;
    
    snprintf(filename, sizeof(filename), "/tmp/io_mixed_%d_%d", getpid(), thread_id);
    memset(buffer, 'B', BUFFER_SIZE);
    
    printf("[I/O Thread %d] Starting (PID: %d, TID: %ld)\n", 
           thread_id, getpid(), pthread_self());
    
    fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create I/O test file");
        free(arg);
        return NULL;
    }
    
    while (keep_running) {
        // Write operation
        if (write(fd, buffer, BUFFER_SIZE) == BUFFER_SIZE) {
            local_writes++;
        }
        
        // Seek and read
        lseek(fd, 0, SEEK_SET);
        if (read(fd, buffer, BUFFER_SIZE) == BUFFER_SIZE) {
            local_reads++;
        }
        
        // Sync to cause blocking
        fsync(fd);
        
        // Sleep to simulate I/O wait
        usleep(5000); // 5ms
    }
    
    close(fd);
    unlink(filename);
    
    pthread_mutex_lock(&global_stats.mutex);
    global_stats.io_reads += local_reads;
    global_stats.io_writes += local_writes;
    pthread_mutex_unlock(&global_stats.mutex);
    
    printf("[I/O Thread %d] Completed %llu reads, %llu writes\n", 
           thread_id, local_reads, local_writes);
    free(arg);
    return NULL;
}

/*
 * Display statistics
 */
void display_stats(int duration, int cpu_threads, int io_threads) {
    double cpu_ops_per_sec = (double)global_stats.cpu_operations / duration;
    double io_ops_per_sec = (double)(global_stats.io_reads + global_stats.io_writes) / duration;
    double total_io_mb = ((global_stats.io_reads + global_stats.io_writes) * BUFFER_SIZE) / (1024.0 * 1024.0);
    
    printf("\n=== Mixed Workload Test Results ===\n");
    printf("Duration: %d seconds\n", duration);
    printf("CPU threads: %d\n", cpu_threads);
    printf("I/O threads: %d\n", io_threads);
    printf("\nCPU Workload:\n");
    printf("  Total operations: %llu\n", global_stats.cpu_operations);
    printf("  Operations/sec: %.2f\n", cpu_ops_per_sec);
    printf("\nI/O Workload:\n");
    printf("  Total reads: %llu\n", global_stats.io_reads);
    printf("  Total writes: %llu\n", global_stats.io_writes);
    printf("  I/O operations/sec: %.2f\n", io_ops_per_sec);
    printf("  Total data transferred: %.2f MB\n", total_io_mb);
    printf("===================================\n");
}

int main(int argc, char *argv[]) {
    int cpu_threads = 2;
    int io_threads = 2;
    int duration = DEFAULT_DURATION;
    pthread_t *threads;
    int total_threads;
    struct timeval start, end;
    
    // Parse arguments
    if (argc > 1) {
        cpu_threads = atoi(argv[1]);
    }
    if (argc > 2) {
        io_threads = atoi(argv[2]);
    }
    if (argc > 3) {
        duration = atoi(argv[3]);
    }
    
    if (cpu_threads < 0 || io_threads < 0 || 
        cpu_threads + io_threads < 1 || cpu_threads + io_threads > 32) {
        fprintf(stderr, "Invalid thread counts (total must be 1-32)\n");
        return 1;
    }
    
    if (duration < 1 || duration > 300) {
        fprintf(stderr, "Duration must be between 1 and 300 seconds\n");
        return 1;
    }
    
    total_threads = cpu_threads + io_threads;
    
    printf("=== Mixed Workload Test Program ===\n");
    printf("PID: %d\n", getpid());
    printf("CPU-intensive threads: %d\n", cpu_threads);
    printf("I/O-intensive threads: %d\n", io_threads);
    printf("Duration: %d seconds\n", duration);
    printf("Use: cat /proc/sched_stats to monitor scheduler behavior\n\n");
    
    // Allocate thread array
    threads = malloc(total_threads * sizeof(pthread_t));
    if (!threads) {
        perror("Failed to allocate thread array");
        return 1;
    }
    
    gettimeofday(&start, NULL);
    
    // Create CPU worker threads
    for (int i = 0; i < cpu_threads; i++) {
        int *thread_id = malloc(sizeof(int));
        *thread_id = i;
        
        if (pthread_create(&threads[i], NULL, cpu_worker, thread_id) != 0) {
            perror("Failed to create CPU thread");
            free(thread_id);
        }
    }
    
    // Create I/O worker threads
    for (int i = 0; i < io_threads; i++) {
        int *thread_id = malloc(sizeof(int));
        *thread_id = i;
        
        if (pthread_create(&threads[cpu_threads + i], NULL, io_worker, thread_id) != 0) {
            perror("Failed to create I/O thread");
            free(thread_id);
        }
    }
    
    // Run for specified duration
    sleep(duration);
    keep_running = 0;
    
    // Wait for all threads to complete
    for (int i = 0; i < total_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    gettimeofday(&end, NULL);
    
    int actual_duration = (end.tv_sec - start.tv_sec);
    display_stats(actual_duration, cpu_threads, io_threads);
    
    free(threads);
    return 0;
}
