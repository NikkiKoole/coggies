#ifndef RESOURCE_H
#define RESOURCE_H
#include "types.h"
#include "multi_platform.h"
#include "SDL_mixer.h"
#include "memory.h"
#include "states.h"

#define FLATTEN_3D_INDEX(x,y,z, width, height) ((x) + ((y) * (width)) + ((z) * (width) * (height)))


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

typedef struct {
    GLuint id;
    u16 width;
    u16 height;
} Texture;

typedef struct {
    u8 format;
    u16 width;
    u16 height;
    u8 bpp;
    u8 *pixels;
} TGA_File;



typedef struct {
    int x;
    int y;
    int z_level;
} World_Size;


void resource_font(BM_Font *font, const char *path);
void resource_texture(Texture *t, const char *path);
void resource_shader(GLuint *shader, const char *vertPath, const char *fragPath);
void resource_sprite_atlas(const char *path);
void resource_level(PermanentState *permanent, LevelData *level, const char * path);
void resource_ogg(Mix_Music **music, const char *path);
void resource_wav(Mix_Chunk **chunk, const char *path);


#endif
