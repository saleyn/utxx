/*
 * robust.c
 *
 * Demonstrates robust mutexes.
 *
 */

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

template <int N>
struct basic_buffer {
    pthread_mutex_t mutex1;
    pthread_mutex_t mutex2;
    char data[N];
};

static const int N = 128;
typedef basic_buffer<N> buffer_t;

void failing_thread(buffer_t* b)
{
    const char* name = "Owner   ";
    /* Lock mutex1 to make thread1 wait */
    pthread_mutex_lock(&b->mutex1);
    pthread_mutex_lock(&b->mutex2);

    fprintf(stderr, "%s %d: mutex1 acquired\n", name, getpid());
    fprintf(stderr, "%s %d: mutex2 acquired\n", name, getpid());

    sleep(1);
    fprintf(stderr, "%s %d: Allow threads to run\n", name, getpid());

    pthread_mutex_unlock(&b->mutex1);

    fprintf(stderr, "%s %d: mutex1 released -> exiting\n", name, getpid());
}

void waiting_thread(buffer_t* b)
{
    const char* name = "Consumer";
    int ret;

    fprintf(stderr, "%s %d: wait on mutex2\n", name, getpid());

    ret = pthread_mutex_lock(&b->mutex2);
    if (ret == EOWNERDEAD) {
        fprintf(stderr, "%s %d: mutex2 owner dead\n", name, getpid());
        /* Owner died; attempt to set mutex back to consistent state */
        if (pthread_mutex_consistent_np(&b->mutex2) != 0) {
            fprintf(stderr, "%s %d Cannot recover mutex2: %s\n", name, getpid(), strerror(errno));
            exit (EXIT_FAILURE);
        }
        fprintf(stderr, "%s %d: mutex2 made consistent\n", name, getpid());
    } else if (ret != 0) {
        fprintf(stderr, "%s %d: Error waiting on mutex2", name, getpid());
        exit(EXIT_FAILURE);
    } else {
        fprintf(stderr, "%s %d: mutex2 acquired\n", name, getpid());
    }

    pthread_mutex_unlock(&b->mutex2);
    fprintf(stderr, "%s %d: unlocked mutex2 and exiting\n", name, getpid());
}

int main()
{
    buffer_t* buffer;

    int zfd = open("/dev/zero", O_RDWR);
    buffer = (buffer_t *)mmap(NULL, sizeof(buffer_t),
        PROT_READ|PROT_WRITE, MAP_SHARED, zfd, 0);
    ftruncate(zfd, sizeof(buffer_t));
    close(zfd);

    bzero(buffer->data, sizeof(buffer_t::data));

    pthread_mutexattr_t mutex_attr;

    /* Initialize mutex2 with robust support */
    pthread_mutexattr_init(&mutex_attr);

    if (pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED) < 0)
        perror("Error setting shared attribute");

    if (pthread_mutexattr_setrobust_np(&mutex_attr, PTHREAD_MUTEX_ROBUST_NP) < 0)
        perror("Error setting robust mutex attribute");
    if (pthread_mutexattr_setprotocol(&mutex_attr, PTHREAD_PRIO_INHERIT) < 0)
        perror("Error setting shared attribute");

    if (pthread_mutex_init(&buffer->mutex1, &mutex_attr) < 0)
        perror("Error initializing mutex1");
    if (pthread_mutex_init(&buffer->mutex2, &mutex_attr) < 0)
        perror("Error initializing mutex2");

    pid_t children[3];

    if ((children[0] = fork()) == 0) {
        failing_thread(buffer);
        exit(0);
    }

    sleep(1);

    for (int i=1; i < sizeof(children)/sizeof(children[0]); i++)
        if ((children[i] = fork()) == 0) {
            waiting_thread(buffer);
            exit(0);
        }

    int status;
    for (int i=0; i < sizeof(children)/sizeof(children[0]); i++)
        waitpid(children[i], &status, 0);

    munmap(buffer, sizeof(buffer_t));
    fprintf(stderr, "Main process exited\n");

    return EXIT_SUCCESS;
}
