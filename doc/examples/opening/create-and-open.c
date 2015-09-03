#include <shr.h>
#include <stdio.h>


#define t(c)  if (called = #c, (c) < 0)  goto fail
static const char* called = NULL;


int main(void)
{
	shr_key_t key;
	shr_t shr;

	t (shr_create(&key, 1024, 3, 0600));
	t (shr_open(&shr, &key, SHR_WRITE));

	shr_close(&shr); /* Not really needed, shr_remove will
	                    do this, but it is still good practice
	                    to be explicit. shr_remove_by_key
	                    however would not. */
	shr_remove(&shr); /* shr_remove_by_key(&key) also works. */

	return 0;
 fail:
	perror(called);
	shr_remove_by_key(&key);
	return 1;
}

