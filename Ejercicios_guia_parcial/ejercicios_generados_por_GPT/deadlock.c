// Deadlock en un programa de sincronización
// Describe un caso de deadlock en un sistema donde dos hilos intentan acceder a dos recursos compartidos en un orden inverso.
// ¿Cómo se podría evitar este deadlock usando semáforos o mutexes?

#include <pthread.h>
#include <stdio.h>


pthread_mutex_t mutex1;
pthread_mutex_t mutex2;


void* function1(void* arg) {
    pthread_mutex_lock(&mutex1);
    printf("El thread1 tiene el mutex1\n");
    pthread_mutex_lock(&mutex2);
    printf("El thread1 tiene el mutex2\n");
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return NULL;
} 
void* function2(void* arg) {
    pthread_mutex_lock(&mutex2);
    printf("El thread2 tiene el mutex2\n");
    pthread_mutex_lock(&mutex1);
    printf("El thread2 tiene el mutex1\n");
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    return NULL;
}

int main() {
    pthread_t thread1, thread2;

    pthread_mutex_init(&mutex1, NULL);
    pthread_mutex_init(&mutex2, NULL);

    pthread_create(&thread1, NULL, function1, NULL);
    pthread_create(&thread2, NULL, function2, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return 0;
}



//ESTO CASI SIEMPRE PRODUCE UN DEADLOCK PERO A VECES NO, ES NORMAL. LOS THREADS SON INDETERMINÍSTICOS. 
//PARA EVITARLO BASTA CON SWAPEAR LAS LINEAS 23 Y 25 O 14 Y 16.







