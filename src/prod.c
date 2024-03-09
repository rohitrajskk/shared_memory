/*
 * SHM Producer 
 */

#include "shm.h"


char        read_buf[UV_READ_BUF_SIZE]  = {0};
shm_mem_t   *shm_base                   = NULL;
int         shm_fd                      = -1;
size_t      size                        = sizeof(shm_mem_t);
uint64_t    prod_seq                    = 0;


static void
prod_cleanup()
{
    close(shm_base->event_fd);
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
prod_write_header(shm_mem_t *buf)
{
    int     event_fd;
    pid_t   pid;

    pid = getpid();
    event_fd = eventfd(0, EFD_CLOEXEC);
    printf("Started eventfd for notification with fd %d\n", event_fd);
    if (event_fd < 0 || pid < 0) {
        exit(1);
    }
    buf->prod_pid = pid;
    buf->event_fd = event_fd;
    atomic_store(&buf->prod_seq, 0);
    atomic_store(&buf->cons_seq, 0);
    printf("Write SHM Header: PID: %d eventfd: %d\n", pid, event_fd);
}

static void
prod_write_data(shm_mem_t *buf, uint64_t data)
{
    uint64_t    cons_seq;

    cons_seq = atomic_load(&buf->cons_seq);
    if (cons_seq > prod_seq) {
        //error;
    }
    if((prod_seq - cons_seq)  >= BUF_COUNT) {
        printf("Slow consumer, buffer full\n");
        return;
    }
    buf->data[prod_seq%BUF_COUNT] = data;
    prod_seq++;
    atomic_store(&buf->prod_seq, prod_seq);
    printf("Write Data: %lu\n", data);
    eventfd_write(buf->event_fd, prod_seq);
    
}

static void
uv_buf_alloc(uv_handle_t *handle, size_t size, uv_buf_t *buf)
{
    buf->base = read_buf;
    buf->len = UV_READ_BUF_SIZE;
}

void
on_stdin_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    if (nread >= 0) {
        if (buf->base[0] == 'q') {
            prod_cleanup();
            exit(1);
        }
        int num = atoi(buf->base);
        if (num != 0 || buf->base[0] == '0') {
            prod_write_data(shm_base, num);
            //printf("You entered: %d\n", num);
        } else {
            printf("Invalid input. Please enter an integer.\n");
        }
    } else {
        printf("Exiting program.\n");
        uv_stop(uv_default_loop());
    }
}


int main(void)
{
    uv_loop_t       *loop       = uv_default_loop();
    uv_tty_t        stdin_tty;


    /* create the shared memory segment as if it was a file */
    shm_fd = shm_open(SHARED_MEM_PATH, O_RDWR | O_CREAT, 0644);
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
    printf("Started SHM producer: %s\n", SHARED_MEM_PATH);
    prod_write_header(shm_base);
    uv_tty_init(loop, &stdin_tty, 0, 1); // 0 for stdin, 1 for readable

    uv_read_start((uv_stream_t*)&stdin_tty,
                  (uv_alloc_cb)uv_buf_alloc,
                  on_stdin_read);

    printf("Please enter integer data to be written in shared memory.\n");
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
