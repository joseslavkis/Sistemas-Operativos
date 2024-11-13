#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define NTHREADS 4

int contador = 0;
pthread_mutex_t mutex;

void* incrementar() {
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&mutex);
        sleep(1);  //pausa para hacer más visibles los efectos de la race condition(aqui ya no hay igualmente)
        contador++;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    pthread_t threads [NTHREADS];
    pthread_mutex_init(&mutex, NULL);
    for (int i = 0; i < NTHREADS; i++) {
        pthread_create(&threads[i], NULL, incrementar, NULL);
    }
    for (int i = 0; i < NTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
    printf("El contador final es %d\n", contador);
    return 0;
}
