#include "sched.h"
#include "switch.h"
#include <stdlib.h>
#include <limits.h>


extern struct proc proc[];
extern int nproc;

struct proc* select_next() {
    struct proc* first_arrived = NULL;
    for (int i = 0; i < nproc; i++) {
        if (proc[i].status == READY && (first_arrived == NULL || proc[i].arrival_time < first_arrived->arrival_time)) {
            first_arrived = &proc[i];
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
