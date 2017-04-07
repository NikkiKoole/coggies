#ifndef RENDERER_H
#define RENDERER_H

#include "multi_platform.h"
#include "types.h"
#include "resource.h"
#include "states.h"
#include "SDL.h"
#include "SDL_mixer.h"


#include "my_math.h"

#define USES_HALF_FLOAT

// TODO Add some more info into the textures (I need the dimenison for UV creation down the line)


#define VERTEX_FLOAT_TYPE GLfloat
#define GL_FLOAT_TYPE GL_FLOAT



/* #ifdef USES_HALF_FLOAT */
/* #define VERTEX_FLOAT_TYPE __fp16 */
/* #ifdef GL3 */
/* #define GL_FLOAT_TYPE GL_HALF_FLOAT */
/* #endif */

/* #ifdef GLES */
/* #define GL_FLOAT_TYPE GL_HALF_FLOAT_OES */
/* #endif */
/* #else */
/* #define VERTEX_FLOAT_TYPE GLfloat */
/* #define GL_FLOAT_TYPE GL_FLOAT */
/* #endif */

/* // for flycheck on osx */
/* #ifndef GL_FLOAT_TYPE */
/* #ifdef USES_HALF_FLOAT */
/* #define GL_FLOAT_TYPE GL_HALF_FLOAT */
/* #else */
/* #define GL_FLOAT_TYPE GL_FLOAT */
/* #endif */

/* #endif */



typedef struct {
    int amount;
    GLenum type;
    int type_size;
    const char* attr_name;
} ShaderLayoutElement;

typedef struct {
    ShaderLayoutElement elements[32]; // NOTE 32 attributes to a shader thats plenty right?
    int element_count;
    int values_per_vertex;
    int values_per_thing;
} ShaderLayout;


typedef struct {
    Texture character;
    Texture blocks;
    Texture palette;
    Texture menlo;
    //LevelData level;

    GLuint xyz_uv_palette;
    GLuint xyz_rgb;
    GLuint xyz_uv;
    GLuint xy_uv;

    Mix_Music *music1;
    Mix_Chunk *wav1;
    BM_Font menlo_font;
} Assets;


typedef struct {
    u16 width;
    u16 height;
} ViewPort;


#define MAX_IN_BUFFER 2048

typedef struct DrawBuffer {
    u32 max_amount;
    u32 used_count;
    u32 count;
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    GLvoid *map_buffer_pointer;
    GLushort indices[MAX_IN_BUFFER * 6];
    VERTEX_FLOAT_TYPE vertices[MAX_IN_BUFFER * 24];
} DrawBuffer;

#define STATIC_BLOCK_BATCH_COUNT 4
#define DYNAMIC_BLOCK_BATCH_COUNT 1
#define TRANSPARENT_BLOCK_BATCH_COUNT 1
#define ACTOR_BATCH_COUNT 16
#define GLYPH_BATCH_COUNT 2
#define LINE_BATCH_COUNT 2


typedef struct RenderState {
//float tx, ty, rotation;
    Matrix4 mvp;
    ViewPort view;
    SDL_Window *window;
    SDL_GLContext context;
    Assets assets;

    DrawBuffer static_blocks[STATIC_BLOCK_BATCH_COUNT];
    int used_static_block_batches;

    DrawBuffer dynamic_blocks[DYNAMIC_BLOCK_BATCH_COUNT];
    int used_dynamic_block_batches;

    DrawBuffer transparent_blocks[TRANSPARENT_BLOCK_BATCH_COUNT];
    int used_transparent_block_batches;

    DrawBuffer actors[ACTOR_BATCH_COUNT];
    int used_actor_batches;
    DrawBuffer glyphs[GLYPH_BATCH_COUNT];
    int used_glyph_batches;
    DrawBuffer colored_lines[LINE_BATCH_COUNT];
    int used_colored_lines_batches;
    int needs_prepare;
    ShaderLayout debug_text_layout;
    ShaderLayout actors_layout;
    ShaderLayout walls_layout;
    ShaderLayout colored_lines_layout;
} RenderState;


void setup_shader_layouts(RenderState *renderer);
void render(PermanentState *permanent, RenderState *renderer, DebugState *debug);
GLuint make_shader_program(const GLchar *vertexPath, const GLchar *fragmentPath);
void initialize_GL(void);
void prepare_renderer(PermanentState *permanent, RenderState *renderer);

#endif
