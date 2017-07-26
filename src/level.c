#include "level.h"
#include <string.h>

#define BUF_SIZE 1024

typedef struct {
    int line_counter;
    int height_counter;
} LevelCounters;


internal b32 exists(const char *fname) {
    FILE *f;
    if ((f = fopen(fname, "r"))) {
        fclose(f);
        return true;
    }
    return false;
}

internal int starts_with(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

internal World_Size validate_and_get_dimensions(const char * path){
  World_Size result = {0,0,0};
    char str[BUF_SIZE];

    FILE *f = fopen(path, "rb");

    if (!f) {
        printf("Couldn't open file: %s\n", path);
    }

    // first  line should contain MAP string
    if (fgets(str, BUF_SIZE, f) != NULL) {
        //printf("map header found ? %d\n", starts_with("MAP", str));
    }

    //second line should contain some version info
    if (fgets(str, BUF_SIZE, f) != NULL) {
        if (atoi(str) != 1) {
            printf("I only grasp version 1.0 at the moment\n");
        } else {
            //printf("version %d\n", atoi(str));
        }
    }

    // third line should contain 3 values, with with commas inbetween, width/depth,height
    if (fgets(str, BUF_SIZE, f) != NULL) {
        char * pch;
        pch = strtok(str,",");
        result.x = atoi(pch);
        pch = strtok(NULL, ",");
        result.y = atoi(pch);
        pch = strtok(NULL, ",");
        result.z_level = atoi(pch);
        //printf("width(x) %d, depth(z) %d, height(y) %d\n", result.width, result.depth, result.height);
    }
    fclose(f);
    return result;
}



internal void read_level_line(LevelCounters * c, int LINES_BEFORE_DATA, World_Size dimensions, LevelData * level,  char * str) {
    if ((c->line_counter - LINES_BEFORE_DATA) % (dimensions.y+1) == 0 ) {
//printf("%s\n", str);
            if (str[0] == '+' && str[dimensions.x + 1] == '+') {
                c->height_counter++;
                if (c->height_counter < dimensions.z_level) {
                    //printf("check! height = %d", height_counter);
                }
            } else {
                printf("missing + signs at the corners.\n");
            }
        } else {
            if (str[0] == '|' && str[dimensions.x+1] == '|') {
                //printf("X %3d,%d, %s ",((line_counter-LINES_BEFORE_DATA) % (dimensions.y+1))-1, height_counter ,str);
                // this is where its at
                int _z_level = c->height_counter;
                int _y_level = (c->line_counter-LINES_BEFORE_DATA) % (dimensions.y+1)-1;

                for (int i = 1; i < dimensions.x+1; i++) {
                    WorldBlock *b = &level->blocks[FLATTEN_3D_INDEX(i-1,_y_level,_z_level, dimensions.x, dimensions.y)];

                    switch(str[i]) {
                    case 'W':{
                        b->object = WindowBlock;
                        break;
                    }
                    case '#':{
                        b->object = WallBlock;
                        break;
                    }
                    case 'L':{
                        b->object = LadderUp;
                        break;
                    }
                    case 'l':{
                        b->object = LadderDown;
                        break;
                    }
                    case 'Z':{
                        b->object = LadderUpDown;
                        break;
                    }
                    case 'S':{
                        b->object = StairsMeta;
                        break;
                    }
                    case 'U':{
                        b->object = EscalatorUpMeta;
                        break;
                    }
                    case 'D':{
                        b->object = EscalatorDownMeta;
                        break;
                    }
                    case '=':{
                        b->object = StairsFollowUpMeta;
                        break;
                    }

                    case '.':{
                        b->object = Floor;
                        break;
                    }
                    default:{
                        b->object = Nothing;
                        break;
                    }
                    }
                }
            } else {
                printf("missing | signs at begin and end. %s. line:%d\n", str, (c->line_counter - LINES_BEFORE_DATA));

            }
        }
        c->line_counter++;


}

void read_level_str(PermanentState * permanent, LevelData * level, World_Size dimensions, char *str) {
    level->x = dimensions.x;
    level->y = dimensions.y;
    level->z_level = dimensions.z_level;
    level->block_count = dimensions.x * dimensions.y * dimensions.z_level;
    level->blocks = (WorldBlock*) PUSH_ARRAY(&permanent->arena, (level->block_count), WorldBlock);
    //printf("reading string level \n");
    {
        for (int z = 0; z< dimensions.z_level; z++){
            for (int y = 0; y < dimensions.y; y++) {
                for(int x = 0; x<dimensions.x;x++){
                    WorldBlock *b = &level->blocks[FLATTEN_3D_INDEX(x,y,z, dimensions.x, dimensions.y)];
                    b->object = Nothing;

                }
            }
        }
    }

    LevelCounters counter;
    counter.line_counter = 0;
    counter.height_counter = -1;;

    char * curLine = str;
    while(curLine) {
        char * nextLine = strchr(curLine, '\n');
        if (nextLine) *nextLine = '\0';  // temporarily terminate the current line
        //printf("curLine=[%s]\n", curLine);
        read_level_line(&counter, 0, dimensions, level, curLine);
        if (nextLine) *nextLine = '\n';  // then restore newline-char, just to be tidy
        curLine = nextLine ? (nextLine+1) : NULL;
    }


}

internal void read_level(PermanentState * permanent, LevelData * level, World_Size dimensions, const char *path) {
    level->x = dimensions.x;
    level->y = dimensions.y;
    level->z_level = dimensions.z_level;
    level->block_count = dimensions.x * dimensions.y * dimensions.z_level;
    level->blocks = (WorldBlock*) PUSH_ARRAY(&permanent->arena, (level->block_count), WorldBlock);
    //printf("reading level : %s \n", path);
    {
        for (int z = 0; z< dimensions.z_level; z++){
            for (int y = 0; y < dimensions.y; y++) {
                for(int x = 0; x<dimensions.x;x++){
                    WorldBlock *b = &level->blocks[FLATTEN_3D_INDEX(x,y,z, dimensions.x, dimensions.y)];
                    b->object = Nothing;

                }
            }
        }
    }

    char str[BUF_SIZE];
    FILE *f = fopen(path, "rb");
    // Skip the first 3 lines. (I've already validated them);
    for (int i = 0; i<3;i++){
        if (fgets(str, BUF_SIZE, f) != NULL) {
        }
    }

    LevelCounters counter;
    counter.line_counter   = 3;
    counter.height_counter = -1;;

    while(fgets(str, BUF_SIZE, f) != NULL) {
        read_level_line(&counter, 3, dimensions, level, str);
    }

    fclose(f);

}


internal b32 has_block_at(LevelData *level, u32 x, u32 y, u32 z_level, Block type) {
    WorldBlock *b = &level->blocks[FLATTEN_3D_INDEX(x,y,z_level, level->x, level->y)];
    return b->object == type;

}

internal void set_block_at(LevelData *level, u32 x, u32 y, u32 z_level, Block type) {
    WorldBlock *b = &level->blocks[FLATTEN_3D_INDEX(x,y,z_level, level->x, level->y)];
    b->object = type;

}

internal void set_meta_block_at(LevelData *level, u32 x, u32 y, u32 z_level, Block type) {
    WorldBlock *b = &level->blocks[FLATTEN_3D_INDEX(x,y,z_level, level->x, level->y)];
    b->meta_object = type;

}

internal void add_stairs(LevelData *level){
    for (int z = 0; z < level->z_level; z++){
        for (int y = 0; y < level->y; y++){
            for (int x = 0; x < level->x; x++){
                Block b = Nothing;
                if (has_block_at(level, x, y, z, StairsMeta)) {
                    b = StairsMeta;
                } else if(has_block_at(level, x, y, z, EscalatorUpMeta)) {
                    b = EscalatorUpMeta;
                } else if(has_block_at(level, x, y, z, EscalatorDownMeta)) {
                    b = EscalatorDownMeta;
                }

                if (b != Nothing) {
                    if (y-4 >= 0) {
                        b32 is_stairs_north = true;

                        for (int i = 1; i < 4; i++) {
                            if (has_block_at(level, x, y-i, z, StairsFollowUpMeta)) {
                            } else{
                                is_stairs_north = false;
                            }
                        }
                        if (is_stairs_north) {
                            set_block_at(level, x, y, z,   (b == StairsMeta) ? Stairs1N : (b == EscalatorUpMeta) ? EscalatorUp1N : EscalatorDown1N);
                            set_block_at(level, x, y-1, z, (b == StairsMeta) ? Stairs2N : (b == EscalatorUpMeta) ? EscalatorUp2N : EscalatorDown2N);
                            set_block_at(level, x, y-2, z, (b == StairsMeta) ? Stairs3N : (b == EscalatorUpMeta) ? EscalatorUp3N : EscalatorDown3N);
                            set_block_at(level, x, y-3, z, (b == StairsMeta) ? Stairs4N : (b == EscalatorUpMeta) ? EscalatorUp4N : EscalatorDown4N);
                        }
                    }

                    if (x+4 < level->x){
                        b32 is_stairs_east = true;

                        for (int i = 1; i < 4; i++) {
                            if (has_block_at(level, x+i, y, z, StairsFollowUpMeta)) {
                            } else{
                                is_stairs_east = false;
                            }
                        }
                        if (is_stairs_east) {
                            set_block_at(level, x, y, z,   (b == StairsMeta) ? Stairs1E : (b == EscalatorUpMeta) ? EscalatorUp1E : EscalatorDown1E);
                            set_block_at(level, x+1, y, z, (b == StairsMeta) ? Stairs2E : (b == EscalatorUpMeta) ? EscalatorUp2E : EscalatorDown2E);
                            set_block_at(level, x+2, y, z, (b == StairsMeta) ? Stairs3E : (b == EscalatorUpMeta) ? EscalatorUp3E : EscalatorDown3E);
                            set_block_at(level, x+3, y, z, (b == StairsMeta) ? Stairs4E : (b == EscalatorUpMeta) ? EscalatorUp4E : EscalatorDown4E);
                        }
                    }
                    if (y+4 < level->y) {
                        b32 is_stairs_south = true;

                        for (int i = 1; i < 4; i++) {
                            if (has_block_at(level, x, y+i, z, StairsFollowUpMeta)) {
                            } else{
                                is_stairs_south = false;
                            }
                        }
                        if (is_stairs_south) {
                            set_block_at(level, x, y, z,   (b == StairsMeta) ? Stairs1S : (b == EscalatorUpMeta) ? EscalatorUp1S : EscalatorDown1S);
                            set_block_at(level, x, y+1, z, (b == StairsMeta) ? Stairs2S : (b == EscalatorUpMeta) ? EscalatorUp2S : EscalatorDown2S);
                            set_block_at(level, x, y+2, z, (b == StairsMeta) ? Stairs3S : (b == EscalatorUpMeta) ? EscalatorUp3S : EscalatorDown3S);
                            set_block_at(level, x, y+3, z, (b == StairsMeta) ? Stairs4S : (b == EscalatorUpMeta) ? EscalatorUp4S : EscalatorDown4S);
                        }
                    }
                    if (x-4 >= 0){
                        b32 is_stairs_west = true;

                        for (int i = 1; i < 4; i++) {
                            if (has_block_at(level, x-i, y, z, StairsFollowUpMeta)) {
                            } else{
                                is_stairs_west = false;
                            }
                        }
                        if (is_stairs_west) {
                            set_block_at(level, x, y, z,   (b == StairsMeta) ? Stairs1W : (b == EscalatorUpMeta) ? EscalatorUp1W : EscalatorDown1W);
                            set_block_at(level, x-1, y, z, (b == StairsMeta) ? Stairs2W : (b == EscalatorUpMeta) ? EscalatorUp2W : EscalatorDown2W);
                            set_block_at(level, x-2, y, z, (b == StairsMeta) ? Stairs3W : (b == EscalatorUpMeta) ? EscalatorUp3W : EscalatorDown3W);
                            set_block_at(level, x-3, y, z, (b == StairsMeta) ? Stairs4W : (b == EscalatorUpMeta) ? EscalatorUp4W : EscalatorDown4W);
                        }
                    }



                }
            }

        }
    }
}

void make_level_str(PermanentState * permanent, LevelData *level, World_Size dimensions,  char * str){
    read_level_str(permanent, level, dimensions, str);
    add_stairs(level);
}

void make_level(PermanentState * permanent, LevelData *level,  const char * path){
    if (exists(path)) {
        World_Size dimensions = validate_and_get_dimensions(path);
        read_level(permanent, level, dimensions,  path);
        add_stairs(level);
    } else {
        printf("file not found!\n");
    }
}
