/**
 * Tony Givargis
 * Copyright (C), 2023
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * scm.c
 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include "scm.h"

/**
 * Required API calls:
 *   fstat()
 *   S_ISREG()
 *   open()
 *   close()
 *   sbrk()
 *   mmap()
 *   munmap()
 *   msync()
 */

#define VM_ADDR 0x600000000000



struct scm {
    int fd;
    size_t size;
    size_t length;
    void *mem;
};

struct scm *file_size(const char *pathname) {
    struct stat st;
    int fd;
    struct scm *scm = malloc(sizeof(struct scm));

    if (!scm) {
        perror("Failed to allocate memory for scm struct");
        return NULL;
    }
    memset(scm, 0, sizeof(struct scm));

    fd = open(pathname, O_RDWR);
    if (fd == -1) {
        perror("File open error");
        free(scm);
        return NULL;
    }

    if (fstat(fd, &st) == -1) {
        perror("Failed to get file stats");
        free(scm);
        close(fd);
        return NULL;
    }

    if (!S_ISREG(st.st_mode)) {
        perror("Not a regular file");
        free(scm);
        close(fd);
        return NULL;
    }

    scm->fd = fd;
    scm->length = st.st_size;
    printf("File capacity: %lu bytes\n", (unsigned long) scm->length);
    return scm;
}

struct scm *scm_open(const char *pathname, int truncate) {
    long vm_addr;
    struct scm *scm = file_size(pathname);
    if (!scm) return NULL;

     vm_addr= (VM_ADDR / 4096) * 4096;

    printf("Attempting mmap with capacity: %lu bytes\n", (unsigned long) scm->length);

    scm->mem = mmap((void *) vm_addr, scm->length, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, scm->fd, 0);
    if (scm->mem == MAP_FAILED) {
        perror("Memory mapping failed");
        close(scm->fd);
        free(scm);
        return NULL;
    }

    if (truncate) {
        if (ftruncate(scm->fd, (long) scm->length) == -1) {
            perror("File truncation failed");
            close(scm->fd);
            free(scm);
            return NULL;
        }
        scm->size = 0;
    } else {
        scm->size = *(size_t *) scm->mem;
    }

    return scm;
}

void scm_close(struct scm *scm) {
    if (!scm) return;

    if (msync(scm->mem, scm->length, MS_SYNC) == -1) perror("Memory synchronization failed");
    if (munmap(scm->mem, scm->length) == -1) perror("Memory unmapping failed");
    if (close(scm->fd) == -1) perror("File close failed");

    memset(scm, 0, sizeof(struct scm));
    free(scm);
}



size_t block_size(void *p) {
    return *(size_t *) ((char *) p + sizeof(short));
}

void *next_block_addr(void *p) {
    return (char *) (char *) p + sizeof(short) + sizeof(size_t) + block_size(p);
}



void set_block_size(void *p, size_t n) {
    *(size_t *) ((char *) p + sizeof(short)) = n;
}

void set_utilized(struct scm *scm, void *p, short increment) {
    size_t delta = increment * (block_size(p) + sizeof(short) + sizeof(size_t));
    scm->size += delta;
    *(size_t *) scm->mem = scm->size;
}

void *scm_malloc(struct scm *scm, size_t n) {
    void *p;
    void *end;
    if (n == 0) {
        perror("Requested memory size is zero");
        return NULL;
    }

    p=(char *) scm->mem + sizeof(size_t);
    end= (char *) scm->mem + scm->length;

    while (p < end) {
        if (*(short *)(p) == 0 && (char *) ((char *) p + sizeof(short) + sizeof(size_t)) + n <= (char *) end) {
             *(short *) p = 2;
            set_block_size(p, n);
            set_utilized(scm, p, 1);
            return (char *) p + sizeof(short) + sizeof(size_t);
        } else if (*(short *)(p) == 1 && block_size(p) >= n) {
            *(short *) p = 2;
            set_utilized(scm, p, 1);
            return (char *) p + sizeof(short) + sizeof(size_t);
        }
        p = next_block_addr(p);
    }

    perror("No available space for allocation");
    return NULL;
}

char *scm_strdup(struct scm *scm, const char *s) {
    size_t n;
    char *p ;
    if (!s) {
        perror("String is NULL");
        return NULL;
    }

    n=strlen(s) + 1;
    p= scm_malloc(scm, n);
    if (!p) {
        perror("Memory allocation for string duplication failed");
        return NULL;
    }
    memcpy(p, s, n);

    return p;
}

void scm_free(struct scm *scm, void *p) {
    void *addr;
    if (!p) return;

    addr = (char *) p - (sizeof(short) + sizeof(size_t));

    if (*(short *)(addr) == 2) {
        printf("Hi There");
        *(short *) addr= 1;
        set_utilized(scm, addr, -1);
        memset(p, 0, block_size(addr));
    } else {
        perror("Attempted double free or invalid free");
    }
}

size_t scm_size(const struct scm *scm) {
    return scm->size;
}

size_t scm_length(const struct scm *scm) {
    return scm->length;
}

void *scm_memory_base(struct scm *scm) {
    return (char *) ((char *) scm->mem + sizeof(size_t)) + sizeof(short) + sizeof(size_t);

}

