#include "sched.h"
#include "switch.h"
#include <stdlib.h>
#include <limits.h>

extern struct proc proc[];
extern int nproc;

int last_index = -1;

struct proc* select_next() {
    for (int i = 1; i <= nproc; i++) {
        int index = (last_index + i) % nproc; //avanzar cíclicamente
        if (proc[index].status == READY) {
            last_index = index;
            return &proc[index];
        }
    }
    return NULL;  // Si no hay procesos listos
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
