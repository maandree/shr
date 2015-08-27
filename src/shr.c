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
#include "shr.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/stat.h>



/**
 * Get the index of the semaphore for flagging
 * a buffer as being writeable
 * 
 * @param   i  The index of the buffer
 * @return     The index of the semaphore
 */
#define WRITE_SEM(i)  (2 * i + 0)

/**
 * Get the index of the semaphore for flagging
 * a buffer as being readable
 * 
 * @param   i  The index of the buffer
 * @return     The index of the semaphore
 */
#define READ_SEM(i)   (2 * i + 1)



/**
 * Create a shared ring buffer
 * 
 * The shared ring buffer will be owned by the calling
 * process's effective user and effective group
 * 
 * Undefined behaviour will be invoked if
 * `sizeof(char) + buffer_count * (buffer_size + sizeof(size_t)) > SIZE_MAX`,
 * if `buffer_count == 0` or if `(permissions & ~(S_IRWXU | S_IRWXG | S_IRWXO))`
 * 
 * @param   key           Output parameter for the key, must not be `NULL`
 * @param   buffer_size   The size of each buffer, in bytes
 * @param   buffer_count  The number of buffers, most be positive, 3 is recommended
 * @param   permissions   The permissions of the shared ring buffer,
 *                        any access for a user means full access
 * @return                Zero on success, -1 on error; on error,
 *                        `errno` will be set to describe the error
 */
int __attribute__((nonnull))
shr_create(shr_key_t *restrict key, size_t buffer_size, size_t buffer_count, mode_t permissions)
{
	size_t ring_size = sizeof(char) + buffer_count * (buffer_size + sizeof(size_t));
	size_t sem_count = 2 * buffer_count;
	void *address = NULL;
	unsigned short *values = NULL;
	int shm_id, sem_id;
	int rint;
	double r;
	size_t i;
	int saved_errno;

	key->buffer_size  = buffer_size;
	key->buffer_count = buffer_count;

	permissions |= (permissions & S_IRWXU) ? S_IRWXU : 0;
	permissions |= (permissions & S_IRWXG) ? S_IRWXG : 0;
	permissions |= (permissions & S_IRWXO) ? S_IRWXO : 0;
	permissions &= ~(S_IXUSR | S_IXGRP | S_IXOTH);

	/* Create shared memory. */
	for (;;) {
		rint = rand();
		r = (double)rint;
		r /= (double)RAND_MAX + 1;
		r *= (1 << (8 * sizeof(key_t) - 2)) - 1;

		key->shm = (key_t)r + 1;
		if (key->shm == IPC_PRIVATE)
			continue;
      
		shm_id = shmget(key->shm, ring_size, IPC_CREAT | IPC_EXCL | permissions);
		if (shm_id != -1)
			break;
      
		if ((errno != EEXIST) && (errno != EINTR)) {
			key->shm = IPC_PRIVATE;
			goto fail;
		}
	  }

	/* Get shared memory. */
	address = shmat(shm_id, NULL, 0);
	if (!address || (address == (void*)-1)) {
		address = NULL;
		goto fail;
	}

	/* Initialise shared memory. */
	*(char*)address = 0;

	/* Create semaphore array. */
	for (;;) {
		rint = rand();
		r = (double)rint;
		r /= (double)RAND_MAX + 1;
		r *= (1 << (8 * sizeof(key_t) - 2)) - 1;

		key->sem = (key_t)r + 1;
		if (key->sem == IPC_PRIVATE)
			continue;

		sem_id = semget(key->sem, sem_count, IPC_CREAT | IPC_EXCL | permissions);
		if (sem_id != -1)
			break;

		if ((errno != EEXIST) && (errno != EINTR)) {
			key->sem = IPC_PRIVATE;
			goto fail;
		}
	}

	/* Initialise semaphore array. */
	values = malloc(sem_count * sizeof(unsigned short));
	if (!values)
		goto fail;
	for (i = 0; i < buffer_count; i++) {
		values[WRITE_SEM(i)] = 1;
		values[READ_SEM(i)]  = 0;
	}
	if (semctl(sem_id, 0, SETALL, values) == -1)
		goto fail;

	free(values);
	shmdt(address);
	return 0;

 fail:
	saved_errno = errno;
	if (!values)   free(values);
	if (!address)  shmdt(address);
	shr_remove_by_key(key);
	key->shm = key->sem = IPC_PRIVATE;
	return errno = saved_errno, -1;
}


/**
 * Remove a shared ring buffer
 * 
 * Undefined behaviour is invoked if called twice
 * 
 * @param  shr  The shared ring buffer, nothing will happen if this is `NULL`
 */
void
shr_remove(const shr_t *restrict shr)
{
	struct shmid_ds _info;
	shr_t shr_ = *shr;
	if (!shr)
		return;
	shr_close(&shr_);
	if (shr->shm != -1)  shmctl(shr->shm, IPC_RMID, &_info);
	if (shr->sem != -1)  semctl(shr->sem, 0, IPC_RMID);
}


/**
 * Remove a shared ring buffer, without the need of opening it
 * 
 * Undefined behaviour is invoked if called twice
 * 
 * @param  key  The key for the shared ring buffer, must not be `NULL`
 */
void __attribute__((nonnull))
shr_remove_by_key(const shr_key_t *restrict key)
{
	struct shmid_ds _info;
	int shm_id = key->shm == IPC_PRIVATE ? -1 : shmget(key->shm, 0, 0);
	int sem_id = key->sem == IPC_PRIVATE ? -1 : semget(key->sem, 0, 0);
	if (shm_id != -1)  shmctl(shm_id, IPC_RMID, &_info);
	if (sem_id != -1)  semctl(sem_id, 0, IPC_RMID);
}



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
 */
int __attribute__((nonnull))
shr_open(shr_t *restrict shr, const shr_key_t *restrict key, shr_direction_t direction)
{
	size_t ring_size = sizeof(char) + key->buffer_count * (key->buffer_size + sizeof(size_t));
	size_t sem_count = 2 * key->buffer_count;
	size_t permissions = IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR;
	int saved_errno;

	shr->shm = shr->sem = -1;
	shr->key = *key;
	shr->direction = direction;
	shr->current_buffer = 0;
	shr->address = NULL;

	if (key->shm != IPC_PRIVATE)
		permissions = 0;

	/* Get shared memory. */
 retry_shm:
	shr->shm = shmget(key->shm, ring_size, permissions);
	if (shr->shm == -1) {
		if (errno == EINTR)
			goto retry_shm;
		goto fail;
	}
 retry_mem:
	shr->address = shmat(shr->shm, NULL, direction);
	if (!(shr->address) || (shr->address == (void*)-1)) {
		if (errno == EINTR)
			goto retry_mem;
		shr->address = NULL;
		goto fail;
	}

	/* Get semaphore array. */
 retry_sem:
	shr->sem = semget(shr->sem, sem_count, permissions);
	if (shr->sem == -1) {
		if (errno == EINTR)
			goto retry_sem;
		goto fail;
	}

	return 0;

 fail:
	saved_errno = errno;
	shr_close(shr);
	return errno = saved_errno, -1;
}


/**
 * Duplicate a sharing ring buffer but reverse the direction,
 * so that you get an instance for writting if you already
 * have one for reading, or vise versa
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
 */
int __attribute__((nonnull))
shr_reverse_dup(const shr_t *restrict old, shr_t *restrict new)
{
	*new = *old;
	new->direction ^= SHM_RDONLY;
 retry_mem:
	new->address = shmat(new->shm, NULL, new->direction);
	if (!(new->address) || (new->address == (void*)-1)) {
		if (errno == EINTR)
			goto retry_mem;
		goto fail;
	}
	return 0;

 fail:
	new->address = NULL;
	new->shm = -1;
	new->sem = -1;
	new->key.shm = IPC_PRIVATE;
	new->key.sem = IPC_PRIVATE;
	return -1;
}


/**
 * Close a shared ring buffer
 * 
 * @param  shr  The shared ring buffer, nothing will happen if
 *              this is `NULL`, or if it has already been closed
 */
void
shr_close(shr_t *restrict shr)
{
	if (shr->address)
		shmdt(shr->address), shr->address = NULL;
}



/**
 * Change the ownership of a shared ring buffer
 * 
 * @param   shr    The shared ring buffer, must not be `NULL`
 * @param   owner  The new owner of the shared ring buffer
 * @param   group  The new group of the shared ring buffer
 * @return         Zero on success, -1 on error; on error,
 *                 `errno` will be set to describe the error
 */
int __attribute__((nonnull))
shr_chown(const shr_t *restrict shr, uid_t owner, gid_t group)
{
	struct shmid_ds shm_stat;
	struct semid_ds sem_stat;

	if (shmctl(shr->shm,    IPC_STAT, &shm_stat) == -1)  return -1;
	if (semctl(shr->sem, 0, IPC_STAT, &sem_stat) == -1)  return -1;

	shm_stat.shm_perm.uid = sem_stat.sem_perm.uid = owner;
	shm_stat.shm_perm.gid = sem_stat.sem_perm.gid = group;

	if (shmctl(shr->shm,    IPC_SET, &shm_stat) == -1)  return -1;
	if (semctl(shr->sem, 0, IPC_SET, &sem_stat) == -1)  return -1;

	return 0;
}


/**
 * Change the permissions of a shared ring buffer
 * 
 * @param   shr          The shared ring buffer, must not be `NULL`
 * @param   permissions  The new permissions of the shared ring buffer,
 *                       any access for a user means full access
 * @return               Zero on success, -1 on error; on error,
 *                       `errno` will be set to describe the error
 */
int __attribute__((nonnull))
shr_chmod(const shr_t *restrict shr, mode_t permissions)
{
	struct shmid_ds shm_stat;
	struct semid_ds sem_stat;

	permissions |= (permissions & S_IRWXU) ? S_IRWXU : 0;
	permissions |= (permissions & S_IRWXG) ? S_IRWXG : 0;
	permissions |= (permissions & S_IRWXO) ? S_IRWXO : 0;
	permissions &= ~(S_IXUSR | S_IXGRP | S_IXOTH);

	if (shmctl(shr->shm,    IPC_STAT, &shm_stat) == -1)  return -1;
	if (semctl(shr->sem, 0, IPC_STAT, &sem_stat) == -1)  return -1;

	shm_stat.shm_perm.mode = sem_stat.sem_perm.mode = permissions;

	if (shmctl(shr->shm,    IPC_SET, &shm_stat) == -1)  return -1;
	if (semctl(shr->sem, 0, IPC_SET, &sem_stat) == -1)  return -1;

	return 0;
}


/**
 * Get the ownership and permisions of a shared ring buffer
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
 */
int __attribute__((nonnull(1)))
shr_stat(const shr_t *restrict shr, uid_t *restrict owner, gid_t *restrict group, mode_t *restrict permissions)
{
	struct shmid_ds shm_stat;

	if (shmctl(shr->shm, IPC_STAT, &shm_stat) == -1)
		return -1;

	if (owner)        *owner       = shm_stat.shm_perm.uid;
	if (group)        *group       = shm_stat.shm_perm.gid;
	if (permissions)  *permissions = shm_stat.shm_perm.mode;

	return 0;
}



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
void __attribute__((nonnull))
shr_key_to_str(const shr_key_t *restrict key, char *restrict str)
{
	sprintf(str, "%zu.%zu.%zu.%zu",
	        (size_t)(key->shm), (size_t)(key->sem),
	        key->buffer_size, key->buffer_count);
}


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
void __attribute__((nonnull))
shr_str_to_key(const char *restrict str, shr_key_t *restrict key)
{
	char c;
	memset(key, 0, sizeof(*key));
	while ('.' != (c = *str++))  key->shm *= 10,          key->shm += c & 15;
	while ('.' != (c = *str++))  key->sem *= 10,          key->sem += c & 15;
	while ('.' != (c = *str++))  key->buffer_size *= 10,  key->buffer_size += c & 15;
	while ((c = *str++))         key->buffer_count *= 10, key->buffer_count += c & 15;
}



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
 */
int __attribute__((nonnull))
shr_read(shr_t *restrict shr, const char **restrict buffer, size_t *restrict length)
{
	return (void)shr, (void)buffer, (void)length, 0;
}


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
 */
int __attribute__((nonnull))
shr_read_try(shr_t *restrict shr, const char **restrict buffer, size_t *restrict length)
{
	return (void)shr, (void)buffer, (void)length, 0;
}


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
 */
int __attribute__((nonnull))
shr_read_timed(shr_t *restrict shr, const char **restrict buffer,
	       size_t *restrict length, const struct timespec *timeout)
{
	return (void)shr, (void)buffer, (void)length, (void)timeout, 0;
}


/**
 * Wait for a shared ring buffer to be filled with readable data,
 * but do not flag it as being currently read
 * 
 * @param   shr  The shared ring buffer, must not be `NULL`
 * @return       Zero on success, -1 on error; on error,
 *               `errno` will be set to describe the error
 */
int __attribute__((nonnull))
shr_read_wait(shr_t *restrict shr)
{
	return (void)shr, 0;
}


/**
 * Wait, for a limited time, for a shared ring buffer to be filled
 * with readable data, but do not flag it as being currently read
 * 
 * @param   shr      The shared ring buffer, must not be `NULL`
 * @param   timeout  The time limit, this should be a relative time, `NULL`
 *                   if the function shall fail immediately if it is not ready
 * @return           Zero on success, -1 on error; on error,
 *                   `errno` will be set to describe the error
 */
int __attribute__((nonnull(1)))
shr_read_wait_timed(shr_t *restrict shr, const struct timespec *timeout)
{
	return (void)shr, (void)timeout, 0;
}


/**
 * Mark the, by `shr_read`, `shr_read_try` or `shr_read_timed`,
 * retrieve buffer as fully read
 * 
 * Undefined behaviour is invoked if multiple processes use this
 * function, even if not concurrently
 * 
 * @param   shr  The shared ring buffer, must not be `NULL`
 * @return       Zero on success, -1 on error; on error,
 *               `errno` will be set to describe the error
 */
int __attribute__((nonnull))
shr_read_done(shr_t *restrict shr)
{
	return (void)shr, 0;
}



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
 */
int __attribute__((nonnull))
shr_write(shr_t *restrict shr, char **restrict buffer)
{
	return (void)shr, (void)buffer, 0;
}


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
 */
int __attribute__((nonnull))
shr_write_try(shr_t *restrict shr, char **restrict buffer)
{
	return (void)shr, (void)buffer, 0;
}


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
 */
int __attribute__((nonnull))
shr_write_timed(shr_t *restrict shr, char **restrict buffer, const struct timespec *timeout)
{
	return (void)shr, (void)buffer, (void)timeout, 0;
}


/**
 * Wait for a shared ring buffer to be get a buffer ready for
 * writting, but do not flag it as being currently written
 * 
 * @param   shr  The shared ring buffer, must not be `NULL`
 * @return       Zero on success, -1 on error; on error,
 *               `errno` will be set to describe the error
 */
int __attribute__((nonnull))
shr_write_wait(shr_t *restrict shr)
{
	return (void)shr, 0;
}


/**
 * Wait, for a limited time, for a shared ring buffer to be get a buffer
 * ready for writting, but do not flag it as being currently written
 * 
 * @param   shr      The shared ring buffer, must not be `NULL`
 * @param   timeout  The time limit, this should be a relative time, `NULL`
 *                   if the function shall fail immediately if it is not ready
 * @return           Zero on success, -1 on error; on error,
 *                   `errno` will be set to describe the error
 */
int __attribute__((nonnull(1)))
shr_write_wait_timed(shr_t *restrict shr, const struct timespec *timeout)
{
	return (void)shr, (void)timeout, 0;
}

/**
 * Mark the, by `shr_write`, `shr_write_try` or `shr_write_timed`,
 * retrieve buffer as written and ready to be read
 * 
 * Undefined behaviour is invoked if multiple processes use this
 * function, even if not concurrently
 * 
 * @param   shr     The shared ring buffer, must not be `NULL`
 * @param   length  The number of written bytes
 * @return          Zero on success, -1 on error; on error,
 *                  `errno` will be set to describe the error
 */
int __attribute__((nonnull))
shr_write_done(shr_t *restrict shr, size_t length)
{
	return (void)shr, (void)length, 0;
}

