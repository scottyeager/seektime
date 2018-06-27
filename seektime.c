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

// usage string for help
#define USAGE "Usage: %s [-j] device\n"

// amount of data to read on the
// disk after seeking, by default
// we read 512 bytes (one sector)
#define BUFFERSIZE  512

// number of loop reading the
// disk, we make an average after all
// the loops
#define DISKLOOP    128

#define TYPE_HDD "HDD"
#define TYPE_SDD "SDD"

void diep(const char *str) {
    perror(str);
    exit(EXIT_FAILURE);
}

void dies(const char *str) {
    fprintf(stderr, "[-] %s\n", str);
    exit(EXIT_FAILURE);
}

// returns time in microseconds
uint64_t systime() {
    struct timeval tv;

    if(gettimeofday(&tv, NULL) < 0)
        diep("gettimeofday");

    return (tv.tv_sec * 1000000) + tv.tv_usec;
}

uint64_t seektest(int fd, off_t offa, off_t offb) {
    uint64_t tinit, tend;
    void *buffer;

    // purge full disk from cache
    // this don't do anything else than
    // requesting the kernel to flush all the
    // pages which contains cache about this disk
    // to ensure we hit the disk (at least on a kernel
    // perspective)
    if(posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED))
        diep("posix_fadvise");

    // allocate aligned memory
    // it's not anymore really needed to make
    // aligned memory but it's still better for performance
    //
    // this was required when doing the test using
    // O_DIRECT flag, and implementation still use it
    // in case of change later
    if(posix_memalign(&buffer, 512, BUFFERSIZE))
        dies("posix_memalign");

    // first seek
    lseek(fd, offa, SEEK_SET);
    if(read(fd, buffer, BUFFERSIZE) < 0)
        diep("read init");

    // starting timing
    // we compute the time to do the
    // second seek only
    tinit = systime();

    // second seek
    lseek(fd, offb, SEEK_SET);
    if(read(fd, buffer, BUFFERSIZE) < 0)
        diep("read end");

    // computing time elapsed
    tend = systime();
    free(buffer);

    return tend - tinit;
}

// this is the real implementation of the check
// it returns an averae seektime in microseconds (us)
// this is an approximative value but enough to determine
// if the disk is an SSD or HDD
uint64_t seektime(int fd, size_t disklen) {
    // counters
    size_t checked = 0;
    uint64_t fulltime = 0;

    // loop on the seek test
    for(checked = 0; checked < DISKLOOP; checked += 1) {
        // generate random offset within the disk
        off_t offa = rand() % disklen;
        off_t offb = rand() % disklen;

        // probing seektime between theses two offset
        uint64_t thistime = seektest(fd, offa, offb);
        // printf("[+] seektime: %lu us\n", thistime);

        fulltime += thistime;
        checked += 1;
    }

    // returns the average
    return (fulltime / checked);
}

void output(int json, char *type, char *device, uint64_t elapsed) {
    if (json == 1)
        printf("{\"device\": \"%s\", \"type\": \"%s\", \"elapsed\": %lu}\n", device, type, elapsed);
    else
        printf("%s: %s (%lu us)\n", device, type, elapsed);
}

int main(int argc, char *argv[]) {
    int fd;
    int json, opt;
    char *device;

    json = 0;

    while ((opt = getopt(argc, argv, "j")) != -1) {
        switch (opt) {
        case 'j':
            json = 1;
            break;
        default: /* '?' */
            fprintf(stderr, USAGE,
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, USAGE, argv[0]);
        exit(EXIT_FAILURE);
    }

    device = argv[optind];

    // open disk in read-only
    //
    // we could open it in O_DIRECT flag
    // but this is not as efficient then
    // opening it with default settings
    // and clearing the cache
    if((fd = open(device, O_SYNC | O_RDONLY)) < 0)
        diep(argv[1]);

    // randomize
    srand(time(NULL));

    // seeking disk length
    off_t disklen = lseek(fd, 0, SEEK_END);

    // starting seektime test
    uint64_t elapsed = seektime(fd, disklen);

    // analyzing seektime
    char *type = NULL;

    if(elapsed >= 500)
        type = TYPE_HDD;
    else
        type = TYPE_SDD;

    output(json, type, device, elapsed);
    close(fd);

    return 0;
}
