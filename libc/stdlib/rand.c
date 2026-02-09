#include <stdlib.h>

static unsigned long rand_state = 1;

void srand(unsigned int seed) {
    rand_state = seed;
}

int rand(void) {
    rand_state = rand_state * 1103515245 + 12345;
    return (int)((rand_state >> 16) & RAND_MAX);
}
