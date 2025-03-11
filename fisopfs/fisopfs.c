#define FUSE_USE_VERSION 30

#define _POSIX_C_SOURCE 200809L

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>


#include "defs.h"

#define DEFAULT_FILE_DISK "file.fisopfs"

char *filedisk = DEFAULT_FILE_DISK;
struct file_system filesystem;

#ifndef HAVE_STRDUP
char *
strdup(const char *s)
{
	size_t len = strlen(s) + 1;
	void *new = malloc(len);

	if (new == NULL)
		return NULL;

	return (char *) memcpy(new, s, len);
}
#endif

// obtiene el path del directorio padre a partir de un path

static char *
get_parent_path(const char *path)
{
	// buscar la última ocurrencia de "/"
	const char *last_slash = strrchr(path, '/');
	if (last_slash == NULL) {
		// si no se encuentra "/", devolver una cadena vacía(raiz)
		char *root_path = strdup("");
		return root_path;
	}

	// calcular la longitud del path del directorio padre
	size_t parent_path_len = last_slash - path;
	char *parent_path = (char *) malloc(parent_path_len + 1);
	if (parent_path == NULL) {
		return NULL;
	}

	// copiar el path del directorio padre
	strncpy(parent_path, path, parent_path_len);
	parent_path[parent_path_len] = '\0';

	return parent_path;
}

// Initialize the filesystem

int
initialize_filesystem()
{
	block_t root_block = {
		.status = 1,
		.type = FILE_TYPE_DIR,
		.mode = __S_IFDIR | 0755,
		.size = MAX_DIR_SIZE,
		.uid = 1717,
		.gid = getgid(),
		.last_access = time(NULL),
		.last_modification = time(NULL),
		.creation_time = time(NULL),
	};

	strcpy(root_block.path, "/");
	memset(root_block.content, 0, sizeof(root_block.content));
	strcpy(root_block.dir_path, "");

	filesystem.blocks[0] = root_block;

	// inicializar todos los bloques restantes
	for (int i = 1; i < NUM_BLOCKS_MAX; i++) {
		filesystem.blocks[i] =
		        (block_t){ 0 };  // inicializar el bloque a cero
	}

	return 0;
}

// obtiene el nombre del archivo o directorio a partir de un path

static char *
get_name(const char *path)
{
	// si el path es "/", devolverlo directamente
	if (strcmp(path, "/") == 0) {
		return strdup(path);
	}

	// crear una copia del path sin el primer carácter "/"
	char *filename = strdup(path + 1);
	if (filename == NULL) {
		return NULL;
	}

	// buscar la última ocurrencia de "/"
	const char *last_slash = strrchr(path, '/');
	if (last_slash == NULL) {
		return filename;
	}

	// crear una copia del nombre del archivo o directorio
	char *_name = strdup(last_slash + 1);
	if (_name == NULL) {
		free(filename);
		return NULL;
	}

	free(filename);
	return _name;
}

// obtiene primer bloque libre(devuelve un pointer a un bloque libre)

static block_t *
free_block_pointer()
{
	// buscar el bloque correspondiente al nombre
	for (int i = 0; i < NUM_BLOCKS_MAX; i++) {
		if (filesystem.blocks[i].status == 0) {
			return &filesystem.blocks[i];
		}
	}

	// si no se encuentra el bloque, devolver NULL
	printf("[debug] Error get free block\n");
	return NULL;
}

// obtiene el puntero a un bloque de un determinado path

static block_t *
get_block_pointer(const char *path)
{
	// parsear el path para obtener el nombre del archivo o directorio
	char *filename = get_name(path);
	if (!filename) {
		return NULL;
	}

	// buscar el bloque correspondiente al nombre
	for (int i = 0; i < NUM_BLOCKS_MAX; i++) {
		if (strcmp(filesystem.blocks[i].path, filename) == 0) {
			return &filesystem.blocks[i];
		}
	}

	// si no se encuentra el bloque, devolver NULL
	printf("[debug] Error get block\n");
	return NULL;
}

// Get file attributes.

// Similar to stat(). The 'st_dev' and 'st_blksize' fields are ignored.
// The 'st_ino' field is ignored except if the 'use_ino' mount option is given.
// In that case it is passed to userspace,
// but libfuse and the kernel will still assign a different inode for internal use (called the "nodeid").

// se testea con stat pero igualmente si anda mal getattr, fallan todos los demas(getattr se llama para todo)
static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] getattr: (%s)\n", path);

	// buscar el bloque asociado al path
	block_t *target_block = get_block_pointer(path);
	if (target_block == NULL) {
		fprintf(stderr, "[debug] Error in getattr: %s\n", strerror(ENOENT));
		return -ENOENT;
	}
	// inicializar estructura stat
	memset(st, 0, sizeof(struct stat));
	st->st_dev = 0;  // dispositivo(no relevante en este caso)
	st->st_uid = target_block->uid;
	st->st_gid = target_block->gid;
	st->st_size = target_block->size;
	st->st_mode = target_block->mode;
	st->st_atime = target_block->last_access;
	st->st_mtime = target_block->last_modification;
	st->st_ctime = target_block->creation_time;
	st->st_nlink = 2;  // deafult

	// ajustar atributos según el tipo del bloque
	if (target_block->type == FILE_TYPE_FILE) {
		st->st_mode =
		        S_IFREG | 0644;  // archivo regular con permisos 0644
		st->st_nlink = 1;        // Un enlace para archivos
	} else if (target_block->type == FILE_TYPE_DIR) {
		st->st_mode = S_IFDIR | 0755;  // directorio con permisos 0755
	}

	printf("[debug] Attributes retrieved successfully for path: %s\n", path);
	return 0;  // exito
}

// Read directory
// se testea con ls

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir(%s)\n", path);

	// agregar los directorios "." y ".."
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	// obtener el bloque correspondiente al path
	block_t *block = get_block_pointer(path);
	if (!block) {
		fprintf(stderr, "[debug] Error in readdir: %s\n", strerror(errno));
		return -ENOENT;
	}

	// actualizar el tiempo de último acceso
	block->last_access = time(NULL);

	// verificar si el bloque es un directorio
	if (block->type != FILE_TYPE_DIR) {
		fprintf(stderr, "[debug] Error in readdir: %s\n", strerror(errno));
		return -ENOTDIR;
	}

	// llenar el buffer con los archivos y directorios contenidos
	for (int i = 1; i < NUM_BLOCKS_MAX; i++) {
		block_t *current_block = &filesystem.blocks[i];
		if (current_block->status == 1 &&
		    strcmp(current_block->dir_path, block->path) == 0) {
			filler(buffer, current_block->path, NULL, 0);
		}
	}
	return 0;
}


#define MAX_CONTENIDO 100
static char fisop_file_contenidos[MAX_CONTENIDO] = "hola fisopfs!\n";

// Read data from an open file

// Read should return exactly the number of bytes requested except on EOF or error,
// otherwise the rest of the data will be substituted with zeroes. An exception
// to this is when the 'direct_io' mount option is specified, in which case the
// return value of the read system call will reflect the return value of this operation.

// se testea con cat
static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read - path: %s, offset: %lu, size: %lu\n",
	       path,
	       offset,
	       size);

	block_t *block = get_block_pointer(path);
	if (!block) {
		return -ENOENT;
	}

	if (block->type == FILE_TYPE_DIR) {
		return -EISDIR;
	}

	if (offset >= block->size) {
		return -EINVAL;
	}
	char *content = block->content;
	size_t _size = block->size;

	if (_size - offset < size) {
		size = _size - offset;
	}
	block->last_access = time(NULL);
	strncpy(buffer, content + offset, size);
	printf("[debug] read content: %s\n", buffer);
	return size;
}

// Inicializar el inodo con los valores proporcionados

int
initialize_inode(block_t *inode,
                 const char *name,
                 const char *parent_path,
                 mode_t mode,
                 int type)
{
	// inicializar el inodo con los valores proporcionados
	inode->uid = getuid();
	inode->gid = getgid();
	inode->last_access = time(NULL);
	inode->last_modification = time(NULL);
	inode->type = type;
	inode->status = 1;
	inode->mode = mode;
	inode->size = 0;
	memset(inode->content, 0, sizeof(inode->content));  // limpiar contenido

	// copiar el nombre y la ruta del directorio padre al inodo
	strcpy(inode->path, name);
	strcpy(inode->dir_path, parent_path);

	return 0;
}

// Create a new inode for the file or directory

int
set_new_inode(const char *path, mode_t mode, int type)
{
	// validar la longitud del path
	size_t path_len = strlen(path);
	if (path_len - 1 > MAX_FILE_SIZE) {
		fprintf(stderr,
		        "[debug] Error set_new_inode: %s\n",
		        strerror(ENAMETOOLONG));
		return -ENAMETOOLONG;
	}

	// parsear el path para obtener el nombre
	char *name = get_name(path);
	if (!name) {
		return -1;
	}

	// obtener el bloque libre
	block_t *inode = free_block_pointer();
	if (!inode) {
		fprintf(stderr, "No hay espacio disponible.\n");
		free(name);
		return -ENOSPC;
	}

	// obtener la ruta del directorio padre
	char *parent_path = get_parent_path(path);
	if (!parent_path) {
		free(name);
		return -1;
	}

	// asignar "/" si no hay un directorio padre explícito
	if (strlen(parent_path) == 0) {
		strcpy(parent_path, "/");
	}

	// inicializar el inodo utilizando una función auxiliar
	int result = initialize_inode(inode, name, parent_path, mode, type);

	// liberar memoria utilizada para el nombre y el directorio padre
	free(name);
	free(parent_path);

	return result;
}

// Initialize filesystem

// The return value will passed in the private_data field of struct fuse_context
// to all file operations, and as a parameter to the destroy() method. It
// overrides the initial value provided to fuse_main() / fuse_new().

// se testea cuando se hace ./fisopfs -f prueba/
void *
fisopfs_init(struct fuse_conn_info *conn)
{
	printf("[debug] Initializing filesystem.\n");

	FILE *file = fopen(filedisk, "r");
	if (file == NULL) {
		printf("[debug] No existing filesystem found, creating a new "
		       "one.\n");
		initialize_filesystem();
	} else {
		size_t read_size =
		        fread(&filesystem, sizeof(filesystem), 1, file);
		if (read_size != 1) {
			fprintf(stderr,
			        "[debug] Error reading filesystem: %s\n",
			        strerror(errno));
			fclose(file);
			return NULL;
		}
		fclose(file);
		printf("[debug] Filesystem loaded successfully.\n");
	}

	return NULL;
}

// Create and open a file

// If the file does not exist, first create it with the specified mode, and then open it.

// esta funcion se testea con touch aunque el fs prioriza create en vez de mknod
static int
fisopfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	printf("[debug] fisopfs_mknod - path: %s, mode: %o\n", path, mode);

	// verificar si el tipo de archivo es regular
	if (!S_ISREG(mode)) {
		fprintf(stderr, "[debug] Error mknod: only regular files are supported\n");
		return -EINVAL;
	}

	// crear un nuevo inodo para el archivo
	int result = set_new_inode(path, mode, FILE_TYPE_FILE);
	if (result != 0) {
		return result;
	}

	return 0;
}

// Create a file

// testea con touch

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_mkdir - path: %s\n", path);
	if (set_new_inode(path, mode, FILE_TYPE_FILE) != 0) {
		return -1;
	}
	return 0;
}

// Create a directory
// se testea con mkdir obviamente

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_mkdir - path: %s\n", path);
	if (set_new_inode(path, mode, FILE_TYPE_DIR) != 0) {
		return -1;
	}
	return 0;
}

// Write data to an open file

// Write should return exactly the number of bytes requested except on error.
// An exception to this is when the 'direct_io' mount option is specified (see read operation).

// Unless FUSE_CAP_HANDLE_KILLPRIV is disabled, this method is expected to reset the setuid and setgid bits.

// se testea con echo

static int
fisopfs_write(const char *path,
              const char *buffer,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_write - path: %s, size: %lu, offset: %lu\n",
	       path,
	       size,
	       offset);

	// obtener el bloque correspondiente al path
	block_t *block = get_block_pointer(path);
	if (!block) {
		fprintf(stderr, "[debug] Error write: %s\n", strerror(errno));
		return -ENOENT;
	}

	// verificar si el bloque es un archivo
	if (block->type != FILE_TYPE_FILE) {
		fprintf(stderr, "[debug] Error write: %s\n", strerror(errno));
		return -EISDIR;
	}

	// verificar si el offset es válido
	if (offset > MAX_FILE_SIZE) {
		fprintf(stderr, "[debug] Error write: %s\n", strerror(EFBIG));
		return -EFBIG;
	}

	// calcular el tamaño a escribir
	size_t write_size = size;
	if (offset + size > MAX_FILE_SIZE) {
		write_size = MAX_FILE_SIZE - offset;
	}

	// escribir los datos en el archivo
	memcpy(block->content + offset, buffer, write_size);
	block->size = offset + write_size;

	// actualizar los tiempos de acceso y modificación
	block->last_access = time(NULL);
	block->last_modification = time(NULL);

	printf("[debug] Written content: %s\n", buffer);
	return write_size;
}
// Remove a file
// se testea con rm

static int
fisopfs_unlink(const char *path)
{
	printf("[debug] fisopfs_unlink - path: %s\n", path);

	// obtener el bloque correspondiente al path
	block_t *block = get_block_pointer(path);
	if (!block) {
		fprintf(stderr, "[debug] Error unlink: %s\n", strerror(ENOENT));
		return -ENOENT;
	}

	// verificar si el bloque es un archivo
	if (block->type != FILE_TYPE_FILE) {
		fprintf(stderr, "[debug] Error unlink: %s\n", strerror(EISDIR));
		return -EISDIR;
	}

	// marcar el bloque como libre
	block->status = 0;
	memset(block, 0, sizeof(block_t));

	printf("[debug] File unlinked successfully: %s\n", path);
	return 0;
}

// remove a directory
// se testea con rmdir

static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] fisopfs_rmdir - path: %s\n", path);

	// obtener el bloque correspondiente al path
	block_t *block = get_block_pointer(path);
	if (!block) {
		fprintf(stderr, "[debug] Error rmdir: %s\n", strerror(ENOENT));
		return -ENOENT;
	}

	// verificar si el bloque es un directorio
	if (block->type != FILE_TYPE_DIR) {
		fprintf(stderr, "[debug] Error rmdir: %s\n", strerror(ENOTDIR));
		return -ENOTDIR;
	}

	// verificar si el directorio está vacío
	for (int i = 0; i < NUM_BLOCKS_MAX; i++) {
		if (filesystem.blocks[i].status == 1 &&
		    strcmp(filesystem.blocks[i].dir_path, path) == 0) {
			fprintf(stderr,
			        "[debug] Error rmdir: %s\n",
			        strerror(ENOTEMPTY));
			return -ENOTEMPTY;
		}
	}

	// marcar el bloque como libre
	block->status = 0;
	memset(block, 0, sizeof(block_t));

	printf("[debug] Directory removed successfully: %s\n", path);
	return 0;
}

// Change the size of a file

// fi will always be NULL if the file is not currently open, but may also be NULL if the file is open.

// Unless FUSE_CAP_HANDLE_KILLPRIV is disabled, this method is expected to reset
// the setuid and setgid bits
// es necesaria para escribir, xq sino no podes modificar el tamaño del mismo
// cuando escribis algo se testea con echo ya que sin echo no podrías escribir
// en un archivo(igualmente se puede ejecutar truncate)
static int
fisopfs_truncate(const char *path, off_t size)
{
	printf("[debug] fisopfs_truncate - path: %s, size: %ld\n", path, size);

	// obtener el bloque correspondiente al path
	block_t *block = get_block_pointer(path);
	if (!block) {
		fprintf(stderr, "[debug] Error truncate: %s\n", strerror(ENOENT));
		return -ENOENT;
	}

	// verificar si el bloque representa a un archivo
	if (block->type != FILE_TYPE_FILE) {
		fprintf(stderr, "[debug] Error truncate: %s\n", strerror(EISDIR));
		return -EISDIR;
	}

	// verificar si el tamaño es válido
	if (size > MAX_FILE_SIZE) {
		fprintf(stderr, "[debug] Error truncate: %s\n", strerror(EFBIG));
		return -EFBIG;
	}

	// ajustar el tamaño del archivo
	if (size < block->size) {
		// si el nuevo tamaño es menor, simplemente ajustamos el tamaño
		block->size = size;
	} else if (size > block->size) {
		// si el nuevo tamaño es mayor, rellenamos con ceros
		memset(block->content + block->size, 0, size - block->size);
		block->size = size;
	}

	// actualizar los tiempos de modificación
	block->last_modification = time(NULL);

	printf("[debug] Truncate successful for path: %s, new size: %ld\n",
	       path,
	       size);
	return 0;
}


// Change the access and modification times of a file with nanosecond resolution

// This supersedes the old utime() interface. New applications should use this.

// fi will always be NULL
// if the file is not currently open, but may also be NULL if the file is open.
// se testea con touch(no es la principal pero se puede testear)
static int
fisopfs_utimens(const char *path, const struct timespec tv[2])
{
	printf("[debug] Updating timestamps for path: %s\n", path);

	// obtener el bloque asociado al path
	block_t *target_block = get_block_pointer(path);
	if (target_block == NULL) {
		fprintf(stderr,
		        "[debug] Failed to update timestamps: %s\n",
		        strerror(ENOENT));
		return -ENOENT;  // error: archivo o directorio no encontrado
	}

	// asignar los valores de tiempo desde el argumento
	time_t access_time = tv[0].tv_sec;
	time_t modification_time = tv[1].tv_sec;

	target_block->last_access = access_time;
	target_block->last_modification = modification_time;

	printf("[debug] Timestamps updated successfully.\n");
	return 0;
}

// clean up filesystem memory
void
cleanup_filesystem(struct file_system *fs)
{
	for (int i = 0; i < NUM_BLOCKS_MAX; i++) {
		block_t *block = &fs->blocks[i];
		block->status = 0;
		memset(block->content, 0, sizeof(block->content));
		memset(block->path, 0, sizeof(block->path));
		memset(block->dir_path, 0, sizeof(block->dir_path));
		block->size = 0;
		block->last_access = 0;
		block->last_modification = 0;
		block->creation_time = 0;

		block->type = 0;
		block->mode = 0;
		block->uid = 0;
		block->gid = 0;
	}
	printf("[debug] Filesystem memory cleared.\n");
}

// Clean up filesystem
// Called on filesystem exit.
// ayuda a la persistencia del disco.
static void
fisopfs_destroy(void *private_data)
{
	printf("[debug] Destroying filesystem and persisting to disk.\n");

	FILE *file = fopen(filedisk, "w");
	if (file == NULL) {
		fprintf(stderr,
		        "[debug] Error opening file for writing: %s\n",
		        strerror(errno));
		return;
	}

	size_t write_size = fwrite(&filesystem, sizeof(filesystem), 1, file);
	if (write_size != 1) {
		fprintf(stderr,
		        "[debug] Error writing filesystem to disk: %s\n",
		        strerror(errno));
	} else {
		printf("[debug] Filesystem persisted successfully.\n");
	}

	fclose(file);
	cleanup_filesystem(&filesystem);
}
// Possibly flush cached data
static int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_flush - path: %s\n", path);

	// persistir el fs en disco
	FILE *file = fopen(filedisk, "w");
	if (file == NULL) {
		fprintf(stderr,
		        "[debug] Error opening file for writing: %s\n",
		        strerror(errno));
		return -EIO;
	}

	size_t write_size = fwrite(&filesystem, sizeof(filesystem), 1, file);
	if (write_size != 1) {
		fprintf(stderr,
		        "[debug] Error writing filesystem to disk: %s\n",
		        strerror(errno));
		fclose(file);
		return -EIO;
	}

	fclose(file);
	printf("[debug] Filesystem flushed successfully.\n");
	return 0;
}

// Open a file

// Open flags are available in fi->flags.
// se aprecia su uso en cat aunq no es estricamente necesaria
static int
fisopfs_open(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_open - path: %s\n", path);

	block_t *block = get_block_pointer(path);
	if (!block) {
		fprintf(stderr, "[debug] Error open: %s\n", strerror(ENOENT));
		return -ENOENT;  // no se encuentra el file
	}

	// verificar si es un archivo y no un directorio
	if (block->type != FILE_TYPE_FILE) {
		fprintf(stderr, "[debug] Error open: %s\n", strerror(EISDIR));
		return -EISDIR;  // Intento de abrir un directorio como archivo
	}

	// flags de escritura y lectura
	if ((fi->flags & O_RDONLY) && !(block->mode & S_IRUSR)) {
		fprintf(stderr, "[debug] Error open: %s\n", strerror(EACCES));
		return -EACCES;  // access error
	}
	if ((fi->flags & O_WRONLY) && !(block->mode & S_IWUSR)) {
		fprintf(stderr, "[debug] Error open: %s\n", strerror(EACCES));
		return -EACCES;  // access error
	}
	if ((fi->flags & O_RDWR) &&
	    !(block->mode & S_IRUSR && block->mode & S_IWUSR)) {
		fprintf(stderr, "[debug] Error open: %s\n", strerror(EACCES));
		return -EACCES;  // access error
	}

	// actualizar el tiempo de ultimo acceso
	block->last_access = time(NULL);
	printf("[debug] File %s opened successfully. Updated last_access to "
	       "%ld\n",
	       path,
	       block->last_access);

	return 0;
}
// Change the permission bits of a file
static int
fisopfs_chmod(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_chmod - path: %s, mode: %o\n", path, mode);

	// buscar el block del path
	block_t *block = get_block_pointer(path);
	if (!block) {
		fprintf(stderr, "[debug] Error chmod: %s\n", strerror(ENOENT));
		return -ENOENT;
	}

	// actualizo el mode
	block->mode = mode;
	block->last_modification = time(NULL);

	printf("[debug] Permissions updated for %s. New mode: %o\n", path, mode);
	return 0;
}


static struct fuse_operations operations = {
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.read = fisopfs_read,
	.init = fisopfs_init,
	.mkdir = fisopfs_mkdir,
	.mknod = fisopfs_mknod,
	.create = fisopfs_create,
	.utimens = fisopfs_utimens,
	.write = fisopfs_write,
	.truncate = fisopfs_truncate,
	.rmdir = fisopfs_rmdir,
	.unlink = fisopfs_unlink,
	.destroy = fisopfs_destroy,
	.flush = fisopfs_flush,
	.open = fisopfs_open,
	.chmod = fisopfs_chmod,
};

int
main(int argc, char *argv[])
{
	for (int i = 1; i < argc - 1; i++) {
		if (strcmp(argv[i], "--filedisk") == 0) {
			filedisk = argv[i + 1];

			// We remove the argument so that fuse doesn't use our
			// argument or name as folder.
			// Equivalent to a pop.
			for (int j = i; j < argc - 1; j++) {
				argv[j] = argv[j + 2];
			}

			argc = argc - 2;
			break;
		}
	}

	return fuse_main(argc, argv, &operations, NULL);
}