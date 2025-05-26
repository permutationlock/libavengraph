#ifndef GRAPH_PLANE_GEOMETRY_LABEL_H
    #define GRAPH_PLANE_GEOMETRY_LABEL_H

    #include <aven.h>
    #include <aven/arena.h>
    #include <aven/math.h>

    #include <aven/gl.h>
    #include <aven/gl/text.h>

    #include "../../plane.h"

    static inline void graph_plane_geometry_push_label(
        AvenGlTextGeometry *geometry,
        AvenGlTextFont *font,
        GraphPlaneEmbedding embedding,
        uint32_t v,
        Aff2 trans,
        Vec2 label_offset,
        float scale,
        Vec4 color,
        AvenArena arena
    ) {
        AvenStr label_text = aven_str_uint_decimal(v, &arena);
        AvenGlTextLine label_line = aven_gl_text_line(font, label_text, &arena);

        Vec2 pos;
        aff2_transform(pos, trans, get(embedding, v));

        Aff2 label_trans;
        aff2_identity(label_trans);
        aff2_add_vec2(label_trans, label_trans, pos);
        aff2_add_vec2(label_trans, label_trans, label_offset);

        aven_gl_text_geometry_push_line(
            geometry,
            &label_line,
            label_trans,
            scale,
            color
        );
    }

    static inline void graph_plane_geometry_push_labels(
        AvenGlTextGeometry *geometry,
        AvenGlTextFont *font,
        GraphPlaneEmbedding embedding,
        Aff2 trans,
        Vec2 label_offset,
        float scale,
        Vec4 color,
        AvenArena arena
    ) {
        for (uint32_t v = 0; v < embedding.len; v += 1) {
            graph_plane_geometry_push_label(
                geometry,
                font,
                embedding,
                v,
                trans,
                label_offset,
                scale,
                color,
                arena
            );
        }
    }
#endif // GRAPH_PLANE_GEOMETRY_LABEL_H
