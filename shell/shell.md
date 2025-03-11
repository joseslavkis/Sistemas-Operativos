# shell

### Búsqueda en $PATH

#### Diferencias entre execve(2) y la familia exec(3)

    Ambas se utilizan para reemplazar el proceso actual con un nuevo proceso, pero tienen diferencias clave en su uso y funcionalidad.
    La syscall execve(2) es una llamada de sistema de bajo nivel que requiere especificar la ruta completa del archivo ejecutable, así como los argumentos y las variables de entorno para el nuevo programa. No realiza ninguna búsqueda en el $PATH, lo que significa que el programador debe proporcionar la ruta exacta del ejecutable. Esta syscall es más directa, pero también es más complicada de usar debido a la necesidad de especificar todos los parámetros.

    Por otro lado, la familia de funciones exec(3) son wrappers de alto nivel que facilitan el uso de execve(2). Estas funciones incluyen execl, execle, execlp, execv, execvp y execvpe, y proporcionan varias formas convenientes de pasar los argumentos y las variables de entorno. Algunas de estas funciones, como execlp y execvp, realizan una búsqueda en el $PATH, lo que simplifica la ejecución de comandos sin necesidad de especificar la ruta completa del ejecutable.

    Por ejemplo, en el archivo exec.c de nuestra shell, utilizamos execvp para ejecutar comandos. Esta función busca el ejecutable en los directorios listados en el $PATH y luego llama a execve para reemplazar el proceso actual con el nuevo proceso. El uso de execvp facilita la ejecución de comandos de manera más intuitiva y menos propensa a errores en comparación con el uso directo de execve.

    En resumen, execve(2) ofrece un mayor control al programador, pero a costa de una mayor complejidad. La familia exec(3), en cambio, proporciona una interfaz más conveniente para la mayoría de los casos comunes, realizando tareas adicionales como la búsqueda en el $PATH y simplificando la especificación de argumentos y variables de entorno. Conocer cuándo y cómo usar cada una de estas funciones es esencial para la programación efectiva de sistemas en C.

#### Errores en exec(3)

    Sí, la llamada a exec(3) puede fallar por varias razones, como la falta de permisos para ejecutar el archivo, la inexistencia del archivo especificado, o la superación de los límites del sistema (por ejemplo, demasiados archivos abiertos). Cuando exec(3) falla, retorna -1 y establece errno para indicar el error específico.

    En la implementación de la shell proporcionada en el archivo exec.c, se maneja el posible fallo de exec(3) de la siguiente manera:

    **Verificación del retorno de execvp**:
        En el caso de la ejecución de un comando (EXEC), se utiliza execvp para ejecutar el comando. Si execvp falla, se imprime un mensaje de error utilizando perror y se llama a _exit(-1) para terminar el proceso hijo con un código de error.
    
        case EXEC:
            e = (struct execcmd *) cmd;
            set_environ_vars(e->eargv, e->eargc);
            if (execvp(e->argv[0], e->argv) < 0) {
                perror("execvp");
                _exit(-1);
            }
            _exit(0);
            break;

### Procesos en segundo plano

#### Explicar detalladamente el mecanismo completo utilizado.
    Cuando se detecta un comando que debe ejecutarse en segundo plano, indicado por el carácter & al final del comando, se realiza la ejecución normal del comando con algunas diferencias clave. En la función run_cmd, después de parsear el comando, se verifica si debe ejecutarse en segundo plano (parsed->type == BACK). Si es así, se crea un nuevo proceso hijo utilizando fork(). En el proceso hijo, se llama recursivamente a exec_cmd para ejecutar el comando en segundo plano.

    En el proceso padre, se llama a handle_background_process para manejar el proceso en segundo plano. Esta función imprime información sobre el proceso y utiliza waitpid con la opción WNOHANG para no bloquear la shell mientras espera que el proceso hijo termine. Finalmente, se libera la memoria utilizada por la estructura del comando con free_command.

    Este mecanismo permite que la shell ejecute comandos en segundo plano sin bloquear la terminal, permitiendo que el usuario continúe ingresando y ejecutando otros comandos.

#### ¿Por qué es necesario el uso de señales?
    El uso de señales es importante para manejar procesos en segundo plano porque permiten notificar al proceso padre cuando un proceso hijo termina (SIGCHLD), manejar interrupciones del usuario (SIGINT, SIGTSTP), sincronizar y limpiar recursos, y ofrecer funcionalidades avanzadas como la suspensión y reanudación de procesos (SIGTSTP, SIGCONT). Sin señales, la gestión de procesos en segundo plano sería ineficiente y propensa a errores.

### Flujo estándar

#### Redirección (2>&1)
    El operador 2>&1, permite redirigir el flujo de error estendar (stderr, cuyo descriptor de archivo es 2) al flujo de salida estendar (stdout, cuyo descriptor de archivo es 1). Para comprender mejor su significado y funcionamiento, explicamos cada una de sus partes: 

    > 2 se refiere al descriptor de archivo para stderr (flujo de error estandar).

    > 1 se refiere al descriptor de archivo para stdout (flujo de salida estandar).



    > El operador (>&) indica que queremos duplicar el descriptor de archivo, lo que significa que cualquier salida que normalmente iria a stderr ahora se enviara a donde apunta stdout.

#### Ejemplo cat out.txt
    ($ ls -C /home /noexiste >out.txt 2>&1):
        En este caso, la salida estandar del comando (ls) como la salida de error se almacenaran juntas en el archivo out.txt. El contenido que queda cunado 
        ejecutamos cat out.txt, veremos tanto la lista de archivos de /home como el mensaje de error de /noexiste dentro de out.txt.

#### Orden invertido
    ($ ls -C /home /noexiste 2>&1 >out.txt):
        En este caso, primero redirigimos stderr al mismo lugar que stdout, pero luego cambiamos stdout para que apunte a out.txt. Esto significa que solo stdout se redirige a out.txt, mientras que stderr se muestra en la terminal.

### Tuberías múltiples

#### ¿Cambia en algo el exit code reportado por la shell si se ejecuta un pipe?

Si cambia, el exit code reportado por la shell puede cambiar cuando se ejecuta un pipe. En un pipeline, la shell generalmente reporta el exit code del último comando en la cadena de pipes. Esto significa que si el último comando en el pipeline se ejecuta correctamente, el exit code será 0, incluso si uno de los comandos anteriores falló.

#### ¿Qué ocurre si, en un pipe, alguno de los comandos falla?

Si alguno de los comandos en un pipeline falla, el exit code del pipeline dependerá del exit code del último comando.
Si el último comando se ejecuta correctamente, el exit code del pipeline será 0, a pesar de que uno de los comandos anteriores falló. 
Si el último comando falla, el exit code del pipeline será el exit code de ese comando fallido.

##### Ejemplo en Bash
$ ls /non | grep "something"
ls: cannot access '/non': No such file or directory
$ echo $?
1

##### Ejemplo en nuestra shell
$ ls /non | grep "algo"
ls: no se puede acceder a '/non': No existe el archivo o el directorio
$ echo $?
0
	Program: [echo $?] exited, status: 0 


### Variables de entorno temporarias

#### Por que es necesario hacerlo luego de la llamada a fork(2)?

    Es necesario hacerlo despues de la llamada a fork() porque fork() crea una copia exacta del proceso padre en un proceso hijo. Si ajustas las variables de entorno antes del fork(), las variables de entorno tambien se modificarian en el proceso padre. Como las variables de entorno deben ser temporales y aplicarse solo al proceso hijo que ejecutara el comando, debes hacerlo despues del fork() y antes del exec() en el proceso hijo.

#### Sobre el uso de las funciones de la familia exec(3) con el tercer argumento para las variables de entorno:

    El comportamiento resultante al pasar las variables de entorno a traves del tercer argumento de una funcion como execle() o execve() es diferente que si usas setenv() en el proceso hijo.

    > Con setenv(), las variables de entorno temporales se agregan al entorno del proceso hijo, que luego se hereda por la llamada a execvp() o cualquier otra funcion exec.

    > Con execle() o execve(), puedes pasar un entorno personalizado para el nuevo proceso, pero el entorno del proceso hijo original no se modifica.

    
### Pseudo-variables

#### Tres variables magicas adicionales
    Ademas de $?, otras variables magicas estandar que investigamos en la shell son:
    
    1. ($$)
        > Imprime el ID del PID de la shell actual

        > En bash: echo $$ 
        > Retorno: (PID_ID de la shell actual)

    2. ($#)
        > Imprime el numero de argumentos posicionales pasados al script o a una funcion.

        > En Bash: echo $#
        > Retorno: (#argumentos)
    
    3. ($0)
        > Imprime el nombre del script o el programa que se esta ejecutando

        > En Bash: echo $0
        > Retorno: (nombre script o programa)

### Comandos built-in

#### ¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué?
    > cd no se podria implementar correctamente sin ser built-in. Esto se debe a que un proceso hijo creado por la shell para ejecutar un comando externo (si cd no fuera built-in) modificaria su propio directorio de trabajo, pero este cambio no afectaria al proceso de la shell original. Para cambiar el directorio de trabajo de la shell, el comando debe ejecutarse dentro del mismo proceso de la shell.

    > pwd, por otro lado, si podria ser implementado sin ser built-in. Al ejecutar pwd en un proceso hijo, este puede obtener la ruta actual con getcwd() e imprimirla sin que sea necesario modificar el proceso principal de la shell. Sin embargo, implementarlo como built-in es mas eficiente, ya que evita la sobrecarga de crear un nuevo proceso solo para imprimir el directorio actual.
    
#### ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in?

    La implementacion como built-in de pwd es conveniente para evitar el costo de crear y destruir procesos. En shells modernas, comandos simples como true, false o pwd se implementan como built-ins para maximizar la eficiencia.

### Historial

---
