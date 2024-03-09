#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stdint.h>
#include <stdatomic.h>
#include <uv.h>

#define BUF_COUNT        128
#define UV_READ_BUF_SIZE 64 
#define SHARED_MEM_PATH "/shm_example"

typedef struct shm_mem_ {
    uint32_t                prod_pid;
    uint32_t                event_fd;
    atomic_uint_fast64_t    prod_seq;
    atomic_uint_fast64_t    cons_seq;
    uint64_t                data[BUF_COUNT];
} shm_mem_t;
