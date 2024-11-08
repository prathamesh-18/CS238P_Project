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
#define INITIAL_SIGNATURE 0xDEADBEEF

/* 
// Define the META structure for storing SCM metadata */
typedef struct {
    int signature;       /* / Unique identifier for verification */
    void *begin_addr;    /* // Initial address of the SCM memory region */
    size_t size;         /* // Tracks the current allocated memory size */
} META;


void init_meta(META *meta, void *scm_mem) {
    if (meta->signature != (int) INITIAL_SIGNATURE) {
        meta->signature = INITIAL_SIGNATURE;
        meta->begin_addr = scm_mem;
        meta->size = 0;
    }
}

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
    META *meta;
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

    meta= (META *) scm->mem;
    init_meta(meta, scm->mem);
    

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




void *scm_malloc(struct scm *scm, size_t n) {
    void *p;
    META *meta;

    if (n == 0) {
        perror("Requested memory size is zero");
        return NULL;
    }
    meta= (META *) scm->mem;

    /* // Check if the signature is valid */
    if (meta->signature != (int)INITIAL_SIGNATURE) {
        perror("Invalid or corrupted META block");
        return NULL;  /* // Aborting allocation if signature is invalid */
    }

    p = (char *) scm->mem + sizeof(META) + meta->size;
    if ((meta->size + n) > scm->length - sizeof(META)) {
        perror("No available space for allocation");
        return NULL;
    }

    meta->size += n;  /* // Update META size to reflect allocation */
    return p;
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
    UNUSED(scm);
    UNUSED(p);

}

/* // Return the number of bytes used in the SCM region, based on the META block */
size_t scm_size(const struct scm *scm) {
    META *meta = (META *) scm->mem;
    return meta->size;
}

/* // Return the total length of the SCM region, based on the META block */
size_t scm_length(const struct scm *scm) {

    return scm->length;
}
/* 
// Return the base memory address of the SCM region, based on the META block */
void *scm_memory_base(struct scm *scm) {

    return (char *) scm->mem + sizeof(META);  /* // Skip over the META block */
}
