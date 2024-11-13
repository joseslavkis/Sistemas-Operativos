// Escribe un programas que use tres hilos para sumar en paralelo tres partes de un arreglo grande de enteros.

// Asegúrate de sincronizar correctamente el acceso a la suma total. Identifica posibles puntos de “race conditions” y cómo los resolverías.

#include <stdio.h>
#include <pthread.h>

#define NTHREADS 3
#define TAMANIO_ARREGLO 1000

int arreglo[TAMANIO_ARREGLO];
int suma_total = 0;
pthread_mutex_t mutex;

void* sumar_parte(void* arg) {
    int id = *(int*)arg;
    int inicio = id * (TAMANIO_ARREGLO / NTHREADS);
    int fin = (id + 1) * (TAMANIO_ARREGLO / NTHREADS);

    int suma_parcial = 0;
    for (int i = inicio; i < fin; i++) {
        suma_parcial += arreglo[i];
    }

    pthread_mutex_lock(&mutex);
    suma_total += suma_parcial;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main() {
    pthread_t threads[NTHREADS];
    int thread_ids[NTHREADS];

    for (int i = 0; i <= TAMANIO_ARREGLO; i++) {
        arreglo[i] = i + 1;
    }

    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < NTHREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, sumar_parte, &thread_ids[i]);
    }

    for (int i = 0; i < NTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);

    printf("Suma total: %d\n", suma_total);
    return 0;
}














