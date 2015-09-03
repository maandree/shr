#include <shr.h>
#include <stdio.h>
#include <stdlib.h>


#define t(c)  if (called = #c, (c) < 0)  goto fail
static const char* called = NULL;


int main(int argc, char *argv[])
{
	shr_key_t key;
	shr_t shr;
	uid_t uid;
	gid_t gid;
	mode_t mode;

	if (argc != 2) {
		fprintf(stderr, "See README for usage.\n");
		return 1;
	}

	uid = (uid_t)atoll(argv[3]);
	gid = (gid_t)atoll(argv[4]);
	mode = (mode_t)atoll(argv[5]);

	shr_str_to_key(argv[1], &key);
	t (shr_open(&shr, &key, SHR_READ));
	t (shr_stat(&shr, &uid, &gid, &mode));

	printf("buffer size: %zu\n", SHR_BUFFER_SIZE(&shr));
	printf("buffer count: %zu\n", SHR_BUFFER_COUNT(&shr));
	printf("owner: %lli\n", (long long)uid);
	printf("group: %lli\n", (long long)gid);
	printf("mode: %o\n", (int)mode);

	shr_close(&shr);
	shr_remove(&shr);

	return 0;
 fail:
	perror(called);
	shr_remove(&shr);
	return 1;
}

