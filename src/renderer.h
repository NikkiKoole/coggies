#ifndef RENDERER_H
#define RENDERER_H

#include "multi_platform.h"
#include "types.h"
#include "resource.h"

#include "SDL.h"
#include "SDL_mixer.h"



#define USES_HALF_FLOAT

// TODO Add some more info into the textures (I need the dimenison for UV creation down the line)




#ifdef USES_HALF_FLOAT
#define VERTEX_FLOAT_TYPE __fp16
#ifdef GL3
#define GL_FLOAT_TYPE GL_HALF_FLOAT
#endif

#ifdef GLES
#define GL_FLOAT_TYPE GL_HALF_FLOAT_OES
#endif
#else
#define VERTEX_FLOAT_TYPE GLfloat
#define GL_FLOAT_TYPE GL_FLOAT
#endif

// for flycheck on osx
#ifndef GL_FLOAT_TYPE
#ifdef USES_HALF_FLOAT
#define GL_FLOAT_TYPE GL_HALF_FLOAT
#else
#define GL_FLOAT_TYPE GL_FLOAT
#endif

#endif



typedef struct {
    int amount;
    GLenum type;
    int type_size;
    const char* attr_name;
} ShaderLayoutElement;

typedef struct {
    ShaderLayoutElement elements[32]; // NOTE 32 attributes to a shader thats plenty right?
    int element_count;
    int values_per_quad;
} ShaderLayout;


typedef struct {
    Texture sprite;
    Texture palette;
    Texture menlo;
    //LevelData level;

    GLuint xyz_uv_palette;
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

#define VALUES_PER_ELEM 24
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


typedef struct RenderState {
    ViewPort view;
    SDL_Window *window;
    SDL_GLContext *context;
    Assets assets;
    DrawBuffer walls[8];
    int used_wall_batches;
    DrawBuffer actors[32];
    int used_actor_batches;
    DrawBuffer glyphs[2];
    int used_glyph_batches;

} RenderState;


void setup_shader_layouts(void);
void render(SDL_Window *window);
GLuint make_shader_program(const GLchar *vertexPath, const GLchar *fragmentPath);
void initialize_GL(void);
void prepare_renderer(void);


void make_texture(Texture *t, const char *path);
void make_font(BM_Font *font, const char *path);
void make_sprite_atlas(const char *path);
void make_level(LevelData *level, const char *path);
#endif
