#include <shr.h>
#include <stdio.h>
#include <unistd.h>


#define t(c)  if (called = #c, (c) < 0)  goto fail
static const char* called = NULL;


#define BUFFER_SIZE  16


int main(int argc, char *argv[])
{
	shr_key_t key;
	shr_t shr;
	const char* buf;
	size_t len, ptr;
	ssize_t wrote;
	int closed = 0;

	if (argc != 2) {
		fprintf(stderr, "See README for usage.\n");
		return 1;
	}

	shr_str_to_key(argv[1], &key);
	t (shr_open(&shr, &key, SHR_READ));

	while (!closed) {
		t (shr_read(&shr, &buf, &len));
		for (ptr = 0; ptr < len;) {
			t (wrote = write(STDOUT_FILENO, buf, len));
			ptr += (size_t)wrote;
		}
		t (closed = shr_read_done(&shr));
	}

	shr_close(&shr);
	shr_remove(&shr);
	return 0;
 fail:
	perror(called);
	shr_remove_by_key(&key);
	return 1;
}

