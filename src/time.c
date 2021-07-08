#include <stdint.h>
#include <err.h>
#include "time.h"

int64_t time_nanodiff(struct timespec *a, struct timespec *b)
{
	int64_t diff = a->tv_sec - b->tv_sec;
	diff *= 1000000000;
	diff += a->tv_nsec - b->tv_nsec;
	return diff;
}

struct timespec time_get_monotonic(void)
{
	struct timespec t;
	if (clock_gettime(CLOCK_MONOTONIC_COARSE, &t)) {
		// Should never fail.
		err(20,"Unable to get time");
	}
	return t;
}
