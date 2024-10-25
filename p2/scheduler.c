/**
 * Tony Givargis
 * Copyright (C), 2024
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * scheduler.c
 */

#undef _FORTIFY_SOURCE

#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include "system.h"
#include "scheduler.h"

/**
 * Needs:
 *   setjmp()
 *   longjmp()
 */

/* research the above Needed API and design accordingly */

#define SZ_STACK 1048576

struct thread {
    jmp_buf env;
    enum {
        STATUS_,
        STATUS_RUNNING,
        STATUS_SLEEPING,
        STATUS_TERMINATED
    } status;
    struct {
        void *memory_; /* where it's located at */
        void *memory; /* the one we use */
    } stack;
    scheduler_fnc_t fnc;
    void *arg;
    struct thread *next;
};

static struct {
    struct thread *head;
    struct thread *last;
    struct thread *current;
    jmp_buf env;
} scheduler;

struct thread *thread_candidate(void) {
    struct thread *curr = scheduler.current;
    do {
        if (curr->status != STATUS_TERMINATED) {
            return curr;
        }
        curr = curr->next;
    } while (curr != scheduler.current);
    return NULL;
}

int scheduler_create(scheduler_fnc_t fnc, void *arg) {

    size_t PAGE_SIZE = page_size();
    struct thread *head;
    struct thread *curr;
    void *stack_mem;

    struct thread *thread = (struct thread *) malloc(sizeof(struct thread));
    if (thread == NULL) {
        perror("Error allocating memory for thread");
        return -1;
    }

    stack_mem= malloc(SZ_STACK + PAGE_SIZE);
    if (stack_mem == NULL) {
        perror("Error allocating stack memory");
        free(thread);
        return -1;
    }

    thread->stack.memory_ = stack_mem;


    
    thread->stack.memory = memory_align(thread->stack.memory_, PAGE_SIZE);
    thread->status = STATUS_;
    thread->fnc = fnc;
    thread->arg = arg;
    thread->next = NULL;

    if (scheduler.head == NULL) {
        scheduler.head = scheduler.last = scheduler.current = thread;
        thread->next = thread;
    } else {
        scheduler.last->next = thread;
        thread->next = scheduler.head;
        scheduler.last = thread;
    }

    head = scheduler.head;
    printf("\nLinked list:\n");
    curr = head;
    do {
        printf("%s -> ", (const char *)curr->arg);
        curr = curr->next;
    } while (curr != head);
    printf("(back to head)\n");

    return 0;
}

void schedule(void) {
    struct thread *candidate = thread_candidate();
    if (candidate == NULL) {
        return;
    }

    if (candidate->status == STATUS_) {
        uint64_t esp = (uint64_t) candidate->stack.memory + SZ_STACK; // Set the stack pointer to the top of the candidate's stack

__asm__ volatile (
    "mov %0, %%esp" // Move the esp value to the esp register
    : // No output
    : "r"(esp) // Input: esp value
    : "%esp" // Clobber the esp register
);


        candidate->status = STATUS_RUNNING;
        printf("\n -----------Initializing thread -> %s-----------\n", (const char *) candidate->arg);
        candidate->fnc(candidate->arg);

        candidate->status = STATUS_TERMINATED;
        longjmp(scheduler.env, 1);
    } else {
        printf("\n-----------Resuming thread -> %s-----------\n", (const char *) candidate->arg);
        candidate->status = STATUS_RUNNING;
        longjmp(candidate->env, 1);
    }
}



void destroy(void) {
    struct thread *curr = scheduler.current->next;
    while (curr != scheduler.current) {
        struct thread *next = curr->next;
        free(curr->stack.memory_);
        free(curr);
        curr = next;
    }

    free(scheduler.current->stack.memory_);
    free(scheduler.current);
    scheduler.current = NULL;
    scheduler.head = NULL;
}

/* For automatic yielding */

void start_timer(void) {
    alarm(1);
}

void stop_timer(void) {
    alarm(0);
    if (SIG_ERR == signal(SIGALRM, SIG_DFL)) {
        perror("Error resetting signal");
    }
}

void scheduler_execute(void) {
    if (SIG_ERR == signal(SIGALRM, scheduler_yield)) {
        perror("Error setting signal");
    }
    setjmp(scheduler.env);
    start_timer();
    schedule();
    stop_timer();
    destroy();
}

void scheduler_yield(int sig) {
    assert(SIGALRM == sig); 
    /* Save the running thread's context */
    if (setjmp(scheduler.current->env) == 0) {
        scheduler.current->status = STATUS_SLEEPING;
        scheduler.current = scheduler.current->next;
        longjmp(scheduler.env, 1);
    }
}
