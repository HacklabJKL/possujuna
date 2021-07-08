#include <time.h>

// Calculate difference of two timespecs (a minus b).
int64_t time_nanodiff(struct timespec *a, struct timespec *b);

// Get coarse monotonic time.
struct timespec time_get_monotonic(void);
