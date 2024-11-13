// Condiciones de carrera
// Diseña un programa donde 4 hilos sumen 10 a una variable.
// Sin mecanismos de sincronización, muestra ejemplos de condiciones de carrera y explica cómo influye el orden de ejecución en el resultado final.
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define NTHREADS 4

int contador = 0;

void* incrementar() {
    for (int i = 0; i < 10; i++) {
        sleep(1);  //pausa para hacer más visibles los efectos de la race condition
        contador++;
    }
    return NULL;
}

int main() {
    pthread_t threads [NTHREADS];
    for (int i = 0; i < NTHREADS; i++) {
        pthread_create(&threads[i], NULL, incrementar, NULL);
    }
    for (int i = 0; i < NTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    printf("El contador final es %d\n", contador);
    return 0;
}
