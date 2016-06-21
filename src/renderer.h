#ifndef RENDERER_H
#define RENDERER_H

#include "multi_platform.h"
#include "types.h"

#include "SDL.h"
#include "SDL_mixer.h"


typedef struct {
    u32 id;
    u16 x;
    u16 y;
    u16 width;
    u16 height;
    u16 xoffset;
    u16 yoffset;
    u16 xadvance;
} BM_Glyph;

typedef struct {
    // perhaps I should keep the texture in here too
    BM_Glyph chars[256];
    u16 line_height;
} BM_Font;


typedef struct Assets {
    GLuint sprite_texture;
    GLuint palette_texture;
    GLuint shader1;
    Mix_Music *music1;
    Mix_Chunk *wav1;
    BM_Font menlo;
    GLuint menlo_texture;
} Assets;


typedef struct ViewPort {
    u16 width;
    u16 height;
} ViewPort;

#define VALUES_PER_ELEM 24
// todo find some useful size 2048 might be too big idk, in total these two arrays use 3.5 Mb atm.
#define MAX_IN_BUFFER 2048
typedef struct DrawBuffer {
    u32 max_amount;
    u32 used_count;
    u32 count;
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    GLushort indices[MAX_IN_BUFFER * 6];
    GLfloat vertices[MAX_IN_BUFFER * 24];
} DrawBuffer;


// there is a maximum of 16384 walls that can be drawn
// since there is a maximum of 2048 walls per draw buffer
// that means I need 8 buffers to draw 16384 walls.  (thats roughly 27 Mb) thats ok.

// before mapping the walls into the bufers I need to filter out all invisible ones though


typedef struct RenderState {
    ViewPort view;
    SDL_Window *window;
    SDL_GLContext *context;
    Assets assets;
    DrawBuffer walls;
    DrawBuffer actors[8];
    int used_actor_batches;
    DrawBuffer glyphs[1];
    int used_glyph_batches;

} RenderState;



void render(SDL_Window *window);
GLuint make_shader_program(const GLchar *vertexPath, const GLchar *fragmentPath);
void initialize_GL(void);
void prepare_renderer(void);


void make_texture(GLuint *tex, const char *path);
void make_font(BM_Font *font, const char *path);
void make_sprite_atlas(const char *path);
#endif
