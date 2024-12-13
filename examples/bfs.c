#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/fs.h>
#include <aven/gl.h>
#include <aven/gl/shape.h>
#include <aven/graph.h>
#include <aven/graph/plane.h>
#include <aven/graph/plane/gen.h>
#include <aven/graph/plane/geometry.h>
#include <aven/graph/bfs.h>
#include <aven/path.h>
#include <aven/time.h>

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#define ARENA_PAGE_SIZE 4096
#define GRAPH_ARENA_PAGES 1000
#define ARENA_PAGES (GRAPH_ARENA_PAGES + 1000)

#define GRAPH_MAX_VERTICES (256)
#define GRAPH_MAX_EDGES (3 * GRAPH_MAX_VERTICES - 6)

#define VERTEX_RADIUS (0.04f)

#define BFS_TIMESTEP (AVEN_TIME_NSEC_PER_SEC / 10)

typedef struct {
    AvenGlShapeCtx ctx;
    AvenGlShapeGeometry geometry;
    AvenGlShapeBuffer buffer;
} EdgeShapes;

typedef struct {
    AvenGlShapeRoundedCtx ctx;
    AvenGlShapeRoundedGeometry geometry;
    AvenGlShapeRoundedBuffer buffer;
} VertexShapes;

typedef struct {
    VertexShapes vertex_shapes;
    EdgeShapes edge_shapes;
    AvenGraph graph;
    AvenGraphPlaneEmbedding embedding;
    AvenGraphSubset path_space;
    AvenGraphBfsCtx bfs_ctx;
    Optional(AvenGraphSubset) bfs_path;
    AvenTimeInst last_update;
    int64_t elapsed;
} AppCtx;

static GLFWwindow *window;
static AvenGl gl;
static AppCtx ctx;
static AvenArena arena;

static void app_init(void) {
    ctx = (AppCtx){ 0 };
 
    ctx.edge_shapes.ctx = aven_gl_shape_ctx_init(&gl);
    ctx.edge_shapes.geometry = aven_gl_shape_geometry_init(
        (GRAPH_MAX_EDGES) * 4 * 3,
        (GRAPH_MAX_EDGES) * 6 * 3,
        &arena
    );
    ctx.edge_shapes.buffer = aven_gl_shape_buffer_init(
        &gl,
        &ctx.edge_shapes.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

    ctx.vertex_shapes.ctx = aven_gl_shape_rounded_ctx_init(&gl);
    ctx.vertex_shapes.geometry = aven_gl_shape_rounded_geometry_init(
        (GRAPH_MAX_VERTICES) * 4 * 3,
        (GRAPH_MAX_VERTICES) * 6 * 3,
        &arena
    );
    ctx.vertex_shapes.buffer = aven_gl_shape_rounded_buffer_init(
        &gl,
        &ctx.vertex_shapes.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

    AvenGraphPlaneGenData gen_data = aven_graph_plane_gen_grid(5, 5, &arena);
    ctx.graph = gen_data.graph;
    ctx.embedding = gen_data.embedding;

    ctx.path_space.len = ctx.graph.len;
    ctx.path_space.ptr = aven_arena_create_array(
        uint32_t,
        &arena,
        ctx.path_space.len
    );

    ctx.bfs_ctx = aven_graph_bfs_init(
        ctx.graph,
        0,
        &arena
    );

    ctx.last_update = aven_time_now();
}

static void app_deinit(void) {
    aven_gl_shape_rounded_geometry_deinit(&ctx.vertex_shapes.geometry);
    aven_gl_shape_rounded_buffer_deinit(&gl, &ctx.vertex_shapes.buffer);
    aven_gl_shape_rounded_ctx_deinit(&gl, &ctx.vertex_shapes.ctx);
    aven_gl_shape_geometry_deinit(&ctx.edge_shapes.geometry);
    aven_gl_shape_buffer_deinit(&gl, &ctx.edge_shapes.buffer);
    aven_gl_shape_ctx_deinit(&gl, &ctx.edge_shapes.ctx);
    ctx.bfs_path.valid = false;
}

static void app_update(
    int width,
    int height
) {
    float ratio = (float)width / (float)height;

    float norm_height = 1.0f;
    float norm_width = ratio;
    float pixel_size = 2.0f / (float)height;

    if (ratio < 1.0f) {
        norm_height = 1.0f / ratio;
        norm_width = 1.0f;
        pixel_size = 2.0f / (float)width;
    }

    AvenTimeInst now = aven_time_now();
    int64_t elapsed = aven_time_since(now, ctx.last_update);
    ctx.last_update = now;
    ctx.elapsed += elapsed;

    while (ctx.elapsed >= BFS_TIMESTEP) {
        ctx.elapsed -= BFS_TIMESTEP;
        if (ctx.bfs_path.valid) {
            break;
        }

        bool done = aven_graph_bfs_step(&ctx.bfs_ctx);
        if (done) {
            ctx.bfs_path.value = aven_graph_bfs_path(
                ctx.path_space,
                &ctx.bfs_ctx,
                (uint32_t)ctx.graph.len - 1
            );
            ctx.bfs_path.valid = true;
        }
    }

    aven_gl_shape_geometry_clear(&ctx.edge_shapes.geometry);
    aven_gl_shape_rounded_geometry_clear(&ctx.vertex_shapes.geometry);

    Aff2 graph_transform;
    aff2_identity(graph_transform);
    aff2_stretch(
        graph_transform,
        (Vec2){ norm_width, norm_height },
        graph_transform
    );

    AvenGraphPlaneGeometryEdge edge_info = {
        .color = { 0.0f, 0.0f, 0.0f, 1.0f },
        .thickness = VERTEX_RADIUS / 4.0f,
    };

    aven_graph_plane_geometry_push_edges(
        &ctx.edge_shapes.geometry,
        ctx.graph,
        ctx.embedding,
        graph_transform,
        &edge_info
    );

    if (ctx.bfs_path.valid) {
        AvenGraphSubset path = ctx.bfs_path.value;
        assert(path.len != 0);

        AvenGraphPlaneGeometryEdge path_edge_info = {
            .color = { 1.0f, 0.0f, 0.0f, 1.0f },
            .thickness = VERTEX_RADIUS / 3.0f,
        };

        aven_graph_plane_geometry_push_path_edges(
            &ctx.edge_shapes.geometry,
            ctx.embedding,
            path,
            graph_transform,
            &path_edge_info
        );
    } else {
        AvenGraphPlaneGeometryEdge visited_edge_info = {
            .color = { 0.0f, 1.0f, 0.0f, 1.0f },
            .thickness = VERTEX_RADIUS / 3.0f,
        };

        for (uint32_t v = 0; v < ctx.graph.len; v += 1) {
            uint32_t parent = get(ctx.bfs_ctx.parents, v);
            if (parent != AVEN_GRAPH_BFS_VERTEX_INVALID) {
                aven_graph_plane_geometry_push_edge(
                    &ctx.edge_shapes.geometry,
                    ctx.embedding,
                    v,
                    parent,
                    graph_transform,
                    &visited_edge_info
                );
            }
        }

        AvenGraphPlaneGeometryEdge active_edge_info = {
            .color = { 0.0f, 0.0f, 1.0f, 1.0f },
            .thickness = VERTEX_RADIUS / 3.0f,
        };

        if (ctx.bfs_ctx.vertex != AVEN_GRAPH_BFS_VERTEX_INVALID) {
            AvenGraphAdjList adj = get(ctx.graph, ctx.bfs_ctx.vertex);
            if (ctx.bfs_ctx.neighbor < adj.len) {
                uint32_t neighbor = get(adj, ctx.bfs_ctx.neighbor);
                aven_graph_plane_geometry_push_edge(
                    &ctx.edge_shapes.geometry,
                    ctx.embedding,
                    ctx.bfs_ctx.vertex,
                    neighbor,
                    graph_transform,
                    &active_edge_info
                );
            }
        }
    }

    AvenGraphPlaneGeometryNode node_outline_info = {
        .color = { 0.0f, 0.0f, 0.0f, 1.0f },
        .mat = {
            { 1.25f * VERTEX_RADIUS, 0.0f },
            { 0.0f, 1.25f * VERTEX_RADIUS }
        },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };

    AvenGraphPlaneGeometryNode node_empty_info = {
        .color = { 0.9f, 0.9f, 0.9f, 1.0f },
        .mat = { { VERTEX_RADIUS, 0.0f }, { 0.0f, VERTEX_RADIUS } },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };

    aven_graph_plane_geometry_push_vertices(
        &ctx.vertex_shapes.geometry,
        ctx.embedding,
        graph_transform,
        &node_outline_info
    );

    if (ctx.bfs_path.valid) {
        AvenGraphSubset path = ctx.bfs_path.value;
        assert(path.len != 0);

        aven_graph_plane_geometry_push_vertices(
            &ctx.vertex_shapes.geometry,
            ctx.embedding,
            graph_transform,
            &node_empty_info
        ); 

        AvenGraphPlaneGeometryNode path_node_info = {
            .color = { 1.0f, 0.0f, 0.0f, 1.0f },
            .mat = { { VERTEX_RADIUS, 0.0f, }, { 0.0f, VERTEX_RADIUS } },
            .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
            .roundness = 1.0f,
        };

        aven_graph_plane_geometry_push_subset_vertices(
            &ctx.vertex_shapes.geometry,
            ctx.embedding,
            path,
            graph_transform,
            &path_node_info
        );
    } else {
        aven_graph_plane_geometry_push_marked_vertices(
            &ctx.vertex_shapes.geometry,
            ctx.embedding,
            ctx.bfs_ctx.parents,
            AVEN_GRAPH_BFS_VERTEX_INVALID,
            graph_transform,
            &node_empty_info
        );

        AvenGraphPlaneGeometryNode visited_node_info = {
            .color = { 0.0f, 1.0f, 0.0f, 1.0f },
            .mat = { { VERTEX_RADIUS, 0.0f }, { 0.0f, VERTEX_RADIUS } },
            .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
            .roundness = 1.0f,
        };

        AvenGraphPlaneGeometryNode active_node_info = {
            .color = { 0.0f, 0.0f, 1.0f, 1.0f },
            .mat = { { VERTEX_RADIUS, 0.0f }, { 0.0f, VERTEX_RADIUS } },
            .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
            .roundness = 1.0f,
        };

        aven_graph_plane_geometry_push_unmarked_vertices(
            &ctx.vertex_shapes.geometry,
            ctx.embedding,
            ctx.bfs_ctx.parents,
            AVEN_GRAPH_BFS_VERTEX_INVALID,
            graph_transform,
            &visited_node_info
        );

        if (ctx.bfs_ctx.vertex != AVEN_GRAPH_BFS_VERTEX_INVALID) {
            aven_graph_plane_geometry_push_vertex(
                &ctx.vertex_shapes.geometry,
                ctx.embedding,
                ctx.bfs_ctx.vertex,
                graph_transform,
                &active_node_info
            );
        }
    }

    AvenGraphPlaneGeometryNode target_node_info = {
        .color = { 1.0f, 0.0f, 1.0f, 1.0f },
        .mat = { { VERTEX_RADIUS, 0.0f }, { 0.0f, VERTEX_RADIUS } },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };

    aven_graph_plane_geometry_push_vertex(
        &ctx.vertex_shapes.geometry,
        ctx.embedding,
        (uint32_t)ctx.graph.len - 1,
        graph_transform,
        &target_node_info
    );

    gl.Viewport(0, 0, width, height);
    assert(gl.GetError() == 0);
    gl.ClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    assert(gl.GetError() == 0);
    gl.Clear(GL_COLOR_BUFFER_BIT);
    assert(gl.GetError() == 0);

    float border_padding = 2.0f * VERTEX_RADIUS;
    float scale = 1.0f / (1.0f + border_padding);

    Aff2 cam_transform;
    aff2_camera_position(
        cam_transform,
        (Vec2){ 0.0f, 0.0f },
        (Vec2){ norm_width, norm_height },
        0.0f
    );
    aff2_stretch(cam_transform, (Vec2){ scale, scale }, cam_transform);

    aven_gl_shape_buffer_update(
        &gl,
        &ctx.edge_shapes.buffer,
        &ctx.edge_shapes.geometry
    );
    aven_gl_shape_rounded_buffer_update(
        &gl,
        &ctx.vertex_shapes.buffer,
        &ctx.vertex_shapes.geometry
    );

    aven_gl_shape_draw(
        &gl,
        &ctx.edge_shapes.ctx,
        &ctx.edge_shapes.buffer,
        cam_transform
    );
    aven_gl_shape_rounded_draw(
        &gl,
        &ctx.vertex_shapes.ctx,
        &ctx.vertex_shapes.buffer,
        pixel_size,
        cam_transform
    );
}

static void error_callback(int error, const char *description) {
    fprintf(stderr, "GFLW Error (%d): %s", error, description);
}

static void key_callback(
    GLFWwindow *win,
    int key,
    int scancode,
    int action,
    int mods
) {
    (void)scancode;
    (void)mods;
    if (key == GLFW_KEY_ESCAPE and action == GLFW_PRESS) {
        glfwSetWindowShouldClose(win, GLFW_TRUE);
    }
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

void main_loop(void) {
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);

    app_update(width, height);

    glfwSwapBuffers(window);
    glfwPollEvents();
}
#endif
 
#if defined(_MSC_VER)
int WinMain(void) {
#else // !defined(_MSC_VER)
int main(void) {
#endif // !defined(_MSC_VER)
    aven_fs_utf8_mode();

    // Should probably switch to raw page allocation, but malloc is cross
    // platform and we are dynamically linking the system libc anyway
    void *mem = malloc(ARENA_PAGES * ARENA_PAGE_SIZE);
    if (mem == NULL) {
        fprintf(stderr, "OUT OF MEMORY\n");
        abort();
    }

    arena = aven_arena_init(mem, ARENA_PAGES * ARENA_PAGE_SIZE);

    int width = 480;
    int height = 480;

    glfwInit();
    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    // Raster msaa sample count should try to match the sample count of 4 used
    // for multisampling in our shape fragment shaders and stb font textures
    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(
        (int)width,
        (int)height,
        "AvenGraph BFS Example",
        NULL,
        NULL
    );
    if (window == NULL) {
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
        window = glfwCreateWindow(
            (int)width,
            (int)height,
            "AvenGl Test",
            NULL,
            NULL
        );
        if (window == NULL) {
            printf("test failed: glfwCreateWindow\n");
            return 1;
        }
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 0);
#endif // __EMSCRIPTEN__

    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    gl = aven_gl_load(glfwGetProcAddress);
    app_init();

#if !defined(__EMSCRIPTEN__) // !defined(__EMSCRIPTEN__)
    while (!glfwWindowShouldClose(window)) {
        glfwGetFramebufferSize(window, &width, &height);

        app_update(width, height);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    app_deinit();

    glfwDestroyWindow(window);
    glfwTerminate();
#endif // !defined(__EMSCRIPTEN__)

    // we let the OS free arena memory (or leave it in the case of Emscripten)

    return 0;
}
