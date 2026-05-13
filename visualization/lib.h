#ifndef LIB_H
    #define LIB_H

    #include <aven.h>
    #include <aven/arena.h>
    #include <aven/gl.h>
    #include <aven/gl/shape.h>
    #include <aven/gl/texture.h>
    #include <aven/gl/ui.h>
    #include <aven/gl/window.h>
    #include <aven/rng.h>
    #include <aven/rng/pcg.h>
    #include <aven/time.h>

    #include <graph.h>
    #include <graph/plane.h>
    #include <graph/plane/p3color.h>
    #include <graph/plane/p3color_bfs.h>
    #include <graph/plane/p3choose.h>

    #define LIB_INIT_WIDTH (480)
    #define LIB_INIT_HEIGHT (480)

    #define LIB_GL_ARENA_SIZE (4096 * 1000)
    #define LIB_ALG_ARENA_SIZE (4096 * 500)
    #define LIB_LEVEL_ARENA_SIZE (LIB_ALG_ARENA_SIZE + (4096 * 1000))
    #define LIB_ARENA_SIZE (LIB_LEVEL_ARENA_SIZE + LIB_GL_ARENA_SIZE)

    #define LIB_MAX_VERTICES (1100)
    #define LIB_MAX_EDGES (3 * LIB_MAX_VERTICES - 6)
    #define LIB_COLOR_DIVISIONS (2)

    #define LIB_ROUNDED_GEOMETRY_VERTICES ((LIB_MAX_VERTICES + 3) * 4 * 5)
    #define LIB_GEOMETRY_VERTICES (LIB_MAX_EDGES * 4 * 3)

    #define LIB_VERTEX_RADIUS (0.12f)
    #define LIB_MIN_COEFF (0.01f)
    #define LIB_MIN_AREA (LIB_VERTEX_RADIUS * LIB_VERTEX_RADIUS * 2.0f)

    #define LIB_MIN_TIME_STEP (8 * (AVEN_TIME_NSEC_PER_SEC / 1000))
    #define LIB_MAX_TIME_STEP (256 * LIB_MIN_TIME_STEP)
    #define LIB_TIME_STEP (64 * LIB_MIN_TIME_STEP)

    #define LIB_SCREEN_UPDATES (2)

    #define LIB_PREVIEW_EDGES (8)

    typedef enum {
        LIB_DATA_ALG_TYPE_P3COLOR,
        LIB_DATA_ALG_TYPE_P3COLOR_BFS,
        LIB_DATA_ALG_TYPE_P3CHOOSE,
    } LibInfoAlgType;

    typedef struct {
        bool playing;
        int64_t time_step;
    } LibInfoAlgOpts;

    typedef struct {
        GraphPlaneP3ColorCtx ctx;
        GraphPlaneP3ColorFrameOptionalSlice frames;
    } LibInfoAlgP3Color;

    typedef struct {
        GraphPlaneP3ColorBfsCtx ctx;
        Slice(GraphPlaneP3ColorBfsQueue) queues;
        GraphPlaneP3ColorBfsFrameOptionalSlice frames;
    } LibInfoAlgP3ColorBfs;

    typedef struct {
        GraphPlaneP3ChooseCtx ctx;
        GraphPlaneP3ChooseFrameOptionalSlice frames;
    } LibInfoAlgP3Choose;

    typedef struct {
        LibInfoAlgType type;
        union {
            LibInfoAlgP3Color p3color;
            LibInfoAlgP3ColorBfs p3color_bfs;
            LibInfoAlgP3Choose p3choose;
        } data;
        AvenArena init_arena;
        AvenArena arena;
        size_t steps;
        bool done;
    } LibInfoAlg;

    typedef enum {
        LIB_INFO_GRAPH_TYPE_RAND = 0,
        LIB_INFO_GRAPH_TYPE_PYRAMID,
    } LibInfoGraphType;

    typedef struct {
        size_t nthreads;
        size_t radius;
        LibInfoGraphType graph_type;
        LibInfoAlgType alg_type;
    } LibInfoSessionOpts;

    typedef struct {
        Graph graph;
        GraphAug aug_graph;
        GraphPlaneEmbedding embedding;
        GraphSubset p1;
        GraphSubset p2;
        GraphSubset outer_cycle;
        Slice(Vec4) colors;
        GraphPlaneP3ChooseListProp color_lists;
    } LibInfoSession;

    typedef struct {
        LibInfoSession session;
        LibInfoAlg alg;
        AvenRngPcg pcg;
        AvenArena init_arena;
        AvenArena arena;
    } LibInfo;

    typedef struct {
        AvenGlShapeCtx ctx;
        AvenGlShapeGeometry geometry;
        AvenGlShapeBuffer buffer;
    } LibShapes;

    typedef struct {
        AvenGlShapeRoundedCtx ctx;
        AvenGlShapeRoundedGeometry geometry;
        AvenGlShapeRoundedBuffer buffer;
    } LibRoundedShapes;

    typedef struct {
        AvenGlTextureCtx ctx;
        AvenGlTextureBuffer buffer;
        GLuint framebuffer_id;
        GLuint depthbuffer_id;
    } LibGraphTexture;

    typedef enum {
        LIB_UI_WINDOW_NONE = 0,
        LIB_UI_WINDOW_THREAD,
        LIB_UI_WINDOW_RADIUS,
        LIB_UI_WINDOW_ALG,
        LIB_UI_WINDOW_GRAPH,
        LIB_UI_WINDOW_PREVIEW,
    } LibUiWindow;

    typedef struct {
        uint32_t edge_index;
    } LibPreview;

    typedef struct {
        LibShapes shapes;
        LibRoundedShapes rounded_shapes;
        LibGraphTexture graph_texture;
        AvenGlUi ui;
        AvenArena init_arena;
        AvenArena arena;
        AvenTimeInst last_update;
        LibInfo info;
        AvenRngPcg pcg;
        LibInfoSessionOpts session_opts;
        LibInfoAlgOpts alg_opts;
        LibUiWindow active_window;
        LibPreview preview;
        int64_t elapsed;
        int screen_updates;
        int width;
        int height;
        bool ui_up_to_date;
        bool graph_up_to_date;
        bool initialized;
    } LibCtx;

    void lib_init(AvenGlWindow *win);
    void lib_deinit(AvenGlWindow *win);
    AvenGlWindowAction lib_update(AvenGlWindow *win);
    void lib_damage(AvenGlWindow *win);
    void lib_mouse_move(AvenGlWindow *win, Vec2 pos);
    void lib_mouse_click(
        AvenGlWindow *win,
        Vec2 pos,
        AvenGlWindowMouse button,
        AvenGlWindowPress action,
        uint32_t mods
    );

    static inline LibCtx lib_ctx(AvenArena *arena) {
        LibCtx ctx = {
            .width = LIB_INIT_WIDTH,
            .height = LIB_INIT_HEIGHT,
            .session_opts = {
                .alg_type = LIB_DATA_ALG_TYPE_P3COLOR_BFS,
                .graph_type = LIB_INFO_GRAPH_TYPE_RAND,
                .nthreads = 1,
                .radius = 0,
            },
            .alg_opts = { .time_step = LIB_TIME_STEP },
        };

        ctx.init_arena = aven_arena_init(
            aven_arena_alloc(
                arena,
                LIB_GL_ARENA_SIZE,
                AVEN_ARENA_BIGGEST_ALIGNMENT,
                1
            ),
            LIB_GL_ARENA_SIZE
        );

        ctx.info = (LibInfo){
            .init_arena = aven_arena_init(
                aven_arena_alloc(
                    arena,
                    LIB_LEVEL_ARENA_SIZE,
                    AVEN_ARENA_BIGGEST_ALIGNMENT,
                    1
                ),
                LIB_LEVEL_ARENA_SIZE
            ),
        };

        AvenTimeInst now = aven_time_now();
        ctx.pcg = aven_rng_pcg_seed((uint64_t)now.tv_nsec, (uint64_t)now.tv_sec);

        return ctx;
    }
#endif // LIB_H

