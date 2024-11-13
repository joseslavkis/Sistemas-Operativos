// Escriba un programa en C que permita jugar a dos procesos al ping pong, la pelota es
// un entero, cada vez que un proceso recibe la pelota debe incrementar en 1 su valor.
// ★ El programa debe cortar por overflow o cambio de signo.

#include <stdbool.h>
#include <stdio.h> 

#define ESCRITURA 1
#define LECTURA 0

void jugar_ping_pong(int read_fd, int write_fd) {
    int pelota;

    while (true) {
        if (read(read_fd, &pelota, sizeof(int)) <= 0) {
            perror("Error al leer la pelota");
            exit(1);
        }

        printf("Proceso %d recibió la pelota: %d\n", getpid(), pelota);

        //verificar overflow o cambio de signo
        if (pelota == INT_MAX) {
            printf("Overflow detectado, Proceso %d detiene el juego.\n", getpid());
            break;
        } else if (pelota < 0) {
            printf("cambio de signo detectado, Proceso %d detiene el juego.\n", getpid());
            break;
        }
        pelota++;

        if (write(write_fd, &pelota, sizeof(int)) <= 0) {
            perror("Error al enviar la pelota");
            exit(1);
        }
    }
}


int main() {
    int pipe1[2], pipe2[2];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("Error");
        exit(1);
    }
    int pid = fork();
    if (pid == -1) {
        perror("error");
        exit(1);
    }
    //aquí vamos a usar pipe1 del padre al hijo y pipe2 del hijo al padre
    if (pid == 0) {
        close(pipe1[ESCRITURA]);
        close(pipe2[LECTURA]);
        jugar_ping_pong(pipe1[LECTURA], pipe2[ESCRITURA]);
    } else if (pid > 0) {
        close(pipe1[LECTURA]);
        close(pipe2[ESCRITURA]);

        int pelota = 0;
        if (write(pipe1[1], &pelota, sizeof(int)) <= 0) {
            perror("Error al enviar la pelota inicial");
            exit(1);
        }
        jugar_ping_pong(pipe1[ESCRITURA], pipe2[LECTURA]);

    }













}
















