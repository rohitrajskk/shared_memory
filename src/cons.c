/**
 * Producer 
 *
 *
 */

#include "shm.h" 


char        read_buf[UV_READ_BUF_SIZE]  = {0};
shm_mem_t   *shm_base                   = NULL;
int         shm_fd                      = -1;
size_t      size                        = sizeof(shm_mem_t);
uint64_t    cons_seq                    = 0;


static void
cons_cleanup()
{
    /* remove the mapped memory segment from the address space of the process */
    if (munmap(shm_base, size) == -1) {
        printf("prod: Unmap failed: %s\n", strerror(errno));
        exit(1);
    }

    /* close the shared memory segment as if it was a file */
    if (close(shm_fd) == -1) {
        printf("prod: Close failed: %s\n", strerror(errno));
        exit(1);
    }

}


static void
cons_read_data(shm_mem_t *buf)
{
    uint64_t    prod_seq;
    uint64_t    data;

    prod_seq = atomic_load(&buf->prod_seq);
    if (cons_seq >= prod_seq) {
        printf("No data to read");
        return;
    }
    data = buf->data[cons_seq%BUF_COUNT];
    cons_seq++;
    atomic_store(&buf->cons_seq, cons_seq);
    printf("Read Data : %lu\n", data);
    if(cons_seq < prod_seq) {
        return cons_read_data(buf);
    }
}


void eventfd_cb(uv_poll_t* handle, int status, int events)
{
    if (status < 0) {
        // Handle error
        return;
    }

    if (events & UV_READABLE) {
        // Read the value of the eventfd
        uint64_t value;
        ssize_t n = read(handle->io_watcher.fd, &value, sizeof(value));
        if (n < 0) {
            return;
        }
        if (n == 0) {
            exit(1);
            return;
        }
        cons_read_data(shm_base);
    }
}

bool
cons_start_eventfd_notification(shm_mem_t *shm_base)
{
    int     pidfd;
    int     event_fd;

    if(shm_base->prod_pid == 0 || shm_base->event_fd ==0) {
        printf("Publisher has not written  shm header\n");
        return false;
    }
    pidfd = syscall(SYS_pidfd_open, shm_base->prod_pid, 0);
    event_fd = syscall(SYS_pidfd_getfd, pidfd, shm_base->event_fd, 0);

    uv_loop_t* loop = uv_default_loop();

    // Create the uv_poll_t object
    uv_poll_t eventfd_poll;
    uv_poll_init(loop, &eventfd_poll, event_fd);

    // Start polling for events
    uv_poll_start(&eventfd_poll, UV_READABLE, eventfd_cb);
    printf("Started eventfd notification callback on prod fd: %d copy fd:%d\n",
           shm_base->event_fd, event_fd);
    uv_run(loop, UV_RUN_DEFAULT);
    return true;
}

int main(void)
{

    /* create the shared memory segment as if it was a file */
    shm_fd = shm_open(SHARED_MEM_PATH, O_RDWR, 0666);
    if (shm_fd == -1) {
        printf("prod: Shared memory failed: %s\n", strerror(errno));
        exit(1);
    }

    /* configure the size of the shared memory segment */
    ftruncate(shm_fd, size);

    /* map the shared memory segment to the address space of the process */
    shm_base = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_base == MAP_FAILED) {
        close(shm_fd);
        printf("prod: Map failed: %s\n", strerror(errno));
        // close and shm_unlink?
        exit(1);
    }
    printf("Connected to shared memory: %s\n", SHARED_MEM_PATH);

    cons_start_eventfd_notification(shm_base);
    return 0;
}
