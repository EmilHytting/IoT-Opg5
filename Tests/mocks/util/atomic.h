#pragma once
/* Single-threaded tests: hardware interrupt scheduling is not simulated. */
#define ATOMIC_RESTORESTATE 0
#define ATOMIC_BLOCK(mode) for (int once = 1; once; once = 0)
