#include "SDL.h"
#include "SDL_mixer.h"

#include "types.h"
#include "renderer.h"
#include "memory.h"
#include "random.h"

#include <math.h>




void (*func)(void);

extern RenderState *renderer;
extern GameState *game;

#define SDL_ASSERT(expression)                                                                                               \
    if (!(expression)) {                                                                                                     \
        printf("%s, %s, function %s, file: %s, line:%d. \n", #expression, SDL_GetError(), __FUNCTION__, __FILE__, __LINE__); \
        exit(0);                                                                                                             \
    }
#define SDL_MIX_ASSERT(expression)                                                                                           \
    if (!(expression)) {                                                                                                     \
        printf("%s, %s, function %s, file: %s, line:%d. \n", #expression, Mix_GetError(), __FUNCTION__, __FILE__, __LINE__); \
        exit(0);                                                                                                             \
    }



internal void initialize_SDL(void) {
    int error = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
    if (error < 0) {
        printf("%d %s\n", error, SDL_GetError());
    }
    SDL_MIX_ASSERT(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) >= 0);
    SDL_DisplayMode displayMode;
    SDL_GetDesktopDisplayMode(0, &displayMode);

#ifdef GLES
    renderer->view.width = displayMode.w;
    renderer->view.height = displayMode.h;
    SDL_Log("%d,%d\n", renderer->view.width, renderer->view.height);
#endif
#ifdef GL3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    renderer->window = SDL_CreateWindow("Work in progress",
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SDL_WINDOWPOS_UNDEFINED,
                                      renderer->view.width, renderer->view.height,
                                      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    SDL_ASSERT(renderer->window != NULL);

    renderer->context = SDL_GL_CreateContext(renderer->window);
    SDL_ASSERT(renderer->context != NULL);

#ifndef IOS

    SDL_ASSERT(SDL_GL_SetSwapInterval(0) >= 0);
#endif
}


internal int event_filter(void *userData, SDL_Event *event) {
    UNUSED(userData);
    switch (event->type) {
        case SDL_FINGERMOTION:
            SDL_Log("SDL_FINGERMOTION");
            return 0;
        case SDL_FINGERDOWN:
            SDL_Log("SDL_FINGERDOWN");
            return 0;
        case SDL_FINGERUP:
            SDL_Log("SDL_FINGERUP");
            return 0;
    }
    return 1;
}


internal void load_resources(void) {
    resource_level(&renderer->assets.level, "test4.txt");
    resource_sprite_atlas("out.sho");
    resource_font(&renderer->assets.menlo_font, "menlo.fnt");

    resource_texture(&renderer->assets.menlo, "menlo.tga");
    resource_texture(&renderer->assets.sprite, "Untitled4.tga");
    resource_texture(&renderer->assets.palette, "palette2.tga");

#ifdef GL3
    resource_shader(&renderer->assets.shader1, "gl330.vert", "gl330.frag");
#endif
#ifdef GLES
    resource_shader(&renderer->assets.shader1, "gles20.vert", "gles20.frag");
#endif

    resource_ogg(&renderer->assets.music1, "Stiekem.ogg");
    resource_wav(&renderer->assets.wav1, "scratch.wav");
}

internal void quit(void) {
    SDL_DestroyWindow(renderer->window);
    renderer->window = NULL;
    SDL_Quit();
}

internal void update_frame(void *param) {
    render((SDL_Window *)param);
}




// TODO generalise these three into a reusable function
internal void set_wall_batch_sizes(void) {
    u32 used_batches = ceil(game->wall_count / 2048.0f);
    renderer->used_wall_batches = used_batches;

    if (used_batches == 1) {
        renderer->walls[0].count = game->wall_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches-1; i++) {
            renderer->walls[i].count = 2048;
        }
        renderer->walls[used_batches-1].count = game->wall_count % 2048;
    } else {
        renderer->used_wall_batches = 0;
    }
}

internal void set_actor_batch_sizes(void) {
    u32 used_batches = ceil(game->actor_count / (MAX_IN_BUFFER*1.0f));
    renderer->used_actor_batches = used_batches;

    if (used_batches == 1) {
        renderer->actors[0].count = game->actor_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches-1; i++) {
            renderer->actors[i].count = MAX_IN_BUFFER;
        }
        renderer->actors[used_batches-1].count = game->actor_count % MAX_IN_BUFFER;
    } else {
        renderer->used_actor_batches = 0;
    }
}

internal void set_glyph_batch_sizes(void) {
    u32 used_batches = ceil(game->glyph_count / 2048.0f);
    renderer->used_glyph_batches = used_batches;

    if (used_batches == 1) {
        renderer->glyphs[0].count = game->glyph_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches-1; i++) {
            renderer->glyphs[i].count = 2048;
        }
        renderer->glyphs[used_batches-1].count = game->glyph_count % 2048;
    } else {
        renderer->used_glyph_batches = 0;
    }
}



internal void draw_glyph(u32 offset, u32 x, u32 y, u32 sx, u32 sy, u32 w, u32 h) {
    u32 base = game->glyph_count;
    Glyph * g =  &game->glyphs[base + offset];
    g->x = x;
    g->y = y;
    g->sx = sx;
    g->sy = sy;
    g->w = w;
    g->h = h;
}

internal u32 draw_text(char* str, u32 x, u32 y, BM_Font *font) {
    UNUSED(str);UNUSED(x);UNUSED(y);UNUSED(font);

    u32 currentY = y;
    u32 currentX = x;

    u32 drawn = 0;
    for (u32 i = 0; i < strlen(str); i++) {
        if (str[i] == 10) { // newline
            currentY += font->line_height;
            currentX = x;
            continue;
        }
        if (str[i] == 9) { // tab
            currentX += font->chars[32].xadvance * 4;
            continue;
        }
        if (str[i] == 32) { // space
            currentX += font->chars[32].xadvance;
            continue;
        }

        //printf("(%d, %d) %d\n", currentX, currentY, str[i]);
        currentX += font->chars[(u8)(str[i])].xadvance;
        BM_Glyph glyph = font->chars[(u8)(str[i])];

        draw_glyph(drawn, currentX+glyph.xoffset, currentY+glyph.yoffset, glyph.x, glyph.y, glyph.width, glyph.height);
        drawn++;
    }
    return drawn;
}

internal void center_view(void) {
    // TODO the Y offset is not correct, Keep in mind that drawing starts at the world 0,0,0 and goes up (y) and down (z)

    int real_world_width = game->dims.x * game->block_size.x;
    int real_world_depth = game->dims.y * game->block_size.y/2;
    int real_world_height = game->dims.z_level * game->block_size.z_level;

    s32 offset_x_blocks = (renderer->view.width - real_world_width) / 2;
    s32 offset_y_blocks = (renderer->view.height - (real_world_height+real_world_depth)) / 2;

    game->x_view_offset = offset_x_blocks;
    game->y_view_offset = real_world_depth + offset_y_blocks;

}


typedef struct {
    int x_pos;
    int y_pos;
} BlockTextureAtlasPosition;


internal int wallsortfunc (const void * a, const void * b)
{
    //1536 = some guestimate, assuming the depth is maximum 128.
    // and the height of each block is 128
    const Wall *a2 = (const Wall *) a;
    const Wall *b2 = (const Wall *) b;
    return ( ( b2->y*16384 - b2->z) - (a2->y*16384 -  a2->z));
}

internal int actorsortfunc (const void * a, const void * b)
{
    //1536 = some guestimate, assuming the depth is maximum 128.
    // and the height of each block is 128
    const Actor *a2 = (const Actor *) a;
    const Actor *b2 = (const Actor *) b;
    return ( ( b2->y*16384 - b2->z) - (a2->y*16384 -  a2->z));
}

Shared_Library libgame = {
    .handle = NULL,
    .name = "./gamelibrary.so",
    .creation_time = 0,
    .size = 0,
    .fn_name = "game_update_and_render"
};

internal void stub(void)
{
    SDL_Delay(1);
}

internal void maybe_load_libgame(void)
{
    stat(libgame.name, &libgame.stats);
    if (libgame.stats.st_ino != libgame.id){
        if ((intmax_t)libgame.stats.st_size > 0 && libgame.stats.st_nlink > 0) {
            libgame.id = libgame.stats.st_ino;
            if (libgame.handle) {
                SDL_UnloadObject(libgame.handle);
            }
            libgame.handle = SDL_LoadObject(libgame.name);
            if (!libgame.handle) {
                libgame.handle = NULL;
                libgame.id = 0;
                printf("couldnt load:%s, error: %s\n", libgame.name, SDL_GetError());
                func = stub;
            } else {
                func = (void (*)(void)) SDL_LoadFunction(libgame.handle, libgame.fn_name);
                if (func == NULL) {
                    printf("couldnt find: %s, error: %s\n",libgame.fn_name, SDL_GetError());
                } else {
                    printf("succes loading libgame timestamp: %d \n", SDL_GetTicks());
                }
            }
        }
    }
}





int main(int argc, char **argv) {
    printf("\n");

    BlockTextureAtlasPosition texture_atlas_data[BlockTotal];
    printf("blocktotal %d\n", BlockTotal);

    texture_atlas_data[Floor]       =  (BlockTextureAtlasPosition){0,0};
    texture_atlas_data[WallBlock]   =  (BlockTextureAtlasPosition){1,0};
    texture_atlas_data[Ladder]      =  (BlockTextureAtlasPosition){2,0};
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






    Memory _memory;
    Memory *memory = &_memory;
    reserve_memory(memory);
    ASSERT(sizeof(GameState) <= memory->permanent_size);
    GameState *storage = (GameState *)memory->permanent;
    ASSERT(sizeof(TransState) <= memory->scratch_size);
    TransState *trans = (TransState *) memory->scratch;
    initialize_arena(&storage->arena,
                     memory->permanent_size - sizeof(GameState),
                     (u8 *)memory->permanent + sizeof(GameState));

    initialize_arena(&trans->scratch_arena,
                     memory->scratch_size - sizeof(TransState),
                     (u8 *)memory->scratch + sizeof(TransState));

    memory->is_initialized = true;




    UNUSED(argc);
    UNUSED(argv);

    renderer->view.width = 1920; //1800;
    renderer->view.height = 900;

    initialize_SDL();
    initialize_GL();
    load_resources();


    game->dims = (WorldDims){renderer->assets.level.x,renderer->assets.level.y, renderer->assets.level.z_level};
    printf("dimensions: %d, %d, %d\n",game->dims.x, game->dims.y, game->dims.z_level);
    game->block_size = (WorldDims){24,24,96};

    center_view();







    int used_wall_block =0;
    int count_shadow = 0;
    int used_floors = 0;
    int wall_count =0;

    for (u32 z = 0; z < game->dims.z_level ; z++){
        for (u32 y = 0; y< game->dims.y; y++){
            for (u32 x = 0; x< game->dims.x; x++){
                //renderer->assets.level
                WorldBlock *b = &renderer->assets.level.blocks[FLATTEN_3D_INDEX(x,y,z,game->dims.x, game->dims.y)];

                if (b->object == Nothing){

                }
                if (b->object == StairsUp1N || b->object == StairsUp2N || b->object == StairsUp3N || b->object == StairsUp4N) {
                    game->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                    game->walls[used_wall_block].x = x * game->block_size.x;
                    game->walls[used_wall_block].y = y * game->block_size.y;
                    game->walls[used_wall_block].z = z * game->block_size.z_level;
                    used_wall_block++;

                }
                if (b->object == StairsUp1S || b->object == StairsUp2S || b->object == StairsUp3S || b->object == StairsUp4S) {
                    game->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                    game->walls[used_wall_block].x = x * game->block_size.x;
                    game->walls[used_wall_block].y = y * game->block_size.y;
                    game->walls[used_wall_block].z = z * game->block_size.z_level;
                    used_wall_block++;

                }
                if (b->object == StairsUp1E || b->object == StairsUp2E || b->object == StairsUp3E || b->object == StairsUp4E) {
                    game->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                    game->walls[used_wall_block].x = x * game->block_size.x;
                    game->walls[used_wall_block].y = y * game->block_size.y;
                    game->walls[used_wall_block].z = z * game->block_size.z_level;
                    used_wall_block++;

                }
                if (b->object == StairsUp1W || b->object == StairsUp2W || b->object == StairsUp3W || b->object == StairsUp4W) {
                    game->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                    game->walls[used_wall_block].x = x * game->block_size.x;
                    game->walls[used_wall_block].y = y * game->block_size.y;
                    game->walls[used_wall_block].z = z * game->block_size.z_level;
                    used_wall_block++;

                }


                if (b->object == Floor){
                    game->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                    game->walls[used_wall_block].x = x * game->block_size.x;
                    game->walls[used_wall_block].y = y * game->block_size.y;
                    game->walls[used_wall_block].z = z * game->block_size.z_level;
                    used_wall_block++;
                    used_floors++;
                }



                if (b->object == WallBlock){
                    game->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                    game->walls[used_wall_block].x = x * game->block_size.x;
                    game->walls[used_wall_block].y = y * game->block_size.y;
                    game->walls[used_wall_block].z = z * game->block_size.z_level;
                    used_wall_block++;
                    wall_count++;
                }
                if (b->object == Ladder){
                    game->walls[used_wall_block].frame = texture_atlas_data[b->object].x_pos;
                    game->walls[used_wall_block].x = x * game->block_size.x;
                    game->walls[used_wall_block].y = y * game->block_size.y;
                    game->walls[used_wall_block].z = z * game->block_size.z_level;
                    used_wall_block++;
                }



                // Shadow
                //if (z+1 < game->dims.z_level-1 && b->object != Nothing) {
                    //printf("%d, %d, %d\n",x,y,z+1);

                WorldBlock *one_above = &renderer->assets.level.blocks[FLATTEN_3D_INDEX(x,y,z+1 ,game->dims.x, game->dims.y)];


                    //if (one_above->object == Floor || (one_above->object >=StairsUp1N && one_above->object <= StairsUp4W)) {
                    if (one_above->object == Floor && z+1 < game->dims.z_level){
                        //if (z >= 0) {
                        count_shadow++;
                        //printf("%d, %d, %d, ..... index: %d \n",x,y,z, FLATTEN_3D_INDEX(x,y,(z_up),game->dims.x, game->dims.y));
                            //}

                        game->walls[used_wall_block].frame = texture_atlas_data[Shaded].x_pos;
                        game->walls[used_wall_block].x = x * game->block_size.x;
                        game->walls[used_wall_block].y = y * game->block_size.y;
                        game->walls[used_wall_block].z = z * game->block_size.z_level;
                        used_wall_block++;

                    }
                    //}

            }
        }
    }

    ASSERT(used_wall_block <= 16384);


    // sort the game->walls on Z and Y to help openGl with depth testing etc.
    // this improves frame time from 8ms to 1ms (for 16000 walls)

    qsort(game->walls, used_wall_block, sizeof(Wall), wallsortfunc);


    printf("shadow casters (floor one above) : %d, floor count:%d , wall count: %d\n", count_shadow, used_floors, wall_count);

    game->wall_count = used_wall_block;
    //u32 j =0;

    set_wall_batch_sizes();
#define ACTOR_BATCH 1000

    game->actor_count = ACTOR_BATCH;
    ASSERT(game->actor_count <= 16384);
    set_actor_batch_sizes();


    for (u32 i = 0; i< game->actor_count; i++) {
        game->actors[i].x = rand_int(game->dims.x) * game->block_size.x;;
        game->actors[i].y = rand_int(game->dims.y) * game->block_size.y;
        game->actors[i].z =  rand_int(0) * game->block_size.z_level;
        game->actors[i].frame = rand_int(4);
        float speed = 1;//10 + rand_int(10); // px per seconds
        game->actors[i].dx = rand_bool() ? -1 * speed : 1 * speed;
        game->actors[i].dy = rand_bool() ? -1 * speed : 1 * speed;
        game->actors[i].palette_index = rand_float();
    }

    prepare_renderer();

    Mix_PlayChannel(-1, renderer->assets.wav1, 0);

    b32 wants_to_quit = false;
    SDL_Event e;

#ifdef IOS
    SDL_iPhoneSetAnimationCallback(renderer->Window, 1, update_frame, renderer->Window);
    SDL_AddEventWatch(event_filter, NULL);
#endif

    const u8 *keys = SDL_GetKeyboardState(NULL);
    char frameCount[64];
    char actorCount[64];
    char wallCount[64];
    char updateActorBufferDuration[64];

    u64 last_avg_frame_time = 0;
    u32 avg_frame_time_ticker = 0;
    u64 running_frame_time = 0;
    float last_frame_time_ms = 1.0f;

    maybe_load_libgame();

    while (! wants_to_quit) {
        maybe_load_libgame();
        snprintf(actorCount, 64, "actors: %d", game->actor_count);
        snprintf(wallCount, 64, "walls: %d", game->wall_count);

        game->glyph_count = 0;
        game->glyph_count += draw_text(frameCount, 0, 0, &renderer->assets.menlo_font);
        game->glyph_count += draw_text(actorCount, 0, 24, &renderer->assets.menlo_font);
        game->glyph_count += draw_text(wallCount, 0, 48, &renderer->assets.menlo_font);
        game->glyph_count += draw_text(updateActorBufferDuration, 0, 48+24,&renderer->assets.menlo_font);


        set_glyph_batch_sizes();

        u64 time = SDL_GetPerformanceCounter();


        SDL_PumpEvents();
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT || keys[SDL_SCANCODE_ESCAPE]) {
                 wants_to_quit = true;
            }
            if (keys[SDL_SCANCODE_Z]) {
                for (u32 j = 0; j < ACTOR_BATCH; j++) {
                    if (game->actor_count < (2048 * 32) - ACTOR_BATCH) {
                        actor_add(game);
                        u32 i = game->actor_count;
                        game->actors[i].x = rand_int(game->dims.x) * game->block_size.x;;
                        game->actors[i].y = rand_int(game->dims.y) * game->block_size.y;
                        game->actors[i].z =  rand_int(0) * game->block_size.z_level;//rand_int(game->dims.z_level) * game->block_size.z_level;
                        game->actors[i].frame = rand_int(4);
                        float speed = 10 + rand_int(10); // px per seconds
                        game->actors[i].dx = rand_bool() ? -1 * speed : 1 * speed;
                        game->actors[i].dy = rand_bool() ? -1 * speed : 1 * speed;
                        game->actors[i].palette_index = (1.0f/16.0f)*rand_int(16);// rand_float();
                        set_actor_batch_sizes();
                    } else {
                        printf("Wont be adding actors reached max already\n");
                    }
                }

            }
            if (keys[SDL_SCANCODE_LEFT]) {
                game->x_view_offset-=24;
                prepare_renderer(); // this goes to show that just updating the walls should be a function, I dont want to prepare all other buffers just because
            }
            if (keys[SDL_SCANCODE_RIGHT]) {
                game->x_view_offset+=24;
                prepare_renderer();  // this goes to show that just updating the walls should be a function, I dont want to prepare all other buffers just because

            }
            if (keys[SDL_SCANCODE_UP]) {
                game->y_view_offset-=12;
                prepare_renderer(); // this goes to show that just updating the walls should be a function, I dont want to prepare all other buffers just because
                printf("y offset: %d\n", game->y_view_offset);

            }
            if (keys[SDL_SCANCODE_DOWN]) {
                game->y_view_offset+=12;
                prepare_renderer();  // this goes to show that just updating the walls should be a function, I dont want to prepare all other buffers just because
                printf("y offset: %d\n", game->y_view_offset);

            }

            if (keys[SDL_SCANCODE_X]) {
                //printf("Want to remove an actor rand between 0-4  %d !\n", rand_int2(0, 4));
                for (u32 j = 0; j < ACTOR_BATCH; j++) {
                    actor_remove(game, rand_int2(0,  game->actor_count-1));
                    set_actor_batch_sizes();
                }
            }
            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    renderer->view.width = e.window.data1;
                    renderer->view.height = e.window.data2;
                    center_view();

                    prepare_renderer();
                    //glViewport(0, 0, renderer->Width, renderer->Height);
                }
            }
        }


        // TODO: plenty of bugs are in this loop, never really cleaned up after
        for (u32 i = 0; i < game->actor_count; i++) {
            if (game->actors[i].x <= 0 || game->actors[i].x >= ((game->dims.x-1) * game->block_size.x)) {
                if (game->actors[i].x < 0) {
                   game->actors[i].x = 0;
                }
                if (game->actors[i].x >= ((game->dims.x-1) * game->block_size.x)) {
                    game->actors[i].x = ((game->dims.x-1) * game->block_size.x);
                }

                game->actors[i].dx *= -1.0f;
            }
            game->actors[i].x += game->actors[i].dx * (last_frame_time_ms/1000.0f);

            if (game->actors[i].y <= 0 || game->actors[i].y >= ((game->dims.y-1) * game->block_size.y)) {
                if (game->actors[i].z < 0) {
                   game->actors[i].z = 0;
                }
                if (game->actors[i].y > ((game->dims.y-1) * game->block_size.y)) {
                    game->actors[i].y = ((game->dims.y-1) * game->block_size.y);
                }

                game->actors[i].dy *= -1.0f;


            }
            game->actors[i].y += game->actors[i].dy *  (last_frame_time_ms/1000.0f);
            //printf("is it a float: %f \n", game->actors[i].y);
        }



        // sorting the actors is less profitable(because it runs every frame), it does however improve the speed from  6ms to 3.5ms for 16K actors (almost 100%)
        // on the negative side it introduces some fighting Z cases. (flickering!)
        // TODO: it might be nice to check the results of other sort algos https://github.com/swenson/sort
        qsort(game->actors,  game->actor_count, sizeof(Actor), actorsortfunc);

#ifndef IOS //IOS is being rendered with the animation callback instead.

        u64 delta = render(renderer->window);
        time = SDL_GetPerformanceCounter() - time;
        last_frame_time_ms =  (float)((float)time/SDL_GetPerformanceFrequency()) * 1000.0f;

        //        printf("%f\n", (float)((float)time/SDL_GetPerformanceFrequency()) * 1000.0f);


        // I AM AVERIGING FPS over 5 frames.
        running_frame_time += time;
        avg_frame_time_ticker++;
        if (avg_frame_time_ticker == 5) {
            last_avg_frame_time = ((float)(running_frame_time/5.0f)/(SDL_GetPerformanceFrequency()) ) * 1000.0f;
            avg_frame_time_ticker = 0;
            running_frame_time = 0;
        }
        snprintf (frameCount, 64, "ms: %.2f",  (float)last_avg_frame_time );
        snprintf (updateActorBufferDuration, 64, " %.2f actor batch update",  ((float)delta/(SDL_GetPerformanceFrequency()) ) * 1000.0f );
        //snprintf (frameCount, 64, "frame: %.2f",  ((float)(time)/(SDL_GetPerformanceFrequency()) ) * 1000.0f );

#endif

    }
    quit();
    return 1;
}
