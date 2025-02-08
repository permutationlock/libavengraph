#ifndef GAME_H
#define GAME_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/gl.h>
#include <aven/gl/shape.h>
#include <aven/gl/text.h>
#include <aven/gl/ui.h>
#include <aven/rng.h>
#include <aven/rng/pcg.h>
#include <aven/time.h>

#include <graph.h>
#include <graph/plane.h>
#include <graph/plane/p3color.h>
#include <graph/plane/p3color_bfs.h>
#include <graph/plane/p3choose.h>

#define GAME_GL_ARENA_SIZE (4096 * 2000)
#define GAME_LEVEL_ARENA_SIZE (4096 * 2000)
#define GAME_ALG_ARENA_SIZE (4096 * 2000)

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

typedef enum {
    GAME_DATA_ALG_TYPE_NONE = 0,
    GAME_DATA_ALG_TYPE_P3COLOR,
    GAME_DATA_ALG_TYPE_P3COLOR_BFS,
    GAME_DATA_ALG_TYPE_P3CHOOSE,
} GameInfoAlgType;

typedef struct {
    GraphPlaneP3ColorCtx ctx;
    Slice(GraphPlaneP3ColorFrameOptional) frames;
} GameInfoAlgP3Color;

typedef struct {
    GraphPlaneP3ColorBfsCtx ctx;
    Slice(GraphPlaneP3ColorBfsFrameOptional) frames;
} GameInfoAlgP3ColorBfs;

typedef struct {
    GraphPlaneP3ChooseCtx ctx;
    Slice(GraphPlaneP3ChooseFrameOptional) frames;
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

typedef struct {
    GameInfoSession session;
    GameInfoAlg alg;
    AvenArena init_arena;
    AvenArena arena;
} GameInfo;

typedef struct {
    AvenGlShapeRoundedCtx ctx;
    AvenGlShapeRoundedGeometry geometry;
    AvenGlShapeRoundedBuffer buffer;
} GameShapes;

typedef struct {
    AvenGlTextFont font;
    AvenGlTextCtx ctx;
    AvenGlTextGeometry geometry;
    AvenGlTextBuffer buffer;
} GameText;

typedef struct {
    GameText text;
    GameShapes shapes;
    AvenGlUi ui;
    AvenArena init_arena;
    AvenArena arena;
    AvenTimeInst last_update;
    GameInfo info;
    AvenRng rng;
    AvenRngPcg pcg;
    int64_t time_step;
    int64_t elapsed;
    int width;
    int height;
    size_t nthreads;
    uint32_t nvertices;
    float min_area;
    float min_coeff;
    uint8_t color_divisions;
    bool paused;
} GameCtx;

GameCtx game_init(AvenGl *gl, AvenArena *arena);
void game_deinit(GameCtx *ctx, AvenGl *gl);
int game_reload(GameCtx *ctx, AvenGl *gl);
int game_update(GameCtx *ctx, AvenGl *gl, int width, int height, AvenArena arena);
void game_mouse_move(GameCtx *ctx, Vec2 pos);
void game_mouse_click(GameCtx *ctx, AvenGlUiMouseEvent event);
 
typedef GameCtx (*GameInitFn)(AvenGl *gl, AvenArena *arena);
typedef int (*GameReloadFn)(GameCtx *ctx, AvenGl *gl);
typedef int (*GameUpdateFn)(
    GameCtx *ctx,
    AvenGl *gl,
    int width,
    int height,
    AvenArena arena
);
typedef void (*GameDeinitFn)(GameCtx *ctx, AvenGl *gl);
typedef void (*GameMouseMoveFn)(GameCtx *ctx, Vec2 pos);
typedef void (*GameMouseClickFn)(GameCtx *ctx, AvenGlUiMouseEvent event);

typedef struct {
    GameInitFn init;
    GameReloadFn reload;
    GameUpdateFn update;
    GameDeinitFn deinit;
    GameMouseMoveFn mouse_move;
    GameMouseClickFn mouse_click;
} GameTable;

#endif // GAME_H

