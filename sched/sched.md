# sched

## Las fotos del context switch son adjuntadas sisop_2024b_g20/sched 
(en el header del gdb aparece la instrucción en la cual esta el breakpoint)

Lugar para respuestas en prosa, seguimientos con GDB y documentación del TP.

## Ejemplos de ganancia y pérdida de prioridad en el scheduler

Este scheduler utiliza un mecanismo de prioridad múltiple para organizar la ejecución de procesos.

- **Pérdida de Prioridad**: Un proceso que usa excesivamente la CPU puede perder prioridad con el tiempo. Esto se da cuando el proceso alcanza una cantidad fija de ejecuciones (especificada por `MAX_EXECUTIONS_PER_SLICE`). Al llegar a ese límite, el scheduler degrada la prioridad del proceso, moviéndolo a una cola de prioridad inferior. Esto ayuda a evitar que un solo proceso monopolice la CPU, permitiendo que otros procesos también puedan ser ejecutados.

- **Ganancia de Prioridad**: Ocasionalmente, el scheduler realiza una "elevación de prioridad" o "boost" a todos los procesos, asignándoles la prioridad más alta. Esto sucede cada cierto número de llamadas a `sched_yield` (definido por `BOOST_INTERVAL`). Al reiniciar las prioridades, se previene la starvation, un fenómeno en el que los procesos de baja prioridad podrían no ejecutarse nunca debido a la presencia de procesos con prioridad más alta. Con este boost, los procesos de menor prioridad tienen la oportunidad de ejecutarse después de que su prioridad ha sido elevada.

Ambos casos, ganancia y pérdida de prioridad, también pueden ocurrir en respuesta a interrupciones del sistema, como llamadas a funciones del sistema (syscalls).

## Explicación de la lógica del planificador por prioridades

Dado que cada proceso tiene ahora asignada una prioridad, el scheduler utiliza esta prioridad para decidir cuál proceso ejecutar a continuación. Este scheduler cuenta con varias colas (de 0 a 4, la 0 la mejor y la 4 la peor), donde organiza los procesos según su nivel de prioridad. Por ejemplo:

1. Los procesos con la prioridad más alta se encolan en la cola correspondiente a dicha prioridad. A medida que las prioridades disminuyen, los procesos se colocan en las colas de menor prioridad.

2. El scheduler revisa cada cola, desde la de mayor prioridad hacia abajo, para encontrar el primer proceso listo para ejecutarse. Si hay varios procesos en la misma cola de prioridad, se usa round-robin entre ellos.

3. Si un proceso alcanza el límite de ejecuciones permitidas por su porción de tiempo (`MAX_EXECUTIONS_PER_SLICE`), su prioridad se degrada, moviéndolo a la siguiente cola de menor prioridad.

Este esquema asegura que los procesos más prioritarios obtengan un acceso preferencial a la CPU, mientras que los procesos de menor prioridad aún tienen la posibilidad de ejecutarse periódicamente gracias al mecanismo de "boost" que eleva las prioridades de todos los procesos cada cierto tiempo.
