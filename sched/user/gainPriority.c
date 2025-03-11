#include <inc/lib.h>
// este da error porque intenta mejorar prioridad en el MLFQ
void
umain(int argc, char **argv)
{
    int priority;
    envid_t envid = sys_getenvid();

    priority = sys_get_priority(envid);
    cprintf("Current priority: %d\n", priority);
    //Esta bien que esto de error, ya que no se puede aumentar la prioridad
    if (sys_set_priority(envid, priority - 1) == 0) {
        cprintf("Priority successfully set to: %d\n", priority - 1);
    } else {
        cprintf("Failed to set priority to: %d\n", priority - 1);
    }

    priority = sys_get_priority(envid);
    cprintf("New priority: %d\n", priority);

    for (int i = 0; i < 5; i++) {
        cprintf("Process with priority %d running\n", priority);
        sys_yield();
    }
}