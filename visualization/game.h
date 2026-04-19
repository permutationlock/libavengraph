#ifndef GAME_H
    #define GAME_H

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

    #define GAME_INIT_WIDTH (480)
    #define GAME_INIT_HEIGHT (480)

    #define GAME_GL_ARENA_SIZE (4096 * 1000)
    #define GAME_ALG_ARENA_SIZE (4096 * 500)
    #define GAME_LEVEL_ARENA_SIZE (GAME_ALG_ARENA_SIZE + (4096 * 1000))
    #define GAME_ARENA_SIZE (GAME_LEVEL_ARENA_SIZE + GAME_GL_ARENA_SIZE)

    #define GAME_MAX_VERTICES (1100)
    #define GAME_MAX_EDGES (3 * GAME_MAX_VERTICES - 6)
    #define GAME_COLOR_DIVISIONS (2)

    #define GAME_ROUNDED_GEOMETRY_VERTICES ((GAME_MAX_VERTICES + 3) * 4 * 5)
    #define GAME_GEOMETRY_VERTICES (GAME_MAX_EDGES * 4 * 3)

    #define GAME_VERTEX_RADIUS (0.12f)
    #define GAME_MIN_COEFF (0.01f)
    #define GAME_MIN_AREA (GAME_VERTEX_RADIUS * GAME_VERTEX_RADIUS * 2.0f)

    #define GAME_MIN_TIME_STEP (8 * (AVEN_TIME_NSEC_PER_SEC / 1000))
    #define GAME_MAX_TIME_STEP (256 * GAME_MIN_TIME_STEP)
    #define GAME_TIME_STEP (64 * GAME_MIN_TIME_STEP)

    #define GAME_SCREEN_UPDATES (2)

    #define GAME_PREVIEW_EDGES (8)

    typedef enum {
        GAME_DATA_ALG_TYPE_P3COLOR,
        GAME_DATA_ALG_TYPE_P3COLOR_BFS,
        GAME_DATA_ALG_TYPE_P3CHOOSE,
    } GameInfoAlgType;

    typedef struct {
        bool playing;
        int64_t time_step;
    } GameInfoAlgOpts;

    typedef struct {
        GraphPlaneP3ColorCtx ctx;
        GraphPlaneP3ColorFrameOptionalSlice frames;
    } GameInfoAlgP3Color;

    typedef struct {
        GraphPlaneP3ColorBfsCtx ctx;
        Slice(GraphPlaneP3ColorBfsQueue) queues;
        GraphPlaneP3ColorBfsFrameOptionalSlice frames;
    } GameInfoAlgP3ColorBfs;

    typedef struct {
        GraphPlaneP3ChooseCtx ctx;
        GraphPlaneP3ChooseFrameOptionalSlice frames;
    } GameInfoAlgP3Choose;

    typedef struct {
        GameInfoAlgType type;
        union {
            GameInfoAlgP3Color p3color;
            GameInfoAlgP3ColorBfs p3color_bfs;
            GameInfoAlgP3Choose p3choose;
        } data;
        AvenArena init_arena;
        AvenArena arena;
        size_t steps;
        bool done;
    } GameInfoAlg;

    typedef enum {
        GAME_INFO_GRAPH_TYPE_RAND = 0,
        GAME_INFO_GRAPH_TYPE_PYRAMID,
    } GameInfoGraphType;

    typedef struct {
        size_t nthreads;
        size_t radius;
        GameInfoGraphType graph_type;
        GameInfoAlgType alg_type;
    } GameInfoSessionOpts;

    typedef struct {
        Graph graph;
        GraphAug aug_graph;
        GraphPlaneEmbedding embedding;
        GraphSubset p1;
        GraphSubset p2;
        GraphSubset outer_cycle;
        Slice(Vec4) colors;
        GraphPlaneP3ChooseListProp color_lists;
    } GameInfoSession;

    typedef struct {
        GameInfoSession session;
        GameInfoAlg alg;
        AvenRngPcg pcg;
        AvenArena init_arena;
        AvenArena arena;
    } GameInfo;

    typedef struct {
        AvenGlShapeCtx ctx;
        AvenGlShapeGeometry geometry;
        AvenGlShapeBuffer buffer;
    } GameShapes;

    typedef struct {
        AvenGlShapeRoundedCtx ctx;
        AvenGlShapeRoundedGeometry geometry;
        AvenGlShapeRoundedBuffer buffer;
    } GameRoundedShapes;

    typedef struct {
        AvenGlTextureCtx ctx;
        AvenGlTextureBuffer buffer;
        GLuint framebuffer_id;
        GLuint depthbuffer_id;
    } GameGraphTexture;

    typedef enum {
        GAME_UI_WINDOW_NONE = 0,
        GAME_UI_WINDOW_THREAD,
        GAME_UI_WINDOW_RADIUS,
        GAME_UI_WINDOW_ALG,
        GAME_UI_WINDOW_GRAPH,
        GAME_UI_WINDOW_PREVIEW,
    } GameUiWindow;

    typedef struct {
        uint32_t edge_index;
    } GamePreview;

    typedef struct {
        GameShapes shapes;
        GameRoundedShapes rounded_shapes;
        GameGraphTexture graph_texture;
        AvenGlUi ui;
        AvenArena init_arena;
        AvenArena arena;
        AvenTimeInst last_update;
        GameInfo info;
        AvenRngPcg pcg;
        GameInfoSessionOpts session_opts;
        GameInfoAlgOpts alg_opts;
        GameUiWindow active_window;
        GamePreview preview;
        int64_t elapsed;
        int screen_updates;
        int width;
        int height;
        bool ui_up_to_date;
        bool graph_up_to_date;
        bool initialized;
    } GameCtx;

    void game_init(AvenGlWindow *win);
    void game_deinit(AvenGlWindow *win);
    AvenGlWindowAction game_update(AvenGlWindow *win);
    void game_damage(AvenGlWindow *win);
    void game_mouse_move(AvenGlWindow *win, Vec2 pos);
    void game_mouse_click(
        AvenGlWindow *win,
        Vec2 pos,
        int button,
        int action,
        int mods
    );

    static inline GameCtx game_ctx(AvenArena *arena) {
        GameCtx ctx = {
            .width = GAME_INIT_WIDTH,
            .height = GAME_INIT_HEIGHT,
            .session_opts = {
                .alg_type = GAME_DATA_ALG_TYPE_P3COLOR_BFS,
                .graph_type = GAME_INFO_GRAPH_TYPE_RAND,
                .nthreads = 1,
                .radius = 0,
            },
            .alg_opts = { .time_step = GAME_TIME_STEP },
        };

        ctx.init_arena = aven_arena_init(
            aven_arena_alloc(
                arena,
                GAME_GL_ARENA_SIZE,
                AVEN_ARENA_BIGGEST_ALIGNMENT,
                1
            ),
            GAME_GL_ARENA_SIZE
        );

        ctx.info = (GameInfo){
            .init_arena = aven_arena_init(
                aven_arena_alloc(
                    arena,
                    GAME_LEVEL_ARENA_SIZE,
                    AVEN_ARENA_BIGGEST_ALIGNMENT,
                    1
                ),
                GAME_LEVEL_ARENA_SIZE
            ),
        };

        AvenTimeInst now = aven_time_now();
        ctx.pcg = aven_rng_pcg_seed((uint64_t)now.tv_nsec, (uint64_t)now.tv_sec);

        return ctx;
    }
#endif // GAME_H

