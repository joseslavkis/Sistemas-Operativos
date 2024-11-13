#include "sched.h"
#include "switch.h"
#include <stdlib.h>
#include <limits.h>


extern struct proc proc[];
extern int nproc;

struct proc* select_next() {
    struct proc* menor_vruntime = NULL;
    for (int i = 0; i < nproc; i++) {
        if (proc[i].status == READY && (menor_vruntime == NULL || proc[i].vruntime < menor_vruntime->vruntime)) {
            menor_vruntime = &proc[i]
        }
    }
    return NULL;
}

void scheduler() {
    while (!done()) {
        struct proc *candidate = select_next();

        if (candidate != NULL) {
            candidate->status = RUNNING;
            int vruntime = calculate_time_slice(candidate);
            candidate->vruntime += vruntime;
            candidate->runtime += swtch(candidate);
            candidate->status = READY;  //volver a poner en READY después de ejecutar
        } else {
            idle();
        }
    }
}

//la mejor prioridad sería 0
int calculate_time_slice(struct proc* p) {
    int base_time_slice = 10;
    return base_time_slice / (p->priority + 1) //una forma de hacer que los procesos con mejor prioridad tengan mas time_slice real
}

