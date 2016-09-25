#ifndef LEVEL_H
#define LEVEL_H

#include "memory.h"
#include "states.h"

#define FLATTEN_3D_INDEX(x,y,z, width, height) ((x) + ((y) * (width)) + ((z) * (width) * (height)))

typedef struct {
    int x;
    int y;
    int z_level;
} World_Size;

void make_level(PermanentState * permanent, LevelData *level,  const char * path);
void read_level_str(PermanentState * permanent, LevelData * level, World_Size dimensions, char *str);
void make_level_str(PermanentState * permanent, LevelData *level, World_Size dimensions,  char * str);

#endif
