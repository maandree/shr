#include <shr.h>
#include <stdio.h>
#include <stdlib.h>


#define t(c)  if (called = #c, (c) < 0)  goto fail
static const char* called = NULL;


int main(int argc, char *argv[])
{
	shr_key_t key;
	shr_t shr;
	size_t bufs, bufn;
	uid_t uid;
	gid_t gid;
	mode_t mode;
	char str[SHR_KEY_STR_MAX];
	char* m;

	if (argc != 6) {
		fprintf(stderr, "See README for usage.\n");
		return 1;
	}

	bufs = (size_t)atoll(argv[1]);
	bufn = (size_t)atoll(argv[2]);
	uid = (uid_t)atoll(argv[3]);
	gid = (gid_t)atoll(argv[4]);
	mode = 0, m = argv[5];
	while (*m)
		mode = (mode << 3) | ((*m++) - '0');

	t (shr_create(&key, bufs, bufn, 0600));
	t (shr_open(&shr, &key, SHR_WRITE));
	t (shr_chmod(&shr, mode));
	t (shr_chown(&shr, uid, gid));

	shr_key_to_str(&key, str);
	t (printf("key: %s\n", str));

	return 0;
 fail:
	perror(called);
	shr_remove_by_key(&key);
	return 1;
}

