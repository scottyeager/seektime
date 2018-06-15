#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>

#define BUFFERSIZE    512

void diep(const char *str) {
    perror(str);
    exit(EXIT_FAILURE);
}

void dies(const char *str) {
    fprintf(stderr, "[-] %s\n", str);
    exit(EXIT_FAILURE);
}

// returns time in nanoseconds
uint64_t systime() {
    /*
    struct timespec t;

    if(clock_gettime(CLOCK_MONOTONIC, &t) < 0)
        diep("clock_gettime");

    return (t.tv_sec * 1000000000) + t.tv_nsec;
    */
    struct timeval tv;

    if(gettimeofday(&tv, NULL) < 0)
        diep("gettimeofday");

    return (tv.tv_sec * 1000000) + tv.tv_usec;
}

uint64_t seektest(int fd, off_t offset) {
    uint64_t tinit, tend;
    void *buffer;

    // checking last byte and
    // force move on the beginin of the disk
    off_t last = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    // purge full disk from cache
    if(posix_fadvise(fd, 0, last, POSIX_FADV_DONTNEED))
        diep("posix_fadvise");

    // allocate aligned memory
    // required by O_DIRECT flag
    if(posix_memalign(&buffer, 512, BUFFERSIZE))
        dies("posix_memalign");

    if(read(fd, buffer, BUFFERSIZE) < 0)
        diep("read init");

    // starting timing
    tinit = systime();

    // moving to the end of the disk
    // and reading one sector
    lseek(fd, -offset, SEEK_END);
    if(read(fd, buffer, BUFFERSIZE) < 0)
        diep("read end");

    // computing time elapsed
    tend = systime();

    free(buffer);

    return tend - tinit;
}

int main(int argc, char *argv[]) {
    int fd;

    if(argc != 2) {
        fprintf(stderr, "Usage: %s device\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // if((fd = open(argv[1], O_DIRECT | O_SYNC | O_RDONLY)) < 0)
    if((fd = open(argv[1], O_SYNC | O_RDONLY)) < 0)
        diep(argv[1]);

    size_t checked = 0;
    uint64_t fulltime = 0;

    srand(time(NULL));

    for(checked = 0; checked < 128; checked += 1) {
        // let's do the test one time
        uint64_t thistime = seektest(fd, rand() % 512 * 1024 * 1024);
        // printf("[+] seektime: %lu us\n", thistime);

        fulltime += thistime;
        checked += 1;
    }

    uint64_t elapsed = fulltime / checked;
    // printf("[+] average: %lu us\n", elapsed);

    if(elapsed > 500)
        printf("HDD (%lu us)\n", elapsed);

    else if(elapsed < 500)
        printf("SSD (%lu us)\n", elapsed);

    close(fd);

    return 0;
}
