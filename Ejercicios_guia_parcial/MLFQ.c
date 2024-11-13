#include "sched.h"
#include "switch.h"
#include <stdlib.h>
#include <limits.h>

#define HIGHEST_PRIORITY 5
#define LOWEST_PRIORITY 0

extern struct proc proc[];
extern int nproc;

struct proc* select_next() {
    for (int level = 0; level < HIGHEST_PRIORITY; level++) {
        for (int i = 0; i < nproc; i++) {
            if (proc[i].status == READY && proc[i].priority == level) {
                return &proc[i];
            }
        }
    }
    return NULL;
}

void scheduler() {
    while (!done()) {
        struct proc *candidate = select_next();

        if (candidate != NULL) {
            candidate->status = RUNNING;
            candidate->runtime += swtch(candidate);
            candidate->status = READY;  //volver a poner en READY después de ejecutar
        } else {
            idle();
        }
    }
}
