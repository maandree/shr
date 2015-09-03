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
	char str[SHR_KEY_STR_MAX];
	char* buf;
	ssize_t got = 1;

	if (argc != 1) {
		fprintf(stderr, "See README for usage.\n");
		return 1;
	}

	t (shr_create(&key, BUFFER_SIZE, 3, 0600));
	t (shr_open(&shr, &key, SHR_WRITE));

	shr_key_to_str(&key, str);
	t (printf("key: %s\n", str));

	while (got) {
		t (shr_write(&shr, &buf));
		t (got = read(STDIN_FILENO, buf, BUFFER_SIZE));
		t (shr_write_done(&shr, (size_t)got));
	}

	shr_close(&shr);
	return 0;
 fail:
	perror(called);
	shr_remove_by_key(&key);
	return 1;
	(void) argv;
}

