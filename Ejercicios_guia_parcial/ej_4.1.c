// 4.1 Concurrencia / Threads.
// Utilice la API de los Threads para crear un programa que use 5 threads para incrementar
// una variable compartida por todos en 7 unidades/thread (cada thread suma 7).
// ★ La suma debe cortar al llegar o superar las 1000 unidades.
// ◆ Dar ejemplos de: race-condition; dead-lock.

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#define INCREMENTO 7
#define MAX 1000
#define NTHREADS 5

int variable = 0;
pthread_mutex_t mutex;

void* incrementar(void* arg) {
    while(true) {
        if (variable >= 1000) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_lock(&mutex);
        variable += INCREMENTO;
        pthread_mutex_unlock(&mutex);
    }
}

int main(){
    pthread_t threads[NTHREADS];
    pthread_mutex_init(&mutex, NULL);
    for (int i = 0; i < NTHREADS; i++) {
        pthread_create(&threads[i], NULL, incrementar, (void*)i);
    }
    for (int i = 0; i < NTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
    printf("Valor del contador: %i\n", variable);
    return 0;
}

// NOS DEVUELVE 1001 QUE ES MULTIPLO DE 7 => ESTO FUNCA BIEN!!!







