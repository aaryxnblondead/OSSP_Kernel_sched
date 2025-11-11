/*
 * test_io.c - I/O-bound test program
 * 
 * This program creates I/O-intensive workload to test scheduler behavior
 * with I/O-heavy processes that frequently block and wake up.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#define DEFAULT_DURATION 10
#define BUFFER_SIZE 4096
#define TEMP_FILE_PREFIX "/tmp/io_test_"

unsigned long long read_count = 0;
unsigned long long write_count = 0;

/*
 * Perform I/O operations
 */
void perform_io_operations(const char *filename, int duration) {
    int fd;
    char buffer[BUFFER_SIZE];
    struct timeval start, end;
    
    // Create test file
    fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create test file");
        return;
    }
    
    // Fill buffer with data
    memset(buffer, 'A', BUFFER_SIZE);
    
    printf("Starting I/O operations (PID: %d)\n", getpid());
    printf("Test file: %s\n", filename);
    
    gettimeofday(&start, NULL);
    gettimeofday(&end, NULL);
    
    // Perform I/O operations for specified duration
    while ((end.tv_sec - start.tv_sec) < duration) {
        // Write operation
        if (write(fd, buffer, BUFFER_SIZE) == BUFFER_SIZE) {
            write_count++;
        }
        
        // Seek back to beginning
        lseek(fd, 0, SEEK_SET);
        
        // Read operation
        if (read(fd, buffer, BUFFER_SIZE) == BUFFER_SIZE) {
            read_count++;
        }
        
        // Force sync to disk (causes more blocking)
        fsync(fd);
        
        // Small sleep to simulate realistic I/O pattern
        usleep(1000); // 1ms
        
        gettimeofday(&end, NULL);
    }
    
    close(fd);
    unlink(filename); // Clean up
}

/*
 * Display statistics
 */
void display_stats(int duration) {
    double reads_per_sec = (double)read_count / duration;
    double writes_per_sec = (double)write_count / duration;
    double total_mb = ((read_count + write_count) * BUFFER_SIZE) / (1024.0 * 1024.0);
    
    printf("\n=== I/O-Bound Test Results ===\n");
    printf("Duration: %d seconds\n", duration);
    printf("Total reads: %llu\n", read_count);
    printf("Total writes: %llu\n", write_count);
    printf("Reads per second: %.2f\n", reads_per_sec);
    printf("Writes per second: %.2f\n", writes_per_sec);
    printf("Total data transferred: %.2f MB\n", total_mb);
    printf("==============================\n");
}

int main(int argc, char *argv[]) {
    int duration = DEFAULT_DURATION;
    char filename[256];
    
    // Parse arguments
    if (argc > 1) {
        duration = atoi(argv[1]);
        if (duration < 1 || duration > 300) {
            fprintf(stderr, "Duration must be between 1 and 300 seconds\n");
            return 1;
        }
    }
    
    // Create unique temp filename
    snprintf(filename, sizeof(filename), "%s%d", TEMP_FILE_PREFIX, getpid());
    
    printf("=== I/O-Bound Test Program ===\n");
    printf("PID: %d\n", getpid());
    printf("Duration: %d seconds\n", duration);
    printf("Buffer size: %d bytes\n", BUFFER_SIZE);
    printf("Use: cat /proc/sched_stats to monitor scheduler behavior\n\n");
    
    // Perform I/O operations
    perform_io_operations(filename, duration);
    
    // Display results
    display_stats(duration);
    
    return 0;
}
