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
	shr_t revshr;
	ssize_t got = 1, wrote;
	pid_t pid;
	int closed = 0;
	size_t ptr, len;

	if (argc != 1) {
		fprintf(stderr, "See README for usage.\n");
		return 1;
	}

	SHR_PRIVATE(&key, BUFFER_SIZE, 3);
	t (shr_open(&shr, &key, SHR_WRITE));

	t (shr_reverse_dup(&shr, &revshr));
	t (pid = fork());

	if (pid)
		shr = revshr;

	while (!pid && got) {
		char* buf;
		t (shr_write(&shr, &buf));
		t (got = read(STDIN_FILENO, buf, BUFFER_SIZE));
		t (shr_write_done(&shr, (size_t)got));
	}

	while (pid && !closed) {
		const char* buf;
		t (shr_read(&shr, &buf, &len));
		for (ptr = 0; ptr < len;) {
			t (wrote = write(STDOUT_FILENO, buf, len));
			ptr += (size_t)wrote;
		}
		t (closed = shr_read_done(&shr));
	}

	shr_close(&shr);
	if (pid)
		shr_remove(&shr);
	return 0;
 fail:
	perror(called);
	shr_remove_by_key(&key);
	return 1;
	(void) argv;
}

