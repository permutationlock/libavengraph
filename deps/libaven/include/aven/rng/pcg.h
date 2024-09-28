/*
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *       http://www.pcg-random.org
 */

/*
 * This code is derived from the full C implementation, which is in turn
 * derived from the canonical C++ PCG implementation. The C++ version
 * has many additional features and is preferable if you can use C++ in
 * your project.
 */

#ifndef AVEN_RNG_PCG_H
#define AVEN_RNG_PCG_H

#include "../../aven.h"
#include "../rng.h"

typedef struct {
    uint64_t state;
    uint64_t inc;
} AvenRngPcg;

static inline uint32_t aven_rng_pcg_rand(AvenRngPcg *rng) {
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = (uint32_t)((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = (uint32_t)(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

static inline void aven_rng_pcg_seed(
    AvenRngPcg *rng,
    uint64_t initstate,
    uint64_t initseq
) {
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    aven_rng_pcg_rand(rng);
    rng->state += initstate;
    aven_rng_pcg_rand(rng);
}

static inline AvenRng aven_rng_pcg(AvenRngPcg *rng) {
    return (AvenRng){
        .rand = (aven_rng_rand_fn)aven_rng_pcg_rand,
        .state = (void *)rng
    };
}

#endif // PCG_RNG_H
