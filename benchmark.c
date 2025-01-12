#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <sched.h>

int main() {
    struct timespec start, end;
    cpu_set_t set;
    int cpu = 0;
    int fd2[2];
    pid_t pid;
    char message[] = "Hello world!";
    char inbuf[100];  // Buffer to read from the pipe

    // Create a pipe
    if (pipe(fd2) == -1) {
        perror("pipe failed");
        exit(1);
    }

    // For benchmarking a system call
    int fd = open("foo.txt", O_RDONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("open failed");
        exit(1);
    }

    // Benchmarking the system call
    clock_gettime(CLOCK_MONOTONIC, &start);
    char buf[1];
    for (int i = 0; i < 1000; i++) {
        read(fd, buf, 1);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    close(fd);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("System call time: %f seconds\n", elapsed);

    // Set CPU affinity
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);

    if (sched_setaffinity(0, sizeof(set), &set) != 0) {
        perror("sched_setaffinity failed");
        return EXIT_FAILURE;
    }

    if (sched_getaffinity(0, sizeof(set), &set) != 0) {
        perror("sched_getaffinity failed");
        return EXIT_FAILURE;
    }

    // Fork a child process for pipe communication
    pid = fork();
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i <= 100; i++) {
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (pid == 0) {
            // Child process
            close(fd2[0]); 

            int childPID = getpid();
            write(fd2[1], message, strlen(message) + 1);
            close(fd2[1]);
            exit(0);
        } else {
            // Parent process
            close(fd2[1]);
            read(fd2[0], inbuf, sizeof(inbuf));
            close(fd2[0]);
            printf("Parent received: %s\n", inbuf);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Context switch time: %f seconds\n", elapsed);

    return 0;
}
