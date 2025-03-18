#ifndef GRAPH_IO_H
#define GRAPH_IO_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/io.h>

#include "../graph.h"

typedef enum {
    GRAPH_IO_TYPE_ADJ = 0xad762af,
    GRAPH_IO_TYPE_ADJ_AUG = 0xa8662af,
} GraphIoType;

typedef struct {
    size_t type;
} GraphIoHeader;

#define graph_io_size(g) ( \
        sizeof(GraphIoHeader) + \
        aven_io_slice_size(g.adj) + \
        aven_io_slice_size(g.nb) \
    )

typedef Result(Graph, int) GraphIoResult;

static inline int graph_io_push(AvenIoWriter *writer, Graph graph) {
    GraphIoHeader header = {
        .type = (size_t)GRAPH_IO_TYPE_ADJ,
    };
    int hd_error = aven_io_writer_push_struct(writer, &header);
    if (hd_error != 0) {
        return hd_error;
    }

    int adj_error = aven_io_writer_push_slice(writer, graph.adj);
    if (adj_error != 0) {
        return adj_error;
    }

    int nb_error = aven_io_writer_push_slice(writer, graph.nb);
    if (nb_error != 0) {
        return nb_error;
    }

    return AVEN_IO_ERROR_NONE;
}

static inline GraphIoResult graph_io_pop(
    AvenIoReader *reader,
    AvenArena *arena
) {
    GraphIoHeader header = { 0 };
    int hd_error = aven_io_reader_pop_struct(reader, &header);
    if (hd_error != 0) {
        return (GraphIoResult){ .error = hd_error };
    }
    if (header.type != (size_t)GRAPH_IO_TYPE_ADJ) {
        return (GraphIoResult){ .error = AVEN_IO_ERROR_MISMATCH };
    }

    AvenIoSliceResult adj_res = aven_io_reader_pop_slice(
        GraphAdj,
        reader,
        arena
    );
    if (adj_res.error != 0) {
        return (GraphIoResult){ .error = adj_res.error } ;
    }
    GraphAdjSlice adj = aven_io_slice(GraphAdj, adj_res.payload);

    AvenIoSliceResult nb_res = aven_io_reader_pop_slice(
        uint32_t,
        reader,
        arena
    );
    if (nb_res.error != 0) {
        return (GraphIoResult){ .error = nb_res.error } ;
    }
    GraphNbSlice nb = aven_io_slice(uint32_t, nb_res.payload);

    return (GraphIoResult){
        .payload = { .adj = adj, .nb = nb },
    };
}

typedef Result(GraphAug, int) GraphIoAugResult;

static inline int graph_io_aug_push(AvenIoWriter *writer, GraphAug graph) {
    GraphIoHeader header = {
        .type = (size_t)GRAPH_IO_TYPE_ADJ_AUG,
    };
    int hd_error = aven_io_writer_push_struct(writer, &header);
    if (hd_error != 0) {
        return hd_error;
    }

    int adj_error = aven_io_writer_push_slice(writer, graph.adj);
    if (adj_error != 0) {
        return adj_error;
    }

    int nb_error = aven_io_writer_push_slice(writer, graph.nb);
    if (nb_error != 0) {
        return nb_error;
    }

    return AVEN_IO_ERROR_NONE;
}

static inline GraphIoAugResult graph_io_aug_pop(
    AvenIoReader *reader,
    AvenArena *arena
) {
    GraphIoHeader header = { 0 };
    int hd_error = aven_io_reader_pop_struct(reader, &header);
    if (hd_error != 0) {
        return (GraphIoAugResult){ .error = hd_error };
    }
    if (header.type != (size_t)GRAPH_IO_TYPE_ADJ_AUG) {
        return (GraphIoAugResult){ .error = AVEN_IO_ERROR_MISMATCH };
    }

    AvenIoSliceResult adj_res = aven_io_reader_pop_slice(
        GraphAdj,
        reader,
        arena
    );
    if (adj_res.error != 0) {
        return (GraphIoAugResult){ .error = adj_res.error } ;
    }
    GraphAdjSlice adj = aven_io_slice(GraphAdj, adj_res.payload);

    AvenIoSliceResult nb_res = aven_io_reader_pop_slice(
        GraphAugNb,
        reader,
        arena
    );
    if (nb_res.error != 0) {
        return (GraphIoAugResult){ .error = nb_res.error } ;
    }
    GraphAugNbSlice nb = aven_io_slice(GraphAugNb, nb_res.payload);

    return (GraphIoAugResult){
        .payload = { .adj = adj, .nb = nb },
    };
}

static inline bool graph_io_validate(Graph graph) {
    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
        GraphAdj v_adj = get(graph.adj, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            if (v_adj.index + i >= graph.nb.len) {
                return false;
            }
            uint32_t u = graph_nb(graph.nb, v_adj, i);
            if (u >= graph.adj.len) {
                return false;
            }
        }
    }

    return true;
}

static inline bool graph_io_aug_validate(GraphAug graph) {
    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
        GraphAdj v_adj = get(graph.adj, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            if (v_adj.index + i >= graph.nb.len) {
                return false;
            }
            GraphAugNb vu = graph_aug_nb(graph.nb, v_adj, i);
            if (vu.vertex >= graph.adj.len) {
                return false;
            }
            GraphAdj u_adj = get(graph.adj, vu.vertex);
            if (u_adj.index + vu.back_index >= graph.nb.len) {
                return false;
            }
            GraphAugNb uv = graph_aug_nb(graph.nb, u_adj, vu.back_index);
            if (uv.vertex != v) {
                return false;
            }
        }
    }

    return true;
}

#endif // GRAPH_IO_H
