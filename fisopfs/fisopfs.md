# Informe del Diseño del Sistema de Archivos FisopFS

Este documento describe las decisiones de diseño, estructuras en memoria, y aspectos técnicos del sistema de archivos FisopFS implementado en C utilizando FUSE. El sistema opera enteramente en memoria RAM y permite gestionar archivos y directorios con sus respectivos metadatos.

## 1. Estructuras en memoria para almacenar archivos, directorios y metadatos

### `struct file_system` 
El sistema de archivos utiliza una estructura principal llamada `file_system`, que contiene un arreglo fijo de bloques (`blocks`). Cada bloque representa un archivo o directorio y tiene la siguiente estructura:

#### `struct block_t`
- **`status`**: Indica si el bloque está en uso (1) o libre (0).
- **`type`**: Tipo de bloque (archivo o directorio).
- **`mode`**: Permisos del archivo/directorio (modo POSIX).
- **`size`**: Tamaño del archivo o directorio.
- **`uid`, `gid`**: Identificadores del usuario y grupo propietarios.
- **`last_access`, `last_modification`, `creation_time`**: Marcas de tiempo.
- **`path`**: Ruta completa del archivo/directorio.
- **`content`**: Contenido almacenado (solo para archivos).
- **`dir_path`**: Path del directorio padre (solo para directorios).

### Bloque raíz
El bloque raíz (`/`) se inicializa al inicio del sistema mediante la función `initialize_filesystem()`. Representa el directorio raíz y tiene un estado activo, tipo de directorio y permisos predeterminados.


## 2. Cómo el sistema de archivos encuentra un archivo específico dado un path

El sistema utiliza dos funciones auxiliares principales:

### `get_parent_path(const char *path)`
Devuelve el path del directorio padre dado un path completo. Ejemplo:
- Entrada: `/home/user/file.txt`
- Salida: `/home/user`

### `get_name(const char *path)`
Devuelve el nombre del archivo o directorio a partir de un path completo. Ejemplo:
- Entrada: `/home/user/file.txt`
- Salida: `file.txt`

La combinación de estas funciones permite recorrer la estructura jerárquica almacenada en memoria.


## 3. Estructuras auxiliares utilizadas

El archivo `defs.h` proporciona definiciones y estructuras auxiliares fundamentales para el funcionamiento del sistema de archivos. A continuación, se destacan los elementos clave:

### Constantes y Definiciones
- **`MAX_FILENAME`**: Longitud máxima de un nombre de archivo o directorio (100 caracteres).
- **`MAX_FILE_SIZE`**: Tamaño máximo de un archivo (1024 bytes).
- **`MAX_DIR_SIZE`**: Tamaño máximo de un directorio (1024 bytes).
- **`NUM_BLOCKS_MAX`**: Número máximo de bloques permitidos en el sistema (100).

### Tipos de Datos Auxiliares
1. **`file_type_t`** (enumeración):
   - **`FILE_TYPE_FILE`**: Representa un bloque de tipo archivo.
   - **`FILE_TYPE_DIR`**: Representa un bloque de tipo directorio.

### Estructuras Principales
1. **`block_t`**:
   Cada bloque puede representar un archivo o un directorio. Incluye:
   - **`type`**: Tipo de bloque (archivo/directorio).
   - **`mode`**: Permisos POSIX.
   - **`status`**: Estado (activo/libre).
   - **`content`**: Contenido del archivo (si aplica).
   - **`size`**: Tamaño del bloque.
   - **`last_access`, `last_modification`, `creation_time`**: Tiempos asociados.
   - **`path`, `dir_path`**: Rutas completas del bloque y su directorio padre.
   - **`uid`, `gid`**: Identificadores de usuario y grupo.

2. **`struct file_system`**:
   - Contiene el arreglo `blocks[NUM_BLOCKS_MAX]`, que actúa como tabla maestra para organizar los bloques en memoria.

### Relevancia de las Estructuras Auxiliares
- **Organización en memoria**: Facilitan la gestión eficiente de límites y estructuras.
- **Modularidad**: Encapsulan toda la lógica del almacenamiento en memoria.
- **Escalabilidad**: Aunque las capacidades están limitadas por las constantes, estas pueden ajustarse fácilmente para diferentes escenarios.


## 4. Formato de serialización del sistema de archivos en disco

El sistema de archivos se serializa directamente al disco como una estructura binaria que contiene los bloques en secuencia. Cada bloque incluye todos los campos definidos en `block_t`. No se serializan metadatos generales como la cantidad total de bloques o información del bloque raíz.
- **Contenido serializado**: 
   - Cada bloque incluye todos los campos definidos en `block_t`: estado, tipo, permisos, tamaño, usuario, grupo, marcas de tiempo, rutas y contenido (si aplica).
   - No se guarda explícitamente metadata adicional como la cantidad total de bloques o información del bloque raíz, ya que esta información está implícita en el diseño del sistema de archivos.
- **Esquema en disco**: 
   - Los bloques se almacenan en secuencia dentro de un archivo binario (`file.fisopfs` por defecto).
   - Durante la lectura, la estructura completa del sistema se reconstruye directamente desde el archivo.


## 5. Decisiones e información adicional

- **Escalabilidad**: El sistema utiliza un número fijo de bloques definido por la constante `NUM_BLOCKS_MAX`.
- **Eficiencia**: La búsqueda de bloques libres y la navegación por paths se realiza linealmente, lo cual podría mejorarse utilizando índices o tablas hash.
- **Persistencia**: Aunque el sistema opera en RAM, se define un archivo de respaldo (`DEFAULT_FILE_DISK`) para fines de serialización. No ejeuctar ctrl+z para finalizar la ejecución luego de ./fisopfs -f prueba/, sino no se dará la persistencia.

---

### Diagrama.
[Descargar el diagrama del diseño del sistema de archivos](fisopfs_diagrama.png)

---

## Pruebas y salidas esperadas.

Todas estas salidas serán realizadas en base a ejecutar ./fisopfs -f prueba/.
Se dividirá en 2 partes por comando, la salida en la terminal donde se ejecuta make docker-run (R) y otra en la terminal donde se ejecuta make docker-attach (A).

*cd*
-A:
root@85c84bf721d7:/fisopfs# cd prueba/
root@85c84bf721d7:/fisopfs/prueba# 
-R:
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /

*touch*
-A:
root@85c84bf721d7:/fisopfs/prueba# touch hola.txt
root@85c84bf721d7:/fisopfs/prueba# 
-R:
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] fisopfs_readdir(/)
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] getattr: (/hola.txt)
[debug] Error get block
[debug] Error in getattr: No such file or directory
[debug] fisopfs_mkdir - path: /hola.txt
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt
[debug] fisopfs_flush - path: /hola.txt
[debug] Filesystem flushed successfully.
[debug] Updating timestamps for path: /hola.txt
[debug] Timestamps updated successfully.
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt
[debug] fisopfs_flush - path: /hola.txt
[debug] Filesystem flushed successfully.

*echo*
-A:
root@85c84bf721d7:/fisopfs/prueba# echo "Buenas tardes!" > hola.txt 
-R:
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] fisopfs_readdir(/)
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] fisopfs_readdir(/)
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt
[debug] fisopfs_open - path: /hola.txt
[debug] File /hola.txt opened successfully. Updated last_access to 1733056691
[debug] fisopfs_truncate - path: /hola.txt, size: 0
[debug] Truncate successful for path: /hola.txt, new size: 0
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt
[debug] fisopfs_flush - path: /hola.txt
[debug] Filesystem flushed successfully.
[debug] fisopfs_write - path: /hola.txt, size: 15, offset: 0
[debug] Written content: Buenas tardes!

[debug] fisopfs_flush - path: /hola.txt
[debug] Filesystem flushed successfully.

*stat*
-A:
root@85c84bf721d7:/fisopfs/prueba# stat hola.txt 
  File: hola.txt
  Size: 15        	Blocks: 0          IO Block: 4096   regular file
Device: 6dh/109d	Inode: 2           Links: 1
Access: (0644/-rw-r--r--)  Uid: (    0/    root)   Gid: (    0/    root)
Access: 2024-12-01 12:38:11.000000000 +0000
Modify: 2024-12-01 12:38:11.000000000 +0000
Change: 1970-01-01 00:00:00.000000000 +0000
Birth: -
-R:
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] fisopfs_readdir(/)
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt

*cat*
-A:
root@85c84bf721d7:/fisopfs/prueba# cat hola.txt 
Buenas tardes!
-R:
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt
[debug] fisopfs_open - path: /hola.txt
[debug] File /hola.txt opened successfully. Updated last_access to 1733056905
[debug] fisopfs_read - path: /hola.txt, offset: 0, size: 4096
[debug] read content: Buenas tardes!

[debug] fisopfs_flush - path: /hola.txt
[debug] Filesystem flushed successfully.

*rm*
-A:
root@85c84bf721d7:/fisopfs/prueba# rm hola.txt 
root@85c84bf721d7:/fisopfs/prueba# ls
root@85c84bf721d7:/fisopfs/prueba# 
-R:
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] fisopfs_readdir(/)
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt
[debug] fisopfs_unlink - path: /hola.txt
[debug] File unlinked successfully: /hola.txt
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] fisopfs_readdir(/)

*mkdir*
-A:
root@85c84bf721d7:/fisopfs/prueba# mkdir testing
root@85c84bf721d7:/fisopfs/prueba# ls
testing
root@85c84bf721d7:/fisopfs/prueba# 
-R:
[debug] fisopfs_readdir(/)
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] getattr: (/testing)
[debug] Error get block
[debug] Error in getattr: No such file or directory
[debug] fisopfs_mkdir - path: /testing
[debug] getattr: (/testing)
[debug] Attributes retrieved successfully for path: /testing
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] fisopfs_readdir(/)
[debug] getattr: (/testing)
[debug] Attributes retrieved successfully for path: /testing

*cd <dir>*
-A:
root@85c84bf721d7:/fisopfs/prueba# cd testing/
root@85c84bf721d7:/fisopfs/prueba/testing# ls
root@85c84bf721d7:/fisopfs/prueba/testing# 
-R:
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] fisopfs_readdir(/)
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] getattr: (/testing)
[debug] Attributes retrieved successfully for path: /testing
[debug] getattr: (/testing)
[debug] Attributes retrieved successfully for path: /testing
[debug] fisopfs_readdir(/testing)

*rmdir*
-A:
root@85c84bf721d7:/fisopfs/prueba# rmdir testing/
root@85c84bf721d7:/fisopfs/prueba# ls
root@85c84bf721d7:/fisopfs/prueba# 
-R:
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] getattr: (/testing)
[debug] Attributes retrieved successfully for path: /testing
[debug] fisopfs_rmdir - path: /testing
[debug] Directory removed successfully: /testing
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] fisopfs_readdir(/)

(Para la siguiente prueba se creó un archivo hola.txt de nuevo, cuyo contenido era "Como andan fisopfs!")
root@85c84bf721d7:/fisopfs/prueba# echo "Como andan fisopfs!" > hola.txt

*truncate*
-A:
root@85c84bf721d7:/fisopfs/prueba# truncate -s 2 hola.txt
root@85c84bf721d7:/fisopfs/prueba# cat hola.txt //para ver el contenido del hola.txt truncado a 2 bytes
Coroot@85c84bf721d7:/fisopfs/prueba# //vease que se imprime el "Co", los primeros 2 bytes de "Como andan fisopfs!"
-R:
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt
[debug] fisopfs_open - path: /hola.txt
[debug] File /hola.txt opened successfully. Updated last_access to 1733057458
[debug] fisopfs_truncate - path: /hola.txt, size: 2
[debug] Truncate successful for path: /hola.txt, new size: 2
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt
[debug] fisopfs_flush - path: /hola.txt
[debug] Filesystem flushed successfully.
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] fisopfs_readdir(/)
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt
[debug] fisopfs_open - path: /hola.txt
[debug] File /hola.txt opened successfully. Updated last_access to 1733057466
[debug] fisopfs_read - path: /hola.txt, offset: 0, size: 4096
[debug] read content: Co
[debug] fisopfs_flush - path: /hola.txt
[debug] Filesystem flushed successfully.

*chmod*
-A:
root@85c84bf721d7:/fisopfs/prueba# chmod 755 hola.txt
-R:
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt
[debug] fisopfs_chmod - path: /hola.txt, mode: 100755
[debug] Permissions updated for /hola.txt. New mode: 100755
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt

*cp*
-A:
root@85c84bf721d7:/fisopfs/prueba# cp hola.txt hola2.txt 
root@85c84bf721d7:/fisopfs/prueba# cat hola2.txt 
Coroot@85c84bf721d7:/fisopfs/prueba# //vease que hola2.txt ahora tiene el "Co" de hola.txt
-R:
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] fisopfs_readdir(/)
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] getattr: (/hola2.txt)
[debug] Attributes retrieved successfully for path: /hola2.txt
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] getattr: (/hola2.txt)
[debug] Attributes retrieved successfully for path: /hola2.txt
[debug] getattr: (/hola.txt)
[debug] Attributes retrieved successfully for path: /hola.txt
[debug] fisopfs_open - path: /hola.txt
[debug] File /hola.txt opened successfully. Updated last_access to 1733057881
[debug] fisopfs_open - path: /hola2.txt
[debug] File /hola2.txt opened successfully. Updated last_access to 1733057881
[debug] fisopfs_truncate - path: /hola2.txt, size: 0
[debug] Truncate successful for path: /hola2.txt, new size: 0
[debug] getattr: (/hola2.txt)
[debug] Attributes retrieved successfully for path: /hola2.txt
[debug] fisopfs_read - path: /hola.txt, offset: 0, size: 4096
[debug] read content: Co
[debug] fisopfs_write - path: /hola2.txt, size: 2, offset: 0
[debug] Written content: Co
[debug] fisopfs_flush - path: /hola2.txt
[debug] Filesystem flushed successfully.
[debug] fisopfs_flush - path: /hola.txt
[debug] Filesystem flushed successfully.
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] fisopfs_readdir(/)
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] fisopfs_readdir(/)
[debug] getattr: (/)
[debug] Attributes retrieved successfully for path: /
[debug] getattr: (/hola2.txt)
[debug] Attributes retrieved successfully for path: /hola2.txt
[debug] fisopfs_open - path: /hola2.txt
[debug] File /hola2.txt opened successfully. Updated last_access to 1733057887
[debug] fisopfs_read - path: /hola2.txt, offset: 0, size: 4096
[debug] read content: Co
[debug] fisopfs_flush - path: /hola2.txt
[debug] Filesystem flushed successfully.

*ctrl+c*
-R:
^C[debug] Destroying filesystem and persisting to disk.
[debug] Filesystem persisted successfully.
[debug] Filesystem memory cleared.
root@85c84bf721d7:/fisopfs#
