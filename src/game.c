
#include <stdio.h>
#include "memory.h"
#include "renderer.h"
#include "random.h"
#include "pathfind.h"
#include "states.h"
#include "level.h"
#include "data_structures.h"


#define SORT_NAME Actor
#define SORT_TYPE Actor
//#define SORT_CMP(b, a) ((((a).y * 16384) - (a).z) - (((b).y * 16384) - (b).z))
#define SORT_CMP(b, a) ((((a)._location.y * 16384) - (a)._location.z) - (((b)._location.y * 16384) - (b)._location.z))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Warray-bounds"

#include "sort.h"
#pragma GCC diagnostic pop


internal int path_length_at_index(PermanentState *permanent, int i) {
    Node16 * fp = permanent->paths[i].Sentinel;
    int fp_count = 0;
    while(fp->Next != permanent->paths[i].Sentinel) {
        fp_count++;
        fp = fp->Next;
    }
    return fp_count;
}

void game_update_and_render(Memory* memory,  RenderState *renderer, float last_frame_time_seconds, const u8 *keys, SDL_Event e);

internal int wallsortfunc (const void * a, const void * b)
{
    //1536 = some guestimate, assuming the depth is maximum 128.
    // and the height of each block is 128
    const Wall *a2 = (const Wall *) a;
    const Wall *b2 = (const Wall *) b;

    // TODO maybe I need a special batch of tranparant things that are drawn back to front, so the rest can be faster
    // TODO heyhey the back to front order shows the same artifacts i see when trying to get actors drawn on top of all floors
    return (  (a2->y*16384 + a2->z ) - ( b2->y*16384 + b2->z));
    //return (  (a2->y*16384 -  a2->z) - ( b2->y*16384 - b2->z)); // this sorts walls back to front (needed for transparancy)
    //return ( ( b2->y*16384 - b2->z) - (a2->y*16384 -  a2->z)); //// this sorts walls front to back (much faster rendering)
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


internal grid_node* get_random_walkable_node(Grid *grid) {
    grid_node *result;// = GetNodeAt(grid, 0,0,0);
    do {
        result = GetNodeAt(grid, rand_int(grid->width), rand_int(grid->height), rand_int(grid->depth));
    } while (!result->walkable);
    return result;
}


extern void game_update_and_render(Memory* memory, RenderState *renderer, float last_frame_time_seconds, const u8 *keys, SDL_Event e) {
    UNUSED(keys);
    UNUSED(e);
    UNUSED(last_frame_time_seconds);
    ASSERT(sizeof(PermanentState) <= memory->permanent_size);
    PermanentState *permanent = (PermanentState *)memory->permanent;
    ASSERT(sizeof(ScratchState) <= memory->scratch_size);
    ScratchState *scratch = (ScratchState *)memory->scratch;
    ASSERT(sizeof(DebugState) <= memory->debug_size);
    DebugState *debug = (DebugState *)memory->debug;
    ASSERT(sizeof(Node16Arena) <= memory->node16_size);
    Node16Arena *node16 = (Node16Arena *)memory->node16;

    ASSERT(sizeof(Node16)  == sizeof(Node16*)*2 + 16);
    ASSERT(sizeof(FoundPathNode) == sizeof(BarNode));

    if (memory->is_initialized == false) {
        //printf("Used at permanent:  %lu\n", (unsigned long) permanent->arena.used);
        permanent->walls = (Wall*) PUSH_ARRAY(&permanent->arena, (16384), Wall);
        permanent->actors = (Actor*) PUSH_ARRAY(&permanent->arena, (16384*4), Actor);
        permanent->paths = (ActorPath*) PUSH_ARRAY(&permanent->arena, (16384*4), ActorPath);
        permanent->steer_data = (ActorSteerData*) PUSH_ARRAY(&permanent->arena, (16384*4), ActorSteerData);
        permanent->anim_data = (ActorAnimData*) PUSH_ARRAY(&permanent->arena, (16384*4), ActorAnimData);
        for (int i = 0; i < 16384*4; i++) {
            Node16 *Sentinel = (Node16 *) PUSH_STRUCT(&node16->arena, Node16);
            permanent->paths[i].Sentinel = Sentinel;
            permanent->paths[i].Sentinel->Next = Sentinel;
            permanent->paths[i].Sentinel->Prev = Sentinel;
            permanent->actors[i].index = i;
            permanent->paths[i].Sentinel->path.node = GLKVector3Make(-999,-999,-999);
        }
        //node16->Sentinel = PUSH_STRUCT(&node16->arena, Node16);
        //node16->Sentinel->Next =  node16->Sentinel;
        //node16->Sentinel->Prev =  node16->Sentinel;

        node16->Free = PUSH_STRUCT(&node16->arena, Node16);
        node16->Free->Next = node16->Free;//node16->Sentinel;
        node16->Free->Prev = node16->Free;//node16->Sentinel;
        printf("Node16 used: %lu\n", node16->arena.used);
        permanent->glyphs = (Glyph*) PUSH_ARRAY(&permanent->arena, (16384), Glyph);
        permanent->colored_lines = (ColoredLine*) PUSH_ARRAY(&permanent->arena, (16384), ColoredLine);

        //printf("used scrathc space (before init): %lu\n", (unsigned long)scratch->arena.used);
        //printf("Used at permanent:  %lu\n", (unsigned long)permanent->arena.used);
        BlockTextureAtlasPosition texture_atlas_data[BlockTotal];
        //printf("blocktotal %d\n", BlockTotal);

        texture_atlas_data[Floor]       =  (BlockTextureAtlasPosition){0,0};
        texture_atlas_data[WallBlock]   =  (BlockTextureAtlasPosition){1,0};
        texture_atlas_data[LadderUpDown]=  (BlockTextureAtlasPosition){2,0};
        texture_atlas_data[LadderUp]    =  (BlockTextureAtlasPosition){5,1};
        texture_atlas_data[WindowBlock]    =  (BlockTextureAtlasPosition){7,1};
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
        //int count_shadow = 0;
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
                    permanent->walls[used_wall_block].is_floor = 0;
                    switch (b->object){
                    case Floor:
                        permanent->walls[used_wall_block].frame = texture_atlas_data[b->object];
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = (y * permanent->block_size.y) ;
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        permanent->walls[used_wall_block].is_floor = 1;
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
                    case WindowBlock:
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
                    case Grass:
                    case Wood:
                    case Concrete:
                    case Nothing:
                    case Tiles:
                    case Carpet:
                    case StairsUpMeta:
                    case StairsDownMeta:
                    case StairsFollowUpMeta:
                    case Shaded:
                    case BlockTotal:

                    default:
                        //c = b;
                        //ASSERT("Problem!" && false);
                        break;
                    }

                    WorldBlock *one_above = &permanent->level.blocks[FLATTEN_3D_INDEX(x,y,z+1 ,permanent->dims.x, permanent->dims.y)];

                    if (one_above->object == Floor && z+1 < permanent->dims.z_level ){
                        //count_shadow++;
                        permanent->walls[used_wall_block].frame = texture_atlas_data[Shaded];
                        permanent->walls[used_wall_block].x = x * permanent->block_size.x;
                        permanent->walls[used_wall_block].y = (y * permanent->block_size.y)-4; //??
                        permanent->walls[used_wall_block].z = z * permanent->block_size.z_level;
                        used_wall_block++;
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
        //printf("simple block count : %d, floors:%d, walls:%d\n",simplecount, used_floors, used_walls);
        permanent->wall_count = used_wall_block;
        qsort(permanent->walls, used_wall_block, sizeof(Wall), wallsortfunc);

        //set_wall_batch_sizes(permanent, renderer);

        //printf("wall count: %d used wall block:%d \n", permanent->wall_count, used_wall_block);
        set_wall_batch_sizes(permanent, renderer);
        renderer->needs_prepare = 1;
        //prepare_renderer(permanent, renderer);

        for (u32 i = 0; i < 1; i++) {
            permanent->steer_data[i].location.x = rand_int(permanent->dims.x) * permanent->block_size.x;
            permanent->steer_data[i].location.y = rand_int(permanent->dims.y) * permanent->block_size.y;
            permanent->steer_data[i].location.z = rand_int(0) * permanent->block_size.z_level;
            permanent->anim_data[i].frame = rand_int(4);
            float speed = 1; //10 + rand_int(10); // px per seconds
            permanent->steer_data[i].dx = rand_bool() ? -1 * speed : 1 * speed;
            permanent->steer_data[i].dy = rand_bool() ? -1 * speed : 1 * speed;
            permanent->anim_data[i].palette_index = rand_float();
        }

        set_actor_batch_sizes(permanent, renderer);

        permanent->grid = PUSH_STRUCT(&permanent->arena, Grid);
        // TODO memory for the grid needs to be in the permanent space, currently I am using scratch
#if 1
        init_grid(permanent->grid, &permanent->arena, &permanent->level);
        preprocess_grid(permanent->grid);
        set_colored_line_batch_sizes(permanent, renderer);
#endif
        memory->is_initialized = true;
    }


#if 1
    {
        permanent->colored_line_count = 0;
        for (u32 i = 0; i < permanent->actor_count; i++) {

#if 1
            if (permanent->paths[i].Sentinel->Next != permanent->paths[i].Sentinel) {
                permanent->paths[i].counter--;

                if (permanent->paths[i].counter <=0) {
                    Node16 *First = permanent->paths[i].Sentinel->Next;
                    permanent->paths[i].Sentinel->Next = First->Next;
                    First->Next = node16->Free->Next;
                    node16->Free->Next = First;
                    First->Prev = node16->Free;
                    permanent->paths[i].counter = 10; // this is faking some time it takes to get to the next node
                }
            }

            // here i should peek at the first node in permanent->paths[i].Sentinel->Next->path.node
            // thats the position i want to steer towards
            // here is where i shoudl put that colored debug line drawing, it'll come in handy still
            if (permanent->paths[i].Sentinel->Next != permanent->paths[i].Sentinel) {
                Node16 * d= permanent->paths[i].Sentinel->Next;
                u32 c = permanent->colored_line_count;
                while (d->Next != permanent->paths[i].Sentinel){

                    permanent->colored_lines[c].x1 = d->path.node.x;
                    permanent->colored_lines[c].y1 = d->path.node.y;
                    permanent->colored_lines[c].z1 = d->path.node.z;
                    permanent->colored_lines[c].x2 = d->Next->path.node.x;
                    permanent->colored_lines[c].y2 = d->Next->path.node.y;
                    permanent->colored_lines[c].z2 = d->Next->path.node.z;
                    permanent->colored_lines[c].r = 0.0f;
                    permanent->colored_lines[c].g = 0.0f;
                    permanent->colored_lines[c].b = 0.0f;
                    d = d->Next;
                    c++;
                }
                permanent->colored_line_count = c;
            }
            ASSERT( permanent->colored_line_count < LINE_BATCH_COUNT * MAX_IN_BUFFER)
#endif
            //
            //printf("line count: %d\n",permanent->colored_line_count);




            // if already have a path, i dont need a new one,
            // TODO somehow this makes all my paths appear unsmoothed ;) ??
            if (permanent->paths[i].Sentinel->Next != permanent->paths[i].Sentinel) {
                continue;
            }


            BEGIN_PERFORMANCE_COUNTER(mass_pathfinding);
            TempMemory temp_mem = begin_temporary_memory(&scratch->arena);
            grid_node * Start = GetNodeAt(permanent->grid,4,5,0);//  get_random_walkable_node(permanent->grid);
            grid_node * End =  GetNodeAt(permanent->grid,5,10,0);//get_random_walkable_node(permanent->grid);
            ASSERT(Start->walkable);
            ASSERT(End->walkable);

            path_list * PathRaw = FindPathPlus(Start, End, permanent->grid, &scratch->arena);
            path_list *Path = NULL;



            if (PathRaw) {
                //printf("Smoothing path!\n");
                Path = SmoothenPath(PathRaw,  &scratch->arena, permanent->grid);
                //Path = PathRaw;



#if 0
                if (Path) {
                    u32 path_length = 0;
                    u32 c = permanent->colored_line_count;

                    path_node * done= Path->Sentinel->Next;
                    while (done->Next != Path->Sentinel) {
                        permanent->colored_lines[c].x1 = done->X * permanent->block_size.x;
                        permanent->colored_lines[c].y1 = done->Y * permanent->block_size.y;
                        permanent->colored_lines[c].z1 = done->Z * permanent->block_size.z_level;
                        permanent->colored_lines[c].x2 = done->Next->X * permanent->block_size.x;;
                        permanent->colored_lines[c].y2 = done->Next->Y * permanent->block_size.y;;
                        permanent->colored_lines[c].z2 = done->Next->Z * permanent->block_size.z_level;;
                        permanent->colored_lines[c].r = 0.0f;
                        permanent->colored_lines[c].g = 0.0f;
                        permanent->colored_lines[c].b = 0.0f;
                        done = done->Next;
                        c++;
                        path_length++;
                    }
                    permanent->colored_line_count = c;
                }
#endif




#if 1
                if (Path) {

                    // this is walking backwards, the path is in a dlist so its fine, i might rewrite this to work forward, but this works correct (forward doesnt, it trips over some freelist madness, TODO: investigate it)
                    path_node * done= Path->Sentinel->Prev;
                    while (done != Path->Sentinel) {

                        // this block tries to reuse exitsing nodes on freelistm, if none are there make a new one
                        Node16 *N = NULL;
                        if ( node16->Free->Next !=  node16->Free) {
                            N = node16->Free->Next;
                            node16->Free->Next = N->Next;
                            node16->Free->Next->Prev = node16->Free;
                        } else {
                            N = PUSH_STRUCT(&node16->arena, Node16);
                        }

                        ActorPath * p = &(permanent->paths[i]);
                        N->path.node.x = done->X* permanent->block_size.x;
                        N->path.node.y = done->Y* permanent->block_size.y;
                        N->path.node.z = done->Z* permanent->block_size.z_level;

                        DLIST_ADDFIRST(p, N);
                        done = done->Prev;
                    }
                }
#endif
            }
            END_PERFORMANCE_COUNTER(mass_pathfinding);
            //path_list * Path = ExpandPath(PathSmooth, &scratch->arena);
            //path_list * Path = NULL;
            BEGIN_PERFORMANCE_COUNTER(grid_cleaning);
            //u64 before = SDL_GetPerformanceCounter();
            for (int i = 0; i < permanent->grid->width * permanent->grid->height * permanent->grid->depth;i++) {
                permanent->grid->nodes[i].f = 0;
                permanent->grid->nodes[i].g = 0;
                permanent->grid->nodes[i].opened = 0;
                permanent->grid->nodes[i].closed = 0;
                permanent->grid->nodes[i].Next = NULL;
                permanent->grid->nodes[i].parent = NULL;
            }
            END_PERFORMANCE_COUNTER(grid_cleaning);

            end_temporary_memory(temp_mem);

        }
        //printf("colored lines: %d\n", permanent->colored_line_count);
        set_colored_line_batch_sizes(permanent, renderer);
        //printf("Node16 used: %lu \n", node16->arena.used );
    }
#endif


    BEGIN_PERFORMANCE_COUNTER(actors_update);
    // TODO: plenty of bugs are in this loop, never really cleaned up after

    for (u32 i = 0; i < permanent->actor_count; i++) {
        if (permanent->steer_data[i].location.x <= 0 || permanent->steer_data[i].location.x >= ((permanent->dims.x - 1) * permanent->block_size.x)) {
            if (permanent->steer_data[i].location.x < 0) {
                permanent->steer_data[i].location.x = 0;
            }
            if (permanent->steer_data[i].location.x >= ((permanent->dims.x - 1) * permanent->block_size.x)) {
                permanent->steer_data[i].location.x = ((permanent->dims.x - 1) * permanent->block_size.x);
            }

            permanent->steer_data[i].dx *= -1.0f;
        }
        permanent->steer_data[i].location.x += permanent->steer_data[i].dx * (last_frame_time_seconds);

        if (permanent->steer_data[i].location.y <= 0 || permanent->steer_data[i].location.y >= ((permanent->dims.y - 1) * permanent->block_size.y)) {
            if (permanent->steer_data[i].location.z < 0) {
                permanent->steer_data[i].location.z = 0;
            }
            if (permanent->steer_data[i].location.y > ((permanent->dims.y - 1) * permanent->block_size.y)) {
                permanent->steer_data[i].location.y = ((permanent->dims.y - 1) * permanent->block_size.y);
            }

            permanent->steer_data[i].dy *= -1.0f;
        }
        permanent->steer_data[i].location.y += permanent->steer_data[i].dy * (last_frame_time_seconds);
    }
    BEGIN_PERFORMANCE_COUNTER(actors_data_gathering);

    // TODO for now this suffices, its the easiest and not TOO bad, but i loose 3ms for 64k actors on sorting this way
    // what happens like this is the data is not sorted at all and then i sort it every frame.
    // the solution (I think) would be using some index, so i can leave the renderable data sorted as is, and gather from the correct places.
    // it costs a little in cache misses when gathering, but not as much as the sort is costing more now.
    // maybe as an alternative I can look into using a separate thread to sort this data on, and flip flop data.
    // btw not sorting only costs 10ms per frame (@64k) vs 17/18, but i think the sorting is specifically desired on rpi (TODO TEST THIS AGAiN)


    for (u32 i = 0; i < permanent->actor_count; i++) {
        //printf("index: %d\n",permanent->actors[i].index);

        //permanent->actors[i]._location = permanent->steer_data[permanent->actors[i].index].location;
        //permanent->actors[i]._palette_index = permanent->anim_data[permanent->actors[i].index].palette_index;
        //permanent->actors[i]._frame = permanent->anim_data[permanent->actors[i].index].frame;

        permanent->actors[i]._location = permanent->steer_data[i].location;//
        permanent->actors[i]._palette_index = permanent->anim_data[i].palette_index;//
        permanent->actors[i]._frame = permanent->anim_data[i].frame;//

    }

    END_PERFORMANCE_COUNTER(actors_data_gathering);
    END_PERFORMANCE_COUNTER(actors_update);

    BEGIN_PERFORMANCE_COUNTER(actors_sort);
    //    qsort, timsort, quick_sort
    //64k 7.3    4.8      3.7
    //qsort(permanent->actors,  permanent->actor_count, sizeof(Actor), actorsortfunc);
    //TODO later on when I use binning for steering I might be able to improve sorting
    Actor_quick_sort(permanent->actors, permanent->actor_count);
    //Actor_sqrt_sort(permanent->actors, permanent->actor_count);
    //Actor_tim_sort(permanent->actors,  permanent->actor_count);
    END_PERFORMANCE_COUNTER(actors_sort);

}
