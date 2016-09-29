
#include <stdio.h>
#include "memory.h"
#include "renderer.h"
#include "random.h"
#include "pathfind.h"
#include "states.h"
#include "level.h"

#define SORT_NAME Actor
#define SORT_TYPE Actor
#define SORT_CMP(b, a) ((((a).y * 16384) - (a).z) - (((b).y * 16384) - (b).z))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "sort.h"
#pragma GCC diagnostic pop


void game_update_and_render(Memory* memory,  RenderState *renderer, float last_frame_time_seconds, const u8 *keys, SDL_Event e);

internal int wallsortfunc (const void * a, const void * b)
{
    //1536 = some guestimate, assuming the depth is maximum 128.
    // and the height of each block is 128
    const Wall *a2 = (const Wall *) a;
    const Wall *b2 = (const Wall *) b;
    return ( ( b2->y*16384 - b2->z) - (a2->y*16384 -  a2->z));
}

internal void set_actor_batch_sizes(PermanentState *permanent, RenderState *renderer) {
    u32 used_batches = ceil(permanent->actor_count / (MAX_IN_BUFFER * 1.0f));
    renderer->used_actor_batches = used_batches;

    if (used_batches == 1) {
        renderer->actors[0].count = permanent->actor_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches - 1; i++) {
            renderer->actors[i].count = MAX_IN_BUFFER;
        }
        renderer->actors[used_batches - 1].count = permanent->actor_count % MAX_IN_BUFFER;
    } else {
        renderer->used_actor_batches = 0;
    }
}

internal void set_wall_batch_sizes(PermanentState *permanent, RenderState *renderer) {
    u32 used_batches = ceil(permanent->wall_count / 2048.0f);
    renderer->used_wall_batches = used_batches;

    if (used_batches == 1) {
        renderer->walls[0].count = permanent->wall_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches-1; i++) {
            renderer->walls[i].count = 2048;
        }
        renderer->walls[used_batches-1].count = permanent->wall_count % 2048;
    } else {
        renderer->used_wall_batches = 0;
    }
}
internal void set_colored_line_batch_sizes(PermanentState *permanent, RenderState *renderer) {
    u32 used_batches = ceil(permanent->colored_line_count / (MAX_IN_BUFFER * 1.0f));
    renderer->used_colored_lines_batches = used_batches;

    if (used_batches == 1) {
        renderer->colored_lines[0].count = permanent->colored_line_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches - 1; i++) {
            renderer->colored_lines[i].count = MAX_IN_BUFFER;
        }
        renderer->colored_lines[used_batches - 1].count = permanent->colored_line_count % MAX_IN_BUFFER;
    } else {
        renderer->used_colored_lines_batches = 0;
    }
}


extern void game_update_and_render(Memory* memory, RenderState *renderer, float last_frame_time_seconds, const u8 *keys, SDL_Event e) {
    UNUSED(keys);
    UNUSED(e);
    ASSERT(sizeof(PermanentState) <= memory->permanent_size);
    PermanentState *permanent = (PermanentState *)memory->permanent;
    ASSERT(sizeof(ScratchState) <= memory->scratch_size);
    ScratchState *scratch = (ScratchState *)memory->scratch;
    ASSERT(sizeof(DebugState) <= memory->debug_size);
    DebugState *debug = (DebugState *)memory->debug;

    if (memory->is_initialized == false) {
        printf("Used at permanent:  %lu\n", permanent->arena.used);
        permanent->walls = (Wall*) PUSH_ARRAY(&permanent->arena, (16384), Wall);
        permanent->actors = (Actor*) PUSH_ARRAY(&permanent->arena, (16384*4), Actor);
        permanent->glyphs = (Glyph*) PUSH_ARRAY(&permanent->arena, (16384), Glyph);
        permanent->colored_lines = (ColoredLine*) PUSH_ARRAY(&permanent->arena, (16384), ColoredLine);

        printf("used scrathc space (before init): %lu\n", scratch->arena.used);
        printf("Used at permanent:  %lu\n", permanent->arena.used);
        BlockTextureAtlasPosition texture_atlas_data[BlockTotal];
        printf("blocktotal %d\n", BlockTotal);

        texture_atlas_data[Floor]       =  (BlockTextureAtlasPosition){0,0};
        texture_atlas_data[WallBlock]   =  (BlockTextureAtlasPosition){1,0};
        texture_atlas_data[LadderUpDown]=  (BlockTextureAtlasPosition){2,0};
        texture_atlas_data[LadderUp]    =  (BlockTextureAtlasPosition){5,1};
        texture_atlas_data[LadderDown]  =  (BlockTextureAtlasPosition){6,1};
        texture_atlas_data[StairsUp1N]  =  (BlockTextureAtlasPosition){3,0};
        texture_atlas_data[StairsUp2N]  =  (BlockTextureAtlasPosition){4,0};
        texture_atlas_data[StairsUp3N]  =  (BlockTextureAtlasPosition){5,0};
        texture_atlas_data[StairsUp4N]  =  (BlockTextureAtlasPosition){6,0};
        texture_atlas_data[StairsUp1S]  =  (BlockTextureAtlasPosition){7,0};
        texture_atlas_data[StairsUp2S]  =  (BlockTextureAtlasPosition){8,0};
        texture_atlas_data[StairsUp3S]  =  (BlockTextureAtlasPosition){9,0};
        texture_atlas_data[StairsUp4S]  =  (BlockTextureAtlasPosition){10,0};
        texture_atlas_data[StairsUp1E]  =  (BlockTextureAtlasPosition){11,0};
        texture_atlas_data[StairsUp2E]  =  (BlockTextureAtlasPosition){12,0};
        texture_atlas_data[StairsUp3E]  =  (BlockTextureAtlasPosition){13,0};
        texture_atlas_data[StairsUp4E]  =  (BlockTextureAtlasPosition){14,0};
        texture_atlas_data[StairsUp1W]  =  (BlockTextureAtlasPosition){15,0};
        texture_atlas_data[StairsUp2W]  =  (BlockTextureAtlasPosition){16,0};
        texture_atlas_data[StairsUp3W]  =  (BlockTextureAtlasPosition){17,0};
        texture_atlas_data[StairsUp4W]  =  (BlockTextureAtlasPosition){18,0};
        texture_atlas_data[Shaded]      =  (BlockTextureAtlasPosition){19,0};

        ////

        int used_wall_block =0;
        int count_shadow = 0;
        int used_floors = 0;
        int used_walls = 0;
        int wall_count =0;



        int simplecount =0;

        //printf("level %d,%d,%d \n",permanent->dims.x,permanent->dims.y,permanent->dims.z_level);
        for (u32 z = 0; z < permanent->dims.z_level ; z++){
            for (u32 y = 0; y< permanent->dims.y; y++){
                for (u32 x = 0; x< permanent->dims.x; x++){
                    WorldBlock *b = &permanent->level.blocks[FLATTEN_3D_INDEX(x,y,z,permanent->dims.x, permanent->dims.y)];
                    if (b->object == Nothing){

                    } else {
                        simplecount++;
                    }
                    switch (b->object){
                    case Floor:
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object];
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
                        used_floors++;
                        break;
                    case WallBlock:
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object];
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
                        used_walls++;
                        wall_count++;
                        break;
                    case LadderUpDown:
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object];
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
                        break;
                    case LadderUp:
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object];
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
                        break;
                    case LadderDown:
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object];
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
                        break;
                    case StairsUp1N:
                    case StairsUp1E:
                    case StairsUp1S:
                    case StairsUp1W:
                    case StairsUp2N:
                    case StairsUp2E:
                    case StairsUp2S:
                    case StairsUp2W:
                    case StairsUp3N:
                    case StairsUp3E:
                    case StairsUp3S:
                    case StairsUp3W:
                    case StairsUp4N:
                    case StairsUp4E:
                    case StairsUp4S:
                    case StairsUp4W:
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object];
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
                        break;
                    default:
                        //c = b;
                        //ASSERT("Problem!" && false);
                        break;
                    }
                        /*
                    if (b->object == Floor){
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
                        used_floors++;
                    }

                    if (b->object == WallBlock){
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
                        used_walls++;
                        wall_count++;
                    }


                    if (b->object == StairsUp1N || b->object == StairsUp2N || b->object == StairsUp3N || b->object == StairsUp4N) {
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;

                    }
                    if (b->object == StairsUp1S || b->object == StairsUp2S || b->object == StairsUp3S || b->object == StairsUp4S) {
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;

                    }
                    if (b->object == StairsUp1E || b->object == StairsUp2E || b->object == StairsUp3E || b->object == StairsUp4E) {
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;

                    }
                    if (b->object == StairsUp1W || b->object == StairsUp2W || b->object == StairsUp3W || b->object == StairsUp4W) {
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
                    }


                    if (b->object == Ladder){
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
                    }
                    // Shadow
                    WorldBlock *one_above = &permanent->level.blocks[FLATTEN_3D_INDEX(x,y,z+1 ,permanent->dims.x, permanent->dims.y)];

                    if (one_above->object == Floor && z+1 < permanent->dims.z_level){
                        count_shadow++;
                        permanent->walls[used_wall_block].frame = texture_atlas_data[Shaded].x_pos;
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = y * permanent->block_size.y;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
                    }
                        */
                }
            }
        }
        printf("simple block count : %d, floors:%d, walls:%d\n",simplecount, used_floors, used_walls);
        permanent->wall_count = used_wall_block;
        qsort(permanent->walls, used_wall_block, sizeof(Wall), wallsortfunc);

        //set_wall_batch_sizes(permanent, renderer);

        printf("wall count: %d used wall block:%d \n", permanent->wall_count, used_wall_block);
        set_wall_batch_sizes(permanent, renderer);
        renderer->needs_prepare = 1;
        //prepare_renderer(permanent, renderer);

        for (u32 i = 0; i < 1; i++) {
            permanent->actors[i].x = rand_int(permanent->dims.x) * permanent->block_size.x;
            permanent->actors[i].y = rand_int(permanent->dims.y) * permanent->block_size.y;
            permanent->actors[i].z = rand_int(0) * permanent->block_size.z_level;
            permanent->actors[i].frame = rand_int(4);
            float speed = 1; //10 + rand_int(10); // px per seconds
            permanent->actors[i].dx = rand_bool() ? -1 * speed : 1 * speed;
            permanent->actors[i].dy = rand_bool() ? -1 * speed : 1 * speed;
            permanent->actors[i].palette_index = rand_float();
        }

        set_actor_batch_sizes(permanent, renderer);

        /* for (u32 i = 0; i< 20; i++) { */
        /*     permanent->colored_lines[i].x1 = 0; */
        /*     permanent->colored_lines[i].y1 = i * 20;; */
        /*     permanent->colored_lines[i].z1 = 0; */
        /*     permanent->colored_lines[i].x2 = 100; */
        /*     permanent->colored_lines[i].y2 = i * 20; */
        /*     permanent->colored_lines[i].z2 = 0; */
        /*     permanent->colored_lines[i].r = 1.0f; */
        /*     permanent->colored_lines[i].g = 0; */
        /*     permanent->colored_lines[i].b = i % 10 == 0 ? 1.0f : 0; */
        /* } */
        /* permanent->colored_line_count = 20; */


        permanent->grid = PUSH_STRUCT(&permanent->arena, Grid);
        // TODO memory for the grid needs to be in the permanent space, currently I am using scratch
#if 1
        init_grid(permanent->grid, &permanent->arena, &permanent->level);
        preprocess_grid(permanent->grid);
        printf("used scratch space: %lu\n", scratch->arena.used);
        TempMemory temp_mem = begin_temporary_memory(&scratch->arena);

        grid_node * Start = GetNodeAt(permanent->grid, 4, 5, 3);
        grid_node * End = GetNodeAt(permanent->grid, 15, 6, 0);


        ASSERT(Start->walkable);
        ASSERT(End->walkable);

        path_list * PathRaw = FindPathPlus(Start, End, permanent->grid, &scratch->arena);
        path_list *Path = NULL;
        if (PathRaw) {
            printf("Smoothing path!\n");
            Path = SmoothenPath(PathRaw,  &scratch->arena, permanent->grid);
        }
        //path_list * Path = ExpandPath(PathSmooth, &scratch->arena);
        //path_list * Path = NULL;

        u64 before = SDL_GetPerformanceCounter();
        for (int i = 0; i < permanent->grid->width * permanent->grid->height * permanent->grid->depth;i++) {
            permanent->grid->nodes[i].f = 0;
            permanent->grid->nodes[i].g = 0;
            permanent->grid->nodes[i].opened = 0;
            permanent->grid->nodes[i].closed = 0;
            permanent->grid->nodes[i].Next = NULL;
            permanent->grid->nodes[i].parent = NULL;
        }
        printf("cleaning took: %f.\n", ((SDL_GetPerformanceCounter() - before)/SDL_GetPerformanceFrequency() )*1000.0f);

        //PathRaw = FindPathPlus(Start, End, permanent->grid, &scratch->arena);
        //Path = SmoothenPath(PathRaw,  &scratch->arena, permanent->grid);
        //printf("used scratch space: %lu\n", scratch->arena.used);

        if (Path) {
            u32 i = 0;

            path_node * done= Path->Sentinel->Next;
            while (done->Next != Path->Sentinel) {
                permanent->colored_lines[i].x1 = done->X * permanent->block_size.x;
                permanent->colored_lines[i].y1 = done->Y * permanent->block_size.y;
                permanent->colored_lines[i].z1 = done->Z * permanent->block_size.z_level;
                permanent->colored_lines[i].x2 = done->Next->X * permanent->block_size.x;;
                permanent->colored_lines[i].y2 = done->Next->Y * permanent->block_size.y;;
                permanent->colored_lines[i].z2 = done->Next->Z * permanent->block_size.z_level;;
                permanent->colored_lines[i].r = 0.0f;
                permanent->colored_lines[i].g = 0.0f;
                permanent->colored_lines[i].b = 0.0f;
                done = done->Next;
                i++;
            }
            permanent->colored_line_count = i;
        }

        end_temporary_memory(temp_mem);
        printf("used scratch space: %lu\n", scratch->arena.used);

        set_colored_line_batch_sizes(permanent, renderer);
#endif
        memory->is_initialized = true;
    }


    BEGIN_PERFORMANCE_COUNTER(actors_update);
    // TODO: plenty of bugs are in this loop, never really cleaned up after
    for (u32 i = 0; i < permanent->actor_count; i++) {
        if (permanent->actors[i].x <= 0 || permanent->actors[i].x >= ((permanent->dims.x - 1) * permanent->block_size.x)) {
            if (permanent->actors[i].x < 0) {
                permanent->actors[i].x = 0;
            }
            if (permanent->actors[i].x >= ((permanent->dims.x - 1) * permanent->block_size.x)) {
                permanent->actors[i].x = ((permanent->dims.x - 1) * permanent->block_size.x);
            }

            permanent->actors[i].dx *= -1.0f;
        }
        permanent->actors[i].x += permanent->actors[i].dx * (last_frame_time_seconds);

        if (permanent->actors[i].y <= 0 || permanent->actors[i].y >= ((permanent->dims.y - 1) * permanent->block_size.y)) {
            if (permanent->actors[i].z < 0) {
                permanent->actors[i].z = 0;
            }
            if (permanent->actors[i].y > ((permanent->dims.y - 1) * permanent->block_size.y)) {
                permanent->actors[i].y = ((permanent->dims.y - 1) * permanent->block_size.y);
            }

            permanent->actors[i].dy *= -1.0f;
        }
        permanent->actors[i].y += permanent->actors[i].dy * (last_frame_time_seconds);
        //printf("is it a float: %f \n", permanent->actors[i].y);
    }
    END_PERFORMANCE_COUNTER(actors_update);

    BEGIN_PERFORMANCE_COUNTER(actors_sort);
    //    qsort, timsort, quick_sort
    //64k 7.3    4.8      3.7
    //qsort(permanent->actors,  permanent->actor_count, sizeof(Actor), actorsortfunc);
    Actor_quick_sort(permanent->actors, permanent->actor_count);
    //Actor_tim_sort(game->actors,  game->actor_count);
    END_PERFORMANCE_COUNTER(actors_sort);

}
