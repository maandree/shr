/**
 * MIT/X Consortium License
 * 
 * Copyright © 2015  Mattias Andrée <m@maandree.se>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef SHR_H
#define SHR_H


#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif
#include <stddef.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>



#if defined(__GNUC__)
# define SHR_COMPILER_GCC(X)  X
#else
# define SHR_COMPILER_GCC(X)  /* ignore */
#endif



/**
 * An upper bound of the maximum a `shr_key_t`
 * represented as a string
 */
#define SHR_KEY_STR_MAX  (3 * sizeof(struct shr_key) + 4)

/**
 * Create key that is recogined by `shr_open` as
 * an instruction to create a private shared ring buffer
 * 
 * Undefined behaviour will be invoked if
 * `sizeof(size_t) + BUFFER_COUNT * (BUFFER_SIZE + sizeof(size_t)) > SIZE_MAX`,
 * or if `BUFFER_COUNT > SHORT_MAX`
 * 
 * @param  KEY:struct shr_key *  Output parameter for the psuedo-key
 * @param  BUFFER_SIZE:size_t    The size of each buffer
 * @param  BUFFER_COUNT:size_t   The number of buffers
 */
#define SHR_PRIVATE(KEY, BUFFER_SIZE, BUFFER_COUNT)  \
	((KEY)->shm = (KEY)->sem = IPC_PRIVATE,	     \
	 (KEY)->buffer_size = BUFFER_SIZE,	     \
	 (KEY)->buffer_count = BUFFER_COUNT)

/**
 * Get the buffer size of a shared ring buffer
 * 
 * @param   shr:struct shr *  The shared ring buffer
 * @return  :size_t           The buffer size
 */
#define SHR_BUFFER_SIZE(SHR)  ((SHR)->key.buffer_size)

/**
 * Get the buffer count of a shared ring buffer
 * 
 * @param   shr:struct shr *  The shared ring buffer
 * @return  :size_t           The buffer count
 */
#define SHR_BUFFER_COUNT(SHR)  ((SHR)->key.buffer_count)



/**
 * Should the shared ring buffer be opened for
 * reading or writing
 */
typedef enum shr_direction
{
	/**
	 * The shared ring buffer should be or
	 * is opened for data reading
	 */
	SHR_READ = SHM_RDONLY,

	/**
	 * The shared ring buffer should be or
	 * is opened for data writing
	 */
	SHR_WRITE = 0,

} shr_direction_t;


/**
 * Structure hold keys for the primitives
 * the shared ring buffer uses, it also
 * holds configurations
 */
typedef struct shr_key
{
	/**
	 * The key of the shared memory
	 */
	key_t shm;

	/**
	 * The key of the semaphore array
	 */
	key_t sem;

	/**
	 * The size of each buffer
	 */
	size_t buffer_size;

	/**
	 * The number of buffers
	 */
	size_t buffer_count;

} shr_key_t;


/**
 * Structure hold are information required
 * to describe a shared ring buffer and its
 * state
 */
typedef struct shr
{
	/**
	 * The ID of the shared memory
	 */
	int shm;

	/**
	 * The ID of the semaphore array
	 */
	int sem;

	/**
	 * The key of the shared ring buffer
	 */
	shr_key_t key;

	/**
	 * Which access direction the shared
	 * ring opened
	 */
	shr_direction_t direction;

	/**
	 * The index of the current buffer
	 */
	size_t current_buffer;

	/**
	 * The address of the shared memory
	 */
	char *address;

} shr_t;



/**
 * Create a shared ring buffer
 * 
 * The shared ring buffer will be owned by the calling
 * process's effective user and effective group
 * 
 * Undefined behaviour will be invoked if
 * `sizeof(size_t) + buffer_count * (buffer_size + sizeof(size_t)) > SIZE_MAX`,
 * if `buffer_count == 0` or if `(permissions & ~(S_IRWXU | S_IRWXG | S_IRWXO))`,
 * or if `buffer_count > SHORT_MAX`
 * 
 * @param   key           Output parameter for the key, must not be `NULL`
 * @param   buffer_size   The size of each buffer, in bytes
 * @param   buffer_count  The number of buffers, most be positive, 3 is recommended
 * @param   permissions   The permissions of the shared ring buffer,
 *                        any access for a user means full access
 * @return                Zero on success, -1 on error; on error,
 *                        `errno` will be set to describe the error
 * 
 * @throws  The errors EINVAL, ENOMEM, ENOSPC, as specified for shmget(3) and semget(3)
 * @throws  Any error specified for shmat(3), semctl(3) and malloc(3)
 */
int shr_create(shr_key_t *restrict, size_t, size_t, mode_t)
	SHR_COMPILER_GCC(__attribute__((nonnull, warn_unused_result)));

/**
 * Remove a shared ring buffer
 * 
 * Undefined behaviour is invoked if called twice
 * 
 * @param  shr  The shared ring buffer, nothing will happen if this is `NULL`
 */
void shr_remove(const shr_t *restrict);

/**
 * Remove a shared ring buffer, without the need of opening it
 * 
 * Undefined behaviour is invoked if called twice
 * 
 * @param  key  The key for the shared ring buffer, must not be `NULL`
 */
void shr_remove_by_key(const shr_key_t *restrict)
	SHR_COMPILER_GCC(__attribute__((nonnull)));


/**
 * Open a shared ring buffer
 * 
 * The shared ring buffer will be owned by the calling
 * process's effective user and effective group, if
 * created by this call, only the owner will have access
 * 
 * The behaviour is unspecified if a shared ring buffer
 * is opened for the same access direction more than once
 * 
 * @param   shr        Output parameter for the shared ring buffer, must not be `NULL`
 * @param   key        The key for the shared ring buffer, use `SHR_PRIVATE` on the
 *                     key before passing it to this function, to create and open a
 *                     private shared ring buffer
 * @param   direction  Whether the shared ring buffer should be opened
 *                     for reading or writting, only one is allowed
 * @return             Zero on success, -1 on error; on error,
 *                     `errno` will be set to describe the error
 * 
 * @throws  Any error specified for shmget(3), shmat(3) and semget(3) except EINTR
 * @throws  Any error semctl(3) and malloc(3) if creating a private shared ring buffer
 */
int shr_open(shr_t *restrict, const shr_key_t *restrict, shr_direction_t)
	SHR_COMPILER_GCC(__attribute__((nonnull, warn_unused_result)));

/**
 * Duplicate a sharing ring buffer but reverse the direction,
 * so that you get an instance for writting if you already
 * have one for reading, or vice versa
 * 
 * This is only useful if you have used `shr_open` to
 * create a private shared ring buffer
 * 
 * This function will reopen the shared ring buffer, thus,
 * according to `shr_open`, the behaviour is unspecified if
 * this function is used more than once on a shared ring
 * buffer or if the shared ring buffer already has been
 * opened for both directions
 * 
 * @param   shr  The shared ring buffer, must not be `NULL`
 * @param   new  Output parameter for the new shared ring
 *               buffer descriptor, must not be `NULL`
 * @return       Zero on success, -1 on error; on error,
 *               `errno` will be set to describe the error
 * 
 * @throws  Any error specified for shmat(3)
 */
int shr_reverse_dup(const shr_t *restrict, shr_t *restrict)
	SHR_COMPILER_GCC(__attribute__((nonnull, warn_unused_result)));

/**
 * Close a shared ring buffer
 * 
 * @param  shr  The shared ring buffer, nothing will happen if
 *              this is `NULL`, or if it has already been closed
 */
void shr_close(shr_t *restrict);


/**
 * Change the ownership of a shared ring buffer
 * 
 * @param   shr    The shared ring buffer, must not be `NULL`
 * @param   owner  The new owner of the shared ring buffer
 * @param   group  The new group of the shared ring buffer
 * @return         Zero on success, -1 on error; on error,
 *                 `errno` will be set to describe the error
 * 
 * @throws  The errors EACCES, EPERM and EINVAL, as specified for shmctl(3) and semctl(3)
 */
int shr_chown(const shr_t *restrict, uid_t, gid_t)
	SHR_COMPILER_GCC(__attribute__((nonnull, warn_unused_result)));

/**
 * Change the permissions of a shared ring buffer
 * 
 * @param   shr          The shared ring buffer, must not be `NULL`
 * @param   permissions  The new permissions of the shared ring buffer,
 *                       any access for a user means full access
 * @return               Zero on success, -1 on error; on error,
 *                       `errno` will be set to describe the error
 * 
 * @throws  The errors EACCES, EPERM and EINVAL, as specified for shmctl(3) and semctl(3)
 */
int shr_chmod(const shr_t *restrict, mode_t)
	SHR_COMPILER_GCC(__attribute__((nonnull, warn_unused_result)));

/**
 * Get the ownership and permissions of a shared ring buffer
 * 
 * @param   shr          The shared ring buffer, must not be `NULL`
 * @param   owner        Output parameter for the owner of the shared
 *                       ring buffer, ignored if `NULL`
 * @param   group        Output parameter for the group of the shared
 *                       ring buffer, ignored if `NULL`
 * @param   permissions  Output parameter for the permissions of the
 *                       shared ring buffer, ignored if `NULL`
 * @return               Zero on success, -1 on error; on error,
 *                       `errno` will be set to describe the error
 * 
 * @throws  The errors EACCES and EINVAL, as specified for shmctl(3) and semctl(3)
 */
int shr_stat(const shr_t *restrict, uid_t *restrict, gid_t *restrict, mode_t *restrict)
	SHR_COMPILER_GCC(__attribute__((nonnull(1), warn_unused_result)));


/**
 * Convert a shared ring buffer key to a string
 * 
 * The length of `str` is not checked, it should
 * be allocated with the size `SHR_KEY_STR_MAX * sizeof(char)`,
 * if not undefined behaviour is invoked if its
 * allocation is too small
 * 
 * @param  key  The key of the shared ring buffer, must not be `NULL`
 * @param  str  Output buffer for the string representation of the key,
 *              must not be `NULL` and must have an allocation size of
 *              at least `SHR_KEY_STR_MAX * sizeof(char)`
 */
void shr_key_to_str(const shr_key_t *restrict, char *restrict)
	SHR_COMPILER_GCC(__attribute__((nonnull)));

/**
 * Convert a string to a shared ring buffer key
 * 
 * This function does not validate the value of `str`,
 * undefined behaviour may be invoked if the string
 * is not properly formatted
 * 
 * Undefined behaviour is invoked if `str` is not
 * NUL-terminated
 * 
 * @param  str  The string representation of the key of a
 *              shared ring buffer, must not be `NULL`
 * @param  key  Output parameter for the key represented by
 *              `str`, must not be `NULL`
 */
void shr_str_to_key(const char *restrict, shr_key_t *restrict)
	SHR_COMPILER_GCC(__attribute__((nonnull)));


/**
 * Wait for a shared ring buffer to be filled with readable data,
 * and flag it as being currently read
 * 
 * Undefined behaviour is invoked if multiple processes use this
 * function, even if not concurrently
 * 
 * @param   shr     The shared ring buffer, must not be `NULL`
 * @param   buffer  Output parameter for the buffer to read, must not be `NULL`
 * @param   length  Output parameter for the length of `*buffer`, must not be `NULL`
 * @return          Zero on success, -1 on error; on error,
 *                  `errno` will be set to describe the error
 * 
 * @throws  The errors EACCES, EIDRM, EINTR and EINVAL, as specified for semop(3)
 */
int shr_read(shr_t *restrict, const char **restrict, size_t *restrict)
	SHR_COMPILER_GCC(__attribute__((nonnull, warn_unused_result)));

/**
 * Flag a shared ring buffer as being currently read,
 * but fail if it has not readable data
 * 
 * Undefined behaviour is invoked if multiple processes use this
 * function, even if not concurrently
 * 
 * @param   shr     The shared ring buffer, must not be `NULL`
 * @param   buffer  Output parameter for the buffer to read, must not be `NULL`
 * @param   length  Output parameter for the length of `*buffer`, must not be `NULL`
 * @return          Zero on success, -1 on error; on error,
 *                  `errno` will be set to describe the error
 * 
 * @throws  The errors EACCES, EAGAIN, EIDRM, EINTR and EINVAL,
 *          as specified for semop(3)
 */
int shr_read_try(shr_t *restrict, const char **restrict, size_t *restrict)
	SHR_COMPILER_GCC(__attribute__((nonnull, warn_unused_result)));

/**
 * Wait, for a limited time, for a shared ring buffer to be filled
 * with readable data, and flag it as being currently read
 * 
 * Undefined behaviour is invoked if multiple processes use this
 * function, even if not concurrently
 * 
 * @param   shr      The shared ring buffer, must not be `NULL`
 * @param   buffer   Output parameter for the buffer to read, must not be `NULL`
 * @param   length   Output parameter for the length of `*buffer`, must not be `NULL`
 * @param   timeout  The time limit, this should be a relative time, must not be `NULL`
 * @return           Zero on success, -1 on error; on error,
 *                   `errno` will be set to describe the error
 * 
 * @throws  The errors EACCES, EAGAIN, EFAULT, EIDRM, EINTR and EINVAL,
 *          as specified for semtimedop(3)
 */
int shr_read_timed(shr_t *restrict, const char **restrict, size_t *restrict, const struct timespec *)
	SHR_COMPILER_GCC(__attribute__((nonnull, warn_unused_result)));

/**
 * Mark the, by `shr_read`, `shr_read_try` or `shr_read_timed`,
 * retrieve buffer as fully read
 * 
 * Undefined behaviour is invoked if multiple processes use this
 * function, even if not concurrently
 * 
 * @param   shr  The shared ring buffer, must not be `NULL`
 * @return       Zero on success, -1 on error, 1 if the write
 *               end has closed and all data has been read; on
 *               error, `errno` will be set to describe the error
 * 
 * @throws  The errors EACCES, EIDRM, EINTR and EINVAL, as specified for semop(3)
 */
int shr_read_done(shr_t *restrict)
	SHR_COMPILER_GCC(__attribute__((nonnull, warn_unused_result)));


/**
 * Wait for a shared ring buffer to be get a buffer ready for
 * writting, and flag it as being currently written
 * 
 * Undefined behaviour is invoked if multiple processes use this
 * function, even if not concurrently
 * 
 * @param   shr     The shared ring buffer, must not be `NULL`
 * @param   buffer  Output parameter for the buffer where the data
 *                  should be written, must not be `NULL` and will
 *                  have the allocation size `SHR_BUFFER_SIZE(shr)`
 * @return          Zero on success, -1 on error; on error,
 *                  `errno` will be set to describe the error
 * 
 * @throws  The errors EACCES, EIDRM, EINTR and EINVAL, as specified for semop(3)
 */
int shr_write(shr_t *restrict, char **restrict)
	SHR_COMPILER_GCC(__attribute__((nonnull, warn_unused_result)));

/**
 * Flag a shared ring buffer as being currently written to,
 * but fail if there is no buffer free for writing
 * 
 * Undefined behaviour is invoked if multiple processes use this
 * function, even if not concurrently
 * 
 * @param   shr     The shared ring buffer, must not be `NULL`
 * @param   buffer  Output parameter for the buffer where the data
 *                  should be written, must not be `NULL` and will
 *                  have the allocation size `SHR_BUFFER_SIZE(shr)`
 * @return          Zero on success, -1 on error; on error,
 *                  `errno` will be set to describe the error
 * 
 * @throws  The errors EACCES, EAGAIN, EIDRM, EINTR and EINVAL,
 *          as specified for semop(3)
 */
int shr_write_try(shr_t *restrict, char **restrict)
	SHR_COMPILER_GCC(__attribute__((nonnull, warn_unused_result)));

/**
 * Wait, for a limited time, for a shared ring buffer to be get a
 * buffer ready for writting, and flag it as being currently written
 * 
 * Undefined behaviour is invoked if multiple processes use this
 * function, even if not concurrently
 * 
 * @param   shr      The shared ring buffer, must not be `NULL`
 * @param   buffer   Output parameter for the buffer where the data
 *                   should be written, must not be `NULL` and will
 *                   have the allocation size `SHR_BUFFER_SIZE(shr)`
 * @param   timeout  The time limit, this should be a relative time,
 *                   must not be `NULL`
 * @return           Zero on success, -1 on error; on error,
 *                   `errno` will be set to describe the error
 * 
 * @throws  The errors EACCES, EAGAIN, EFAULT, EIDRM, EINTR and EINVAL,
 *          as specified for semtimedop(3)
 */
int shr_write_timed(shr_t *restrict, char **restrict, const struct timespec *)
	SHR_COMPILER_GCC(__attribute__((nonnull, warn_unused_result)));

/**
 * Mark the, by `shr_write`, `shr_write_try` or `shr_write_timed`,
 * retrieve buffer as written and ready to be read
 * 
 * Undefined behaviour is invoked if multiple processes use this
 * function, even if not concurrently
 * 
 * @param   shr     The shared ring buffer, must not be `NULL`
 * @param   length  The number of written bytes,
 *                  may not exceed `SHR_BUFFER_SIZE(shr)`
 * @return          Zero on success, -1 on error; on error,
 *                  `errno` will be set to describe the error
 * 
 * @throws  The errors EACCES, EIDRM, EINTR and EINVAL, as specified for semop(3)
 */
int shr_write_done(shr_t *restrict, size_t)
	SHR_COMPILER_GCC(__attribute__((nonnull, warn_unused_result)));



#endif

