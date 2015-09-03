#include <shr.h>
#include <stdio.h>
#include <stdint.h>


#define t(c)  if (called = #c, (c) < 0)  goto fail
static const char* called = NULL;


int main(void)
{
	shr_key_t key;
	shr_key_t new;
	char str[SHR_KEY_STR_MAX];

	t (shr_create(&key, 1024, 3, 0600));

	printf("INTERNAL KEY-INFORMATION:\n");
	printf("sysv shm key: %ji (x%jx)\n", (intmax_t)(key.shm), (intmax_t)(key.shm));
	printf("sysv sem key: %ji (x%jx)\n", (intmax_t)(key.sem), (intmax_t)(key.sem));
	printf("buffer size: %zu (x%zx)\n", key.buffer_size, key.buffer_size);
	printf("buffer count: %zu (x%zx)\n", key.buffer_count, key.buffer_size);
	printf("\n");

	printf("KEY CONVERTION:\n");
	shr_key_to_str(&key, str);
	shr_str_to_key(str, &new);
	printf("key: %s\n", str);
	printf("sysv shm key: %ji (x%jx)\n", (intmax_t)(new.shm), (intmax_t)(new.shm));
	printf("sysv sem key: %ji (x%jx)\n", (intmax_t)(new.sem), (intmax_t)(new.sem));
	printf("buffer size: %zu (x%zx)\n", new.buffer_size, new.buffer_size);
	printf("buffer count: %zu (x%zx)\n", new.buffer_count, new.buffer_size);
	printf("\n");

	shr_remove_by_key(&key);

	return 0;
 fail:
	return perror(called), 1;
}

