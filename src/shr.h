/**
 * MIT/X Consortium License
 * 
 * Copyright © 2015  Mattias Andrée <maandree@member.fsf.org>
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


#include <stddef.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>



/**
 * An upper bound of the maximum a `shr_key_t`
 * represented as a string
 */
#define SHR_KEY_STR_MAX  (3 * sizeof(struct shr_key) + 4)

/**
 * Create key that is recogined by `shr_open` as
 * an instruction to create a private shared ring buffer
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
#define SHR_BUFFER_SIZE(SHR)  ((KEY)->key.buffer_size)

/**
 * Get the buffer count of a shared ring buffer
 * 
 * @param   shr:struct shr *  The shared ring buffer
 * @return  :size_t           The buffer count
 */
#define SHR_BUFFER_COUNT(SHR)  ((KEY)->key.buffer_count)



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
	READ = SHM_RDONLY,

	/**
	 * The shared ring buffer should be or
	 * is opened for data writing
	 */
	WRITE = 0,

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
	int direction;

	/**
	 * The index of the current buffer
	 */
	size_t current_buffer;

	/**
	 * The address of the shared memory
	 */
	void *address;

} shr_t;



int __attribute__((nonnull))
shr_create(shr_key_t *restrict key, size_t buffer_size, size_t buffer_count, mode_t mode);

void __attribute__((nonnull))
shr_remove(const shr_t *restrict shr);

void __attribute__((nonnull))
shr_remove_by_key(const shr_key_t *restrict key);


int __attribute__((nonnull))
shr_open(shr_t *restrict shr, const shr_key_t *restrict key, shr_direction_t direction);

int __attribute__((nonnull))
shr_reverse_duplicate(const shr_t *restrict old, shr_t *restrict new);

int __attribute__((nonnull))
shr_close(shr_t *restrict shr);


int __attribute__((nonnull))
shr_chown(const shr_t *restrict shr, uid_t owner, gid_t group);

int __attribute__((nonnull))
shr_chmod(const shr_t *restrict shr, mode_t mode);

int __attribute__((nonnull(1)))
shr_stat(const shr_t *restrict shr, uid_t *restrict owner, gid_t *restrict group, mode_t *restrict mode);


void __attribute__((nonnull))
shr_key_to_str(const shr_key_t *restrict key, char *restrict str);

void __attribute__((nonnull))
shr_str_to_key(const char *restrict str, shr_key_t *restrict key);


int __attribute__((nonnull))
shr_read(shr_t *restrict shr, const char **restrict buffer, size_t *restrict length);

int __attribute__((nonnull))
shr_read_try(shr_t *restrict shr, const char **restrict buffer, size_t *restrict length);

int __attribute__((nonnull))
shr_read_wait(shr_t *restrict shr);

int __attribute__((nonnull))
shr_read_timed(shr_t *restrict shr, const char **restrict buffer,
	       size_t *restrict length, const struct timespec *timeout);

int __attribute__((nonnull))
shr_read_done(shr_t *restrict shr);


int __attribute__((nonnull))
shr_write(shr_t *restrict shr, char **restrict buffer);

int __attribute__((nonnull))
shr_write_try(shr_t *restrict shr, char **restrict buffer);

int __attribute__((nonnull))
shr_write_wait(shr_t *restrict shr, char **restrict buffer);

int __attribute__((nonnull))
shr_write_timed(shr_t *restrict shr, char **restrict buffer, const struct timespec *timeout);

int __attribute__((nonnull))
shr_write_done(shr_t *restrict shr, size_t length);



#endif

