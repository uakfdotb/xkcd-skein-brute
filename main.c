#include <stdbool.h>
#include <ctype.h>
#include <endian.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/queue.h>

#include "skein.h"
#if 1
# define SKEIN_UNROLL_1024 10
# include "skein_block.c"
#endif
#include "skein.c"

#define NTHREADS 16
#define LENGTH 120

const char *target =
"5b4da95f5fa08280fc9879df44f418c8f9f12ba424b7757de02bbdfbae0d4c4fdf9317c80cc5fe04c6429073466cf29706b8c25999ddd2f6540d4475cc977b87f4757be023f19b8f4035d7722886b78869826de916a79cf9c94cc79cd4347d24b567aa3e2390a573a373a48a5e676640c79cc70197e1c5e7f902fb53ca1858b6";
uint8_t target_bytes[1024/8];

uint8_t
nibble(char c)
{

	c = tolower(c);
	if (c >= 'a' && c <= 'f')
		return c - 'a';
	else
		return c - '0';

}

void
read_hex(const char *hs, uint8_t *out)
{
	size_t slen = strlen(hs);

	for (unsigned i = 0; i < slen; i += 2) {
		uint32_t x;
		sscanf(hs, "%08x", &x);

		*(uint32_t *)out = htobe32(x);


		out += sizeof(uint32_t);
		hs += 2*sizeof(uint32_t);
	}
}

inline void
plock(pthread_mutex_t *l)
{
	pthread_mutex_lock(l);
}

inline void
punlock(pthread_mutex_t *l)
{
	pthread_mutex_unlock(l);
}

inline void
condwait(pthread_cond_t *c, pthread_mutex_t *l)
{
	pthread_cond_wait(c, l);
}

inline void *
xmalloc(size_t z)
{
	void *r;
	r = malloc(z);
	return r;
}

inline char *
xstrdup(const char *s)
{
	char *r;
	r = strdup(s);
	return r;
}

inline void
wakeup(pthread_cond_t  *c)
{
	pthread_cond_broadcast(c);
}

struct prefix_work {
	STAILQ_ENTRY(prefix_work) entry;
	char *prefix;
};

inline bool
ascii_incr_char(char *c)
{
	if (*c != 'z') {
		if (*c != 'Z') {
			if (*c != '9')
				*c += 1;
			else
				*c = 'A';
		} else
			*c = 'a';
		return true;
	} else
		*c = '0';
	
	return false;
}

inline void
ascii_incr(char *str)
{
	char *eos = str + LENGTH - 1;

	while (eos >= str) {
		if(ascii_incr_char(eos))
			return;
		eos--;
	}
}

inline unsigned
xor_dist(uint8_t *a8, uint8_t *b8, size_t len)
{
	unsigned tot = 0;
	uint64_t *a = (void*)a8,
		 *b = (void*)b8;

	while (len > 0) {
		tot += __builtin_popcountll(*a ^ *b);
		a++;
		b++;
		len -= sizeof(*a);
	}

	return tot;
}

inline unsigned
hash_dist(const char *trial, size_t len, uint8_t *hash, uint8_t trhash[1024/8])
{
	Skein1024_Ctxt_t c;

	Skein1024_Init(&c, 1024);
	Skein1024_Update(&c, (void*)trial, len);
	Skein1024_Final(&c, trhash);
	return xor_dist(trhash, hash, 1024/8);
}

unsigned rbestdist = 2000;
char beststring[128] = { 0 };
pthread_mutex_t rlock = PTHREAD_MUTEX_INITIALIZER;

void *
make_hash_sexy_time(void *v)
{
	char string[256] = { 0 };
	uint8_t loc_target_hash[1024/8];
	uint8_t trhash[1024/8];
	unsigned last_best = 4000;
	
	plock(&rlock);
	for(int i = 0; i < LENGTH; i++) {
		string[i] = rand() % 24 + ((rand() % 2) ? 65 : 97);
	}
	punlock(&rlock);

	memcpy(loc_target_hash, target_bytes, sizeof(target_bytes));

	while (true) {
		unsigned hdist = hash_dist(string, LENGTH, loc_target_hash, trhash);

		if (hdist < last_best) {
			bool improved = false;

			plock(&rlock);
			if (hdist < rbestdist) {
				rbestdist = hdist;
				memcpy(beststring, string, sizeof beststring);
				improved = true;
			}
			last_best = rbestdist;
			punlock(&rlock);

			if (improved) {
				printf("Found '%s' with distance %u\n", string,
				    hdist);
				fflush(stdout);
			}
		}

		ascii_incr(string);
	}
}

int
main(void)
{
	pthread_attr_t pdetached;
	pthread_t thr;
	
	//seed the RNG
	FILE *input = fopen("/dev/urandom", "rb");
	unsigned int seed;
	fread(&seed, sizeof(seed), 1, input);
	fclose(input);
	
	srand(seed);

	read_hex(target, target_bytes);

	pthread_attr_init(&pdetached);
	pthread_attr_setdetachstate(&pdetached, PTHREAD_CREATE_DETACHED);

	for (unsigned i = 0; i < NTHREADS; i++) {
		pthread_create(&thr, &pdetached, make_hash_sexy_time,
		    NULL);
	}

	while (true)
		sleep(100000);

	return 0;
}
