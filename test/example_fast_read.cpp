// g++ -std=c++11 -O3 -o example_fast_read example_fast_read.cpp
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static const long BUFFER_SIZE = (1 * 1024 * 1024);
static const long ITERATIONS  = (10 * 1024);

double now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.;
}

long f1(unsigned char* buffer) {
    long y = 0;
    FILE *fp;
    fp = fopen("/dev/zero", "rb");
    setvbuf(fp, nullptr, _IOFBF, BUFFER_SIZE);

    for(long i = 0; i < ITERATIONS; ++i)
    {
        fread(buffer, BUFFER_SIZE, 1, fp);
        for(long x = 0; x < BUFFER_SIZE; x += 1024)
        {
            y += buffer[x];
        }
    }
    fclose(fp);
    return y;
}

long f2(unsigned char* buffer) {
    long y = 0;
    unsigned char *mmdata;
    int fd = open("/dev/zero", O_RDONLY);
    for(long i = 0; i < ITERATIONS; ++i)
    {
        mmdata = (unsigned char*)mmap(NULL, BUFFER_SIZE, PROT_READ, MAP_PRIVATE|MAP_NORESERVE|MAP_POPULATE, fd, i * BUFFER_SIZE);
        if (!mmdata) {
            perror("mmap");
            exit(1);
        } 
        // But if we don't touch it, it won't be read...
        // I happen to know I have 4 KiB pages, YMMV
        for(long x = 0; x < BUFFER_SIZE; x += 1024)
        {
            y += mmdata[x];
        }
        munmap(mmdata, BUFFER_SIZE);
    }
    close(fd);
    return y;
}

long f3(unsigned char* buffer) {
    long y = 0;
    int fd;
    fd = open("/dev/zero", O_RDONLY);
    for(long i = 0; i < ITERATIONS; ++i)
    {
        //readahead(fd, 0, BUFFER_SIZE);
        read(fd, buffer, BUFFER_SIZE);
        for(long x = 0; x < BUFFER_SIZE; x += 1024)
        {
            y += buffer[x];
        }
    }
    close(fd);
    return y;
}

template <typename Fun>
void call(Fun f, const char* title) {
    double start_time = now();

    long   sum = f();

    double end_time = now();

    double total_time = end_time - start_time;

    printf("%10s: %.3f seconds to read %.1f GB (speed=%.3f GB/s, sum=%ld)\n",
           title, total_time, double(BUFFER_SIZE*ITERATIONS / 1024 / 1024 / 1024),
           double(BUFFER_SIZE*ITERATIONS / (1024*1024*1024) / total_time), sum);
}

int main()
{
    unsigned char buf[BUFFER_SIZE]; // 1 MiB buffer
    unsigned char* p = buf;

    call([p](){ return f1(p); }, "fread");
    call([p](){ return f2(p); }, "mmap");
    call([p](){ return f3(p); }, "read");

    return 0;
}
