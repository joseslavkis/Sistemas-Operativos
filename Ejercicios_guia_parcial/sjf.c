#include "sched.h"
#include "switch.h"
#include <stdlib.h>
#include <limits.h>


//ACLARO QUE TODO ESTO ES PSEUDOCÓDIGO, SEGURAMENTE NO COMPILE

extern struct proc proc[];
extern int nproc;
struct proc* select_next(){
    struct proc* shortest = NULL;
    for (int i = 0; i < nproc; i++) {
        if (proc[i].status == READY && (shortest == NULL || proc[i].time_needed < shortest->time_needed)) {
            shortest = &proc[i];
        }
    }
    return &shortest
}

void scheduler(){
    while (!done()){
        struct proc *candidate = select_next();

        if (candidate != NULL) {
            candidate->status = RUNNING;
            candidate->runtime += swtch(candidate);
            candidate->status = READY;
        } else {
            idle();
        }
    }
}