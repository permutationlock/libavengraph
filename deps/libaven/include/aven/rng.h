#ifndef AVEN_RNG_H
#define AVEN_RNG_H

typedef uint32_t (*aven_rng_rand_fn)(void *state);

typedef struct {
    aven_rng_rand_fn rand;
    void *state;
} AvenRng;

static uint32_t aven_rng_rand(AvenRng rng) {
    return rng.rand(rng.state);
}

static uint32_t aven_rng_rand_bounded(AvenRng rng, uint32_t bound) {
    uint32_t threshold = -bound % bound;

    uint32_t n;
    do {
        n = aven_rng_rand(rng);
    } while (n < threshold);

    return n % bound;
}

#endif // RNG_H

