#ifndef TEST_GEN_H
    #define TEST_GEN_H

    #include <aven.h>
    #include <aven/arena.h>
    #include <aven/math.h>
    #include <aven/path.h>
    #include <aven/rng.h>
    #include <aven/rng/pcg.h>
    #include <aven/str.h>
    #include <aven/test.h>

    #include <graph.h>
    #include <graph/gen.h>
    #include <graph/plane/gen.h>

    typedef enum {
        TEST_GEN_GRAPH_TYPE_COMPLETE,
        TEST_GEN_GRAPH_TYPE_GRID,
        TEST_GEN_GRAPH_TYPE_PYRAMID,
        TEST_GEN_GRAPH_TYPE_TRIANGULATION,
    } TestGenGraphType;

    static Graph test_gen_graph(
        uint32_t size,
        TestGenGraphType type,
        AvenArena *arena
    ) {
        AvenRngPcg pcg = aven_rng_pcg_seed(0xdead, 0xbeef);
        AvenRng rng = aven_rng_pcg(&pcg);

        Graph graph;
        switch (type) {
            case TEST_GEN_GRAPH_TYPE_COMPLETE:
                graph = graph_gen_complete(size, arena);
                break;
            case TEST_GEN_GRAPH_TYPE_GRID:
                graph = graph_gen_grid(size, size, arena);
                break;
            case TEST_GEN_GRAPH_TYPE_PYRAMID:
                graph = graph_gen_pyramid(size, arena);
                break;
            case TEST_GEN_GRAPH_TYPE_TRIANGULATION:
                graph = graph_gen_triangulation(
                    size,
                    rng,
                    (Vec2){ 0.33f, 0.33f },
                    arena
                );
                break;
            default:
                assert(false);
                break;
        }

        return graph;
    }

#endif // TEST_GEN_H
