#ifndef RENDERER_H
#define RENDERER_H

#include "multi_platform.h"
#include "types.h"

#include "SDL.h"
#include "SDL_mixer.h"


typedef struct Assets {
    GLuint tex1;
    GLuint tex2;
    GLuint shader1;
    Mix_Music *music1;
    Mix_Chunk *wav1;
} Assets;

typedef struct ViewPort {
    u16 width;
    u16 height;
} ViewPort;

#define VALUES_PER_ELEM 24
// todo find some useful size 2048 might be too big idk, in total these two arrays use 3.5 Mb atm.
#define MAX_IN_BUFFER 2048
typedef struct DrawBuffer {
    u32 count;
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    GLushort indices[MAX_IN_BUFFER * 6];
    GLfloat vertices[MAX_IN_BUFFER * 24];
} DrawBuffer;


typedef struct RenderState {
    ViewPort view;
    SDL_Window *window;
    SDL_GLContext *context;
    Assets assets;
    DrawBuffer walls;
    DrawBuffer actors;
} RenderState;



void render(SDL_Window *window);
void make_texture(GLuint *tex, const char *path);
GLuint make_shader_program(const GLchar *vertexPath, const GLchar *fragmentPath);
void initialize_GL(void);
void prepare_renderer(void);
#endif
