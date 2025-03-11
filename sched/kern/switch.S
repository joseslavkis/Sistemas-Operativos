/*
 * Your code here for context_switch
 *
 * It should restore the register values in the Trapframe with the 'iret' instruction.
 * This exits the kernel and starts executing some environment's code.
 *
 * This function does not return.
 */

.globl context_switch;
context_switch:

    mov 4(%esp), %esp # skipear el return adress(ajusto el stack pointer), no es parte del trapframe

    popal # pongo los valores de los registros generales en la pila(ebx, ecx...)
    pop %es # pongo en el stack los registros de segmento que no se incluyen en el popal
    pop %ds

    add $8, %esp # skipear los valores de los registros eip y cs
    iret # salto a la tarea
spin:
	jmp spin # no deberia llegar aca porque es bucle infinito
