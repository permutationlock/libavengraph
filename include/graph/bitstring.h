#ifndef GRAPH_BITSTRING_H
    #define GRAPH_BITSTRING_H

    #include <aven.h>
    #include <aven/arena.h>
    #include <aven/rng.h>

    typedef Slice(uint32_t) GraphBitstringWords;
    typedef struct {
        GraphBitstringWords words;
        size_t nbits;
        size_t final_word_len;
    } GraphBitstring;
    typedef struct {
        uint32_t bits;
        uint32_t nbits;
    } GraphBitstringChunk;

    static inline size_t graph_bitstring_weight(GraphBitstring bstring) {
        size_t weight = 0;
        for (size_t i = 0; i < bstring.words.len; i += 1) {
    #if __has_builtin(__builtin_popcount)
            weight += (size_t)__builtin_popcount(get(bstring.words, i));
    #else
            uint32_t word = get(bstring.words, i);
            while (word != 0) {
                weight += word & 1;
                word >> 1;
            }
    #endif
        }
        return weight;
    }

    static inline size_t graph_bitstring_word_len(
        GraphBitstring bstring,
        size_t word_idx
    ) {
        return word_idx == bstring.words.len - 1 ? bstring.final_word_len : 32;
    }

    static inline uint32_t graph_bitstring_word_mask(
        GraphBitstring bstring,
        size_t word_idx
    ) {
        if (word_idx == bstring.words.len - 1 && bstring.final_word_len < 32) {
            return (~(uint32_t)0) >> (32 - bstring.final_word_len);
        }
        return ~(uint32_t)0;
    }

    static inline uint32_t graph_bitstring_word_submask(
        GraphBitstring bstring,
        size_t word_idx,
        size_t bit_idx,
        size_t len
    ) {
        uint32_t word_mask = graph_bitstring_word_mask(bstring, word_idx);
        size_t word_len = graph_bitstring_word_len(bstring, word_idx);
        return (word_mask >> (word_len - len)) << bit_idx;
    }

    static inline bool graph_bitstring_get_bit(
        GraphBitstring bstring,
        size_t pos
    ) {
        size_t word_idx = pos / 32;
        size_t bit_idx = pos % 32;
        return get(bstring.words, word_idx) & graph_bitstring_word_submask(
            bstring,
            word_idx,
            bit_idx,
            1
        );
    }

    static GraphBitstringChunk graph_bitstring_read(
        GraphBitstring bstring,
        size_t pos,
        size_t len
    ) {
        assert(pos <= bstring.nbits);
        assert(len <= 32);
        uint32_t out_bits = 0;
        size_t write_bit_idx = 0;
        while (write_bit_idx < len) {
            size_t read_word_idx = pos / 32;
            size_t read_bit_idx = pos % 32;
            size_t word_len = graph_bitstring_word_len(bstring, read_word_idx);
            size_t read_len = word_len - read_bit_idx;
            read_len = min(read_len, len - write_bit_idx);
            uint32_t read_mask = graph_bitstring_word_submask(
                bstring,
                read_word_idx,
                read_bit_idx,
                read_len
            );
            uint32_t masked_bits = get(bstring.words, read_word_idx) & read_mask;
            uint32_t in_bits = masked_bits >> read_bit_idx;
            out_bits |= in_bits << write_bit_idx;
            write_bit_idx += read_len;
            pos = pos + read_len;
            if (pos >= bstring.nbits) {
                pos -= bstring.nbits;
            }
        }
        return (GraphBitstringChunk){ .bits = out_bits, .nbits = (uint32_t)len };
    }

    static void graph_bitstring_write(
        GraphBitstring bstring,
        size_t pos,
        GraphBitstringChunk chunk
    ) {
        while (chunk.nbits > 0) {
            size_t write_word_idx = pos / 32;
            size_t write_bit_idx = pos % 32;
            size_t word_len = graph_bitstring_word_len(bstring, write_word_idx);
            size_t write_len = word_len - write_bit_idx;
            write_len = min(write_len, chunk.nbits);
            uint32_t write_mask = graph_bitstring_word_submask(
                bstring,
                write_word_idx,
                write_bit_idx,
                write_len
            );
            uint32_t read_mask = write_mask >> write_bit_idx;
            uint32_t in_bits = chunk.bits & read_mask;
            get(bstring.words, write_word_idx) &= ~write_mask;
            get(bstring.words, write_word_idx) |= in_bits << write_bit_idx;
            chunk.nbits -= (uint32_t)write_len;
            if (chunk.nbits > 0) {
                chunk.bits >>= write_len;
            }
        }
    }

    static void graph_bitstring_rotate_left(
        GraphBitstring bstring,
        size_t nbits
    ) {
        if (nbits == 0) {
            return;
        }
        size_t bs_nbits = bstring.nbits;
        size_t bs_final_word_len = bstring.final_word_len;
        bstring.nbits += 32 - bs_final_word_len;
        bstring.final_word_len = 32;

        size_t word_rot = nbits / 32;
        size_t bit_rot = nbits % 32;

        // rotate 32 bit words
        for (size_t word_idx = 0; word_idx < word_rot / 2; word_idx += 1) {
            size_t i = word_idx;
            size_t j = word_rot - word_idx - 1;
            uint32_t tmp = get(bstring.words, i);
            get(bstring.words, i) = get(bstring.words, j);
            get(bstring.words, j) = tmp;
        }
        for (
            size_t word_idx = 0;
            word_idx < (bstring.words.len - word_rot) / 2;
            word_idx += 1
        ) {
            size_t i = word_rot + word_idx;
            size_t j = bstring.words.len - word_idx - 1;
            uint32_t tmp = get(bstring.words, i);
            get(bstring.words, i) = get(bstring.words, j);
            get(bstring.words, j) = tmp;
        }
        for (
            size_t word_idx = 0;
            word_idx < bstring.words.len / 2;
            word_idx += 1
        ) {
            size_t i = word_idx;
            size_t j = bstring.words.len - word_idx - 1;
            uint32_t tmp = get(bstring.words, i);
            get(bstring.words, i) = get(bstring.words, j);
            get(bstring.words, j) = tmp;
        }

        // rotate bits within words
        GraphBitstringChunk first_word = graph_bitstring_read(
            bstring,
            bit_rot,
            graph_bitstring_word_len(bstring, 0)
        );
        for (size_t word_idx = 1; word_idx < bstring.words.len; word_idx += 1) {
            GraphBitstringChunk new_word = graph_bitstring_read(
                bstring,
                word_idx * 32 + bit_rot,
                graph_bitstring_word_len(bstring, word_idx)
            );
            get(bstring.words, word_idx) = new_word.bits;
        }
        get(bstring.words, 0) = first_word.bits;

        // shift bits to remove gap
        size_t gap = 32 - bs_final_word_len;
        if (gap == 0) {
            return;
        }
        size_t wpos = bs_nbits - nbits;
        size_t rpos = wpos + gap;

        {
            size_t wbit_idx = wpos % 32;
            size_t wword_idx = wpos / 32;
            size_t wword_len = graph_bitstring_word_len(bstring, wword_idx);
            size_t write_len = wword_len - wbit_idx;
            GraphBitstringChunk newword = graph_bitstring_read(
                bstring,
                rpos,
                write_len
            );
            graph_bitstring_write(bstring, wpos, newword);
            wpos += write_len;
            rpos += write_len;
        }

        assert(wpos % 32 == 0);
        for (
            size_t wword_idx = wpos / 32;
            wword_idx < bstring.words.len;
            wword_idx += 1
        ) {
            GraphBitstringChunk new_word = graph_bitstring_read(
                bstring,
                rpos,
                32
            );
            get(bstring.words, wword_idx) = new_word.bits;
            rpos += 32;
        }

        bstring.nbits = bs_nbits;
        bstring.final_word_len = bs_final_word_len;
        uint32_t end_mask = graph_bitstring_word_mask(
            bstring,
            bstring.words.len - 1
        );
        get(bstring.words, bstring.words.len - 1) &= end_mask;
    }

    static void graph_bitstring_shuffle(GraphBitstring bstring, AvenRng rng) {
        assert(bstring.nbits < UINT32_MAX);
        for (size_t tpos = 0; tpos < bstring.nbits - 1; tpos += 1) {
            size_t tpos_word_idx = tpos / 32;
            size_t tpos_bit_idx = tpos % 32;

            size_t rnd_offset = aven_rng_rand_bounded(
                rng,
                (uint32_t)((bstring.nbits - 1) - tpos)
            );
            size_t spos = tpos + rnd_offset;
            size_t spos_word_idx = spos / 32;
            size_t spos_bit_idx = spos % 32;

            uint32_t ttmp = get(bstring.words, tpos_word_idx);
            uint32_t stmp = get(bstring.words, spos_word_idx);
            uint32_t tmask = ((uint32_t)1 << tpos_bit_idx);
            uint32_t smask = ((uint32_t)1 << spos_bit_idx);
            get(bstring.words, tpos_word_idx) &= ~tmask;
            get(bstring.words, tpos_word_idx) |= (uint32_t)((stmp & smask) > 0) <<
                tpos_bit_idx;
            get(bstring.words, spos_word_idx) &= ~smask;
            get(bstring.words, spos_word_idx) |= (uint32_t)((ttmp & tmask) > 0) <<
                spos_bit_idx;
        }
    }

    static inline GraphBitstring graph_bitstring_random(
        size_t len,
        size_t weight,
        AvenRng rng,
        AvenArena *arena
    ) {
        assert(weight > 0);
        GraphBitstring bstring = {
            .words = aven_arena_create_slice(uint32_t, arena, 1 + len / 32),
            .nbits = len,
            .final_word_len = len % 32,
        };
        size_t set_bit_len = weight % 32;
        size_t set_word_len = weight / 32;
        for (size_t i = 0; i < set_word_len; i += 1) {
            get(bstring.words, i) = ~(uint32_t)0;
        }
        get(bstring.words, set_word_len) = (~(uint32_t)0) >> (32 - set_bit_len);
        for (size_t i = set_word_len + 1; i < bstring.words.len; i += 1) {
            get(bstring.words, i) = 0;
        }
        graph_bitstring_shuffle(bstring, rng);
        return bstring;
    }
#endif // GRAPH_BITSTRING_H
