#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>

typedef struct {
    int pelota;
    pthread_mutex_t mutex;
    pthread_cond_t cond_padre;
    pthread_cond_t cond_hijo;
} Juego;

void* jugar_ping_pong(void* arg) {
    Juego* juego = (Juego*)arg;
    int jugador = 0;

    while (1) {
        pthread_mutex_lock(&juego->mutex);

        if (jugador == 0) {
            printf("Padre recibe la pelota: %d\n", juego->pelota);

            if (juego->pelota == INT_MAX) {
                printf("¡Overflow detectado! El juego termina.\n");
                pthread_mutex_unlock(&juego->mutex);
                break;
            } else if (juego->pelota < 0) {
                printf("¡Cambio de signo detectado! El juego termina.\n");
                pthread_mutex_unlock(&juego->mutex);
                break;
            }
            juego->pelota++;
            pthread_cond_signal(&juego->cond_hijo);
            pthread_cond_wait(&juego->cond_padre, &juego->mutex);
        }
        else {
            printf("Hijo recibe la pelota: %d\n", juego->pelota);

            if (juego->pelota == INT_MAX) {
                printf("¡Overflow detectado! El juego termina.\n");
                pthread_mutex_unlock(&juego->mutex);
                break;
            } else if (juego->pelota < 0) {
                printf("¡Cambio de signo detectado! El juego termina.\n");
                pthread_mutex_unlock(&juego->mutex);
                break;
            }

            juego->pelota++;

            pthread_cond_signal(&juego->cond_padre);

            pthread_cond_wait(&juego->cond_hijo, &juego->mutex);
        }

        jugador = 1 - jugador;

        pthread_mutex_unlock(&juego->mutex);
    }

    return NULL;
}

int main() {
    Juego juego = {0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER};
    
    pthread_t hilo_padre, hilo_hijo;

    if (pthread_create(&hilo_padre, NULL, jugar_ping_pong, &juego) != 0) {
        perror("Error al crear el hilo padre");
        exit(1);
    }

    if (pthread_create(&hilo_hijo, NULL, jugar_ping_pong, &juego) != 0) {
        perror("Error al crear el hilo hijo");
        exit(1);
    }

    pthread_mutex_lock(&juego.mutex);
    pthread_cond_signal(&juego.cond_padre);
    pthread_mutex_unlock(&juego.mutex);

    pthread_join(hilo_padre, NULL);
    pthread_join(hilo_hijo, NULL);

    return 0;
}
