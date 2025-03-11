#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <inc/env.h>
void sched_halt(void);
static int total_exec_time = 0;
static int sched_runs = 0;
#define MAX_PRIORITY_LEVELS 5
#define BOOST_INTERVAL 1000
#define MAX_EXECUTIONS_PER_SLICE 3

struct SchedStats {
    int num_calls;
    int num_executions[NENV];
    int history[NENV];
    int history_idx;
};

static struct SchedStats sched_stats;
//una implementacion basica de una cola(usando los indices head y tail para saber los elementos válidos) 
struct PriorityQueue {
    struct Env *envs[NENV];
    int head;
    int tail;
    int tamanio;
};

static struct PriorityQueue priority_queues[MAX_PRIORITY_LEVELS];
//imprime las stats de la ejecucion
void 
print_sched_stats(void) {
    cprintf("Scheduler Statistics:\n");
    cprintf("Number of calls to sched_yield: %d\n", sched_stats.num_calls);
    for (int i = 0; i < NENV; i++) {
        cprintf("Environment %d executed %d times\n", i, sched_stats.num_executions[i]);
    }
    cprintf("Execution history:\n");
    for (int i = 0; i < sched_stats.history_idx; i++) {
        cprintf("%d ", sched_stats.history[i]);
    }
    cprintf("\n");
}
//inicializar las colas de prioridad(sin procesos ni nd)
void init_priority_queues(void) {
    for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        priority_queues[i].head = 0;
        priority_queues[i].tail = 0;
        priority_queues[i].tamanio = 0;
    }
}
//agregar un proceso a la cola de prioridad
void enqueue(struct PriorityQueue *queue, struct Env *env) {
    if (queue->tamanio < NENV) {
        queue->envs[queue->tail] = env;
        queue->tail = (queue->tail + 1) % NENV;
        queue->tamanio++;
    }
}
//sacar un proceso de la cola de prioridad
struct Env* dequeue(struct PriorityQueue *queue) {
    if (queue->tamanio > 0) {
        struct Env *env = queue->envs[queue->head];
        queue->head = (queue->head + 1) % NENV;
        queue->tamanio--;
        return env;
    }
    return NULL;
}
//le suma uno a la prioridad del proceso(degrada su priority)
void demote_process(struct Env *env) {
    if (env->env_priority < MAX_PRIORITY_LEVELS - 1) {
        env->env_priority++;
        enqueue(&priority_queues[env->env_priority], env);
    }
}
//boostea la prioridad de todos los procesos
void boost_processes(void) {
    for (int i = 0; i < NENV; i++) {
        if (envs[i].env_status == ENV_RUNNABLE) {
            envs[i].env_priority = 0;
            enqueue(&priority_queues[0], &envs[i]);
        }
    }
}

//Paso esto a una func para usarla en priorities
void round_robin() {
	int start_idx = (curenv == NULL) ? 0 : (ENVX(curenv->env_id) + 1) % NENV;
	int checked_envs = 0;

	while (checked_envs < NENV) {
		if (envs[start_idx].env_status == ENV_RUNNABLE) {
			sched_stats.num_executions[start_idx]++;
            sched_stats.history[sched_stats.history_idx++] = start_idx;
			env_run(&envs[start_idx]);
			return;
		}
		start_idx = (start_idx + 1) % NENV;
		checked_envs++;
	}

	if (curenv && curenv->env_status == ENV_RUNNING) {
		sched_stats.num_executions[ENVX(curenv->env_id)]++;
        sched_stats.history[sched_stats.history_idx++] = ENVX(curenv->env_id);
		env_run(curenv);
		return;
	}
}


// Choose a user environment to run and run it.
void
sched_yield(void)
{
	sched_stats.num_calls++;
	sched_runs++;

    if (sched_runs >= BOOST_INTERVAL) { //no es técnicamente el tiempo igualmente(es cuantas veces se llama a sched_yield)
        boost_processes();//cada 1000 llamadas se boostean los procesos a la mejor prioridad
        total_exec_time = 0;
    }
#ifdef SCHED_ROUND_ROBIN
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running. Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	round_robin();
#endif

#ifdef SCHED_PRIORITIES
    static bool initialized = false;
    if (!initialized) {
        init_priority_queues(); //inicializar las prioridades
        initialized = true;
    }

    // mover los procesos RUNNABLE a las colas de prioridades
    for (int i = 0; i < NENV; i++) {
        if (envs[i].env_status == ENV_RUNNABLE) {
            enqueue(&priority_queues[envs[i].env_priority], &envs[i]); //agregar el proceso a la cola de prioridad
        }
    }

    // buscar el proceso RUNNABLE de mayor prioridad para ejecurarlo
    for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        if (priority_queues[i].tamanio > 0) {
            if (priority_queues[i].tamanio > 1) {
                round_robin(); //si hay mas de un proceso en la cola de prioridad, se hace round robin
                return;
            }
            struct Env *env = dequeue(&priority_queues[i]); //sacar el proceso de la cola de prioridad
            sched_stats.num_executions[ENVX(env->env_id)]++;
            sched_stats.history[sched_stats.history_idx++] = ENVX(env->env_id);

			if (sched_stats.num_executions[ENVX(env->env_id)] % MAX_EXECUTIONS_PER_SLICE == 0) {
                demote_process(env); //Si se alcanzó el máximo de ejecuciones por slice, se degrada la prioridad del env
            }

            env_run(env); //ejecutar el proceso
            return;
        }
    }
#endif

	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");

		//print stats
		print_sched_stats();

		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics on
	// performance. Your code here

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}
