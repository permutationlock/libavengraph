#ifndef GAME_H
#define GAME_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/gl.h>
#include <aven/gl/shape.h>
#include <aven/gl/text.h>
#include <aven/gl/texture.h>
#include <aven/gl/ui.h>
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

#define GAME_MAX_VERTICES (1000)
#define GAME_MAX_EDGES (3 * GAME_MAX_VERTICES - 6)
#define GAME_COLOR_DIVISIONS (2)

#define GAME_ROUNDED_GEOMETRY_VERTICES  ((GAME_MAX_VERTICES + 3) * 4 * 5)
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
    GAME_INFO_GRAPH_TYPE_GRID,
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
} GameCtx;

GameCtx game_init(AvenGl *gl, AvenArena *arena);
void game_deinit(GameCtx *ctx, AvenGl *gl);
int game_reload(GameCtx *ctx, AvenGl *gl);
bool game_update(
    GameCtx *ctx,
    AvenGl *gl,
    int width,
    int height,
    AvenArena arena
);
void game_damage(GameCtx *ctx);
void game_mouse_move(GameCtx *ctx, Vec2 pos);
void game_mouse_click(GameCtx *ctx, AvenGlUiMouseEvent event);

typedef GameCtx (*GameInitFn)(AvenGl *gl, AvenArena *arena);
typedef int (*GameReloadFn)(GameCtx *ctx, AvenGl *gl);
typedef bool (*GameUpdateFn)(
    GameCtx *ctx,
    AvenGl *gl,
    int width,
    int height,
    AvenArena arena
);
typedef void (*GameDamageFn)(GameCtx *ctx);
typedef void (*GameDeinitFn)(GameCtx *ctx, AvenGl *gl);
typedef void (*GameMouseMoveFn)(GameCtx *ctx, Vec2 pos);
typedef void (*GameMouseClickFn)(GameCtx *ctx, AvenGlUiMouseEvent event);

typedef struct {
    GameInitFn init;
    GameReloadFn reload;
    GameUpdateFn update;
    GameDamageFn damage;
    GameDeinitFn deinit;
    GameMouseMoveFn mouse_move;
    GameMouseClickFn mouse_click;
} GameTable;

#endif // GAME_H

