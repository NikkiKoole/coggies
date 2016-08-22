#ifndef RESOURCE_H
#define RESOURCE_H
#include "types.h"
#include "multi_platform.h"
#include "SDL_mixer.h"

#define FLATTEN_3D_INDEX(x,y,z, width, height) ((x) + ((y) * (width)) + ((z) * (width) * (height)))


typedef struct {
    u32 id;
    u16 x;
    u16 y;
    u16 width;
    u16 height;
    s16 xoffset;
    s16 yoffset;
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


typedef enum {
    Nothing,
    WallBlock,
    Floor, Grass, Wood, Concrete, Tiles, Carpet,
    Ladder,
    StairsUpMeta,
    StairsFollowUpMeta,
    StairsUp1N, StairsUp2N, StairsUp3N, StairsUp4N,
    StairsDown1N, StairsDown2N, StairsDown3N, StairsDown4N,
    StairsUp1E, StairsUp2E, StairsUp3E, StairsUp4E,
    StairsDown1E, StairsDown2E, StairsDown3E, StairsDown4E,
    StairsUp1S, StairsUp2S, StairsUp3S, StairsUp4S,
    StairsDown1S, StairsDown2S, StairsDown3S, StairsDown4S,
    StairsUp1W, StairsUp2W, StairsUp3W, StairsUp4W,
    StairsDown1W, StairsDown2W, StairsDown3W, StairsDown4W,
    Shaded,
    BlockTotal
} Block;



typedef struct {
    Block object;
    Block floor;
} WorldBlock;

typedef struct {
    WorldBlock *blocks;
    int block_count;
    int x, y, z_level;
} LevelData;

typedef struct {
    int x;
    int y;
    int z_level;
} World_Size;


void resource_font(BM_Font *font, const char *path);
void resource_texture(Texture *t, const char *path);
void resource_shader(GLuint *shader, const char *vertPath, const char *fragPath);
void resource_sprite_atlas(const char *path);
void resource_level(LevelData *level, const char * path);
void resource_ogg(Mix_Music **music, const char *path);
void resource_wav(Mix_Chunk **chunk, const char *path);

#endif
