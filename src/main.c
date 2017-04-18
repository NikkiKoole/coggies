#include "SDL.h"
#include "SDL_mixer.h"

#include "types.h"
#include "renderer.h"
#include "pathfind.h"
#include "memory.h"
#include "random.h"

#include <math.h>

#define NO_HOT_RELOADING
#ifdef NO_HOT_RELOADING
void game_update_and_render(Memory* memory,  RenderState *renderer, float last_frame_time_seconds, const u8 *keys, SDL_Event e);
#endif

void (*func)(Memory *memory, RenderState *renderer, float last_frame_time_seconds, const u8 *keys, SDL_Event e);

typedef struct {
    PermanentState * permanent;
    RenderState * renderer;
    DebugState * debug;
} IOSCallbackParams;


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


internal void initialize_SDL(RenderState *renderer) {
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
    //SetSDLIcon(renderer->window);
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


internal void load_resources(PermanentState *permanent, RenderState *renderer) {
    resource_level(permanent, &permanent->level, "levels/16.txt"); //two_floors.txt
    resource_sprite_atlas("out.sho");
    resource_font(&renderer->assets.menlo_font, "fonts/osaka.fnt");
    resource_png(&renderer->assets.menlo, "fonts/osaka.png");
    resource_png(&renderer->assets.blocks, "textures/blocks.png");
    resource_png(&renderer->assets.character, "textures/test.png");
    resource_png(&renderer->assets.palette, "textures/palette.png");

#ifdef GL3
    resource_shader(&renderer->assets.xyz_uv_palette, "shaders/xyz_uv_palette.GL330.vert", "shaders/xyz_uv_palette.GL330.frag");
    resource_shader(&renderer->assets.xyz_uv, "shaders/xyz_uv.GL330.vert", "shaders/xyz_uv.GL330.frag");
    resource_shader(&renderer->assets.xyz_rgb, "shaders/xyz_rgb.GL330.vert", "shaders/xyz_rgb.GL330.frag");
    resource_shader(&renderer->assets.xy_uv, "shaders/xy_uv.GL330.vert", "shaders/xy_uv.GL330.frag");
#endif
#ifdef GLES
    resource_shader(&renderer->assets.xyz_uv_palette, "shaders/xyz_uv_palette.GLES2.vert", "shaders/xyz_uv_palette.GLES2.frag");
    resource_shader(&renderer->assets.xyz_uv, "shaders/xyz_uv.GLES2.vert", "shaders/xyz_uv.GLES2.frag");
    resource_shader(&renderer->assets.xyz_rgb, "shaders/xyz_rgb.GLES2.vert", "shaders/xyz_rgb.GLES2.frag");
    resource_shader(&renderer->assets.xy_uv, "shaders/xy_uv.GLES2.vert", "shaders/xy_uv.GLES2.frag");
#endif

    //resource_ogg(&renderer->assets.music1, "ogg/Stiekem.ogg");
    resource_wav(&renderer->assets.wav1, "wav/scratch.wav");
}

internal void quit(RenderState *renderer) {
    SDL_DestroyWindow(renderer->window);
    renderer->window = NULL;
    SDL_Quit();
}

internal void update_frame(void *param) {
    IOSCallbackParams *p = (IOSCallbackParams *) param;
    render(p->permanent, p->renderer, p->debug);
}

internal void set_actor_batch_sizes(PermanentState *permanent, RenderState *renderer) {
#define ACTOR_PARTS 1

    u32 used_batches = ceil((permanent->actor_count * ACTOR_PARTS) / ((MAX_IN_BUFFER) * 1.0f));
    renderer->used_actor_batches = used_batches;

    if (used_batches == 1) {
        renderer->actors[0].count = (permanent->actor_count * ACTOR_PARTS);
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches - 1; i++) {
            renderer->actors[i].count = (MAX_IN_BUFFER);
        }
        renderer->actors[used_batches - 1].count = (permanent->actor_count * ACTOR_PARTS)  % (MAX_IN_BUFFER);
    } else {
        renderer->used_actor_batches = 0;
    }
    printf("used batches: %d\n", used_batches);
#undef ACTOR_PARTS
}


internal void set_glyph_batch_sizes(PermanentState *permanent, RenderState *renderer) {
    u32 used_batches = ceil(permanent->glyph_count / 2048.0f);
    renderer->used_glyph_batches = used_batches;

    if (used_batches == 1) {
        renderer->glyphs[0].count = permanent->glyph_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches - 1; i++) {
            renderer->glyphs[i].count = 2048;
        }
        printf("%d array out bounds glyphs\n", used_batches - 1);
        renderer->glyphs[used_batches - 1].count = permanent->glyph_count % 2048;
    } else {
        renderer->used_glyph_batches = 0;
    }
}


internal void draw_glyph(PermanentState *permanent, u32 offset, u32 x, u32 y, u32 sx, u32 sy, u32 w, u32 h) {
    u32 base = permanent->glyph_count;
    Glyph *g = &permanent->glyphs[base + offset];
    g->x = x;
    g->y = y;
    g->sx = sx;
    g->sy = sy;
    g->w = w;
    g->h = h;
}

u32 debug_text_x = 5;
u32 debug_text_y = 5;

internal u32 draw_text(char *str, u32 x, u32 y, BM_Font *font, PermanentState *permanent) {
    UNUSED(str);
    UNUSED(x);
    UNUSED(y);
    UNUSED(font);

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

        BM_Glyph glyph = font->chars[(u8)(str[i])];
        draw_glyph(permanent, drawn, currentX + glyph.xoffset, currentY + glyph.yoffset, glyph.x, glyph.y, glyph.width, glyph.height);
        currentX += (glyph.xadvance);
        drawn++;
    }
    debug_text_y = currentY;
    return drawn;
}




#pragma GCC diagnostic ignored "-Wformat-nonliteral"
internal void print(PermanentState *permanent, RenderState *renderer, const char *text, ...) {
    ASSERT(permanent != NULL && renderer != NULL);

    char buffer[999];
    va_list va;
    va_start(va, text);
    vsprintf(buffer, text, va);
    va_end(va);
    permanent->glyph_count += draw_text(buffer, debug_text_x, debug_text_y, &renderer->assets.menlo_font, permanent);
}
#pragma GCC diagnostic warning "-Wformat-nonliteral"

internal void center_view(PermanentState *permanent, RenderState *renderer) {
    int real_world_width = permanent->dims.x * permanent->block_size.x;
    int real_world_depth = permanent->dims.y * permanent->block_size.y / 2;
    int real_world_height = permanent->dims.z_level * permanent->block_size.z_level;

    s32 offset_x_blocks = (renderer->view.width - real_world_width) / 2;
    s32 offset_y_blocks = (renderer->view.height - (real_world_height + real_world_depth)) / 2;

    permanent->x_view_offset = offset_x_blocks;
    permanent->y_view_offset = real_world_depth + offset_y_blocks;
}

Shared_Library libgame = {
    .handle        = NULL,
    .name          = "./gamelibrary.so",
    .creation_time = 0,
    .size          = 0,
    .fn_name       = "game_update_and_render"
};

internal void stub(Memory *memory, RenderState *renderer, float last_frame_time_seconds, const u8 *keys, SDL_Event e) {
    UNUSED(memory);
    UNUSED(renderer);
    UNUSED(last_frame_time_seconds);
    UNUSED(keys);
    UNUSED(e);
    SDL_Delay(1);
}


PerfDict clone;
int ticker = 0;

internal void reset_debug_performance_components(DebugState *debug) {
    perf_dict_sort_clone(&debug->perf_dict, &clone);
    perf_dict_reset(&debug->perf_dict);
    ticker = 0;

}

internal void maybe_load_libgame(DebugState *debug) {
    stat(libgame.name, &libgame.stats);
    if (libgame.stats.st_ino != libgame.id) {
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
                //func = (void (*)(void)) SDL_LoadFunction(libgame.handle, libgame.fn_name);
                func = (void (*)(Memory *memory, RenderState *renderer, float last_frame_time_seconds, const u8 *keys, SDL_Event e))SDL_LoadFunction(libgame.handle, libgame.fn_name);
                if (func == NULL) {
                    printf("couldnt find: %s, error: %s\n", libgame.fn_name, SDL_GetError());
                } else {
                    printf("succes loading libgame timestamp: %d \n", SDL_GetTicks());
                    //perf_dict_reset(&debug->perf_dict);
                    reset_debug_performance_components(debug);
                }
            }
        }
    }
}

internal grid_node* get_random_walkable_node(Grid *grid) {
    grid_node *result;// = GetNodeAt(grid, 0,0,0);
    do {
        result = get_node_at(grid, rand_int(grid->width), rand_int(grid->height), rand_int(grid->depth));
    } while (!result->walkable);
    return result;
}




int main(int argc, char **argv) {
    Memory _memory;
    Memory *memory = &_memory;

    void *base_address = (void *)GIGABYTES(0);
    memory->permanent_size = MEGABYTES(20);
    memory->scratch_size = MEGABYTES(10);
    memory->node16_size = MEGABYTES(30);
    memory->debug_size = MEGABYTES(2);

    u64 total_storage_size = memory->permanent_size + memory->scratch_size + memory->node16_size + memory->debug_size;
    memory->permanent = mmap(base_address, total_storage_size,
                             PROT_READ | PROT_WRITE,
                             MAP_ANON | MAP_PRIVATE,
                             -1, 0);
    memory->scratch = (u8 *)(memory->permanent) + memory->permanent_size;
    memory->node16 = (u8 *)(memory->scratch) + memory->scratch_size;
    memory->debug = (u8 *)(memory->node16) + memory->node16_size;


    memory->is_initialized = false;
    printf("\ntotal amount of MB in use: %lu.\n",(unsigned long) total_storage_size / 1000000);
    ASSERT(sizeof(PermanentState) <= memory->permanent_size);
    PermanentState *permanent = (PermanentState *)memory->permanent;
    ASSERT(sizeof(ScratchState) <= memory->scratch_size);
    ScratchState *scratch = (ScratchState *)memory->scratch;

    ASSERT(sizeof(Node16Arena) <= memory->node16_size);
    Node16Arena *foundPaths = (Node16Arena *)memory->node16;
    ASSERT(sizeof(DebugState) <= memory->debug_size);
    DebugState *debug = (DebugState *)memory->debug;
    RenderState _rstate; //TODO: make this use memory scheme instead of stack space, it gets in the order of 5-8 Mb now (when using 32bit floats)
    printf("Renderstate size MB: %lu\n\n", ((unsigned long) sizeof _rstate)/1000000);
    RenderState *renderer = &_rstate;

    //printf("permanent struct size: %lu\n",(unsigned long)(sizeof(PermanentState)));
    initialize_arena(&permanent->arena,
                     memory->permanent_size - sizeof(PermanentState),
                     (u8 *)memory->permanent + sizeof(PermanentState));

    initialize_arena(&scratch->arena,
                     memory->scratch_size - sizeof(ScratchState),
                     (u8 *)memory->scratch + sizeof(ScratchState));

    initialize_arena(&foundPaths->arena,
                     memory->node16_size - sizeof(Node16Arena),
                     (u8 *)memory->node16 + sizeof(Node16Arena));


    initialize_arena(&debug->arena,
                     memory->debug_size - sizeof(DebugState),
                     (u8 *)memory->debug + sizeof(DebugState));

    memory->is_initialized = false;



    UNUSED(argc);
    UNUSED(argv);

    renderer->view.width = 1920; //1800;
    renderer->view.height = 1080;

    initialize_SDL(renderer);
    initialize_GL();
    load_resources(permanent, renderer);
    setup_shader_layouts(renderer);

    permanent->dims = (WorldDims){permanent->level.x, permanent->level.y, permanent->level.z_level};
    permanent->block_size = (WorldDims){24, 24, 96};

    center_view(permanent, renderer);

#define ACTOR_BATCH 1

    permanent->actor_count = ACTOR_BATCH;
    ASSERT(permanent->actor_count <= 16384);
    prepare_renderer(permanent, renderer);
    Mix_PlayChannel(-1, renderer->assets.wav1, 0);

    b32 wants_to_quit = false;
    SDL_Event e;

#ifdef IOS
    IOSCallbackParams p = (IOSCallbackParams){permanent, renderer, debug};
    SDL_iPhoneSetAnimationCallback(renderer->window, 1, update_frame, &p);
    SDL_AddEventWatch(event_filter, NULL);
#endif

    const u8 *keys = SDL_GetKeyboardState(NULL);
    float last_frame_time_ms = 1.0f;

    prepare_renderer(permanent, renderer);
    u64 freq = SDL_GetPerformanceFrequency();

    while (!wants_to_quit) {
        ticker++;
        debug_text_y = 5;
        permanent->glyph_count = 0;
        if (memory->is_initialized) {
            print(permanent, renderer, "%.2f ms\n", (float)last_frame_time_ms);
        }
        if (ticker == 60) {
            reset_debug_performance_components(debug);
        }
        for (int i = 0; i < PERF_DICT_SIZE; i++) {
            PerfDictEntry *perf_entry = &clone.data[i];
            float averaged = ((float)(perf_entry->total_time / (float)freq) * 1000.0f) / 60;
            float min = ((float)(perf_entry->min / (float)freq) * 1000.0f);
            float max = ((float)(perf_entry->max / (float)freq) * 1000.0f);

            if (perf_entry->times_counted >= 60) {
                ASSERT(perf_entry->key != NULL);
                print(permanent, renderer, "%-6.3f %-12s  min:%.5f max:%.3f (x%d)\n", averaged, perf_entry->key, min, max, perf_entry->times_counted / 60);
            } else {
                break;
            }
        }

        set_glyph_batch_sizes(permanent, renderer);
        u64 begin_render_time = SDL_GetPerformanceCounter();

#ifndef NO_HOT_RELOADING
        maybe_load_libgame(debug);
#endif

        BEGIN_PERFORMANCE_COUNTER(main_loop);

        // INPUT  // MOVE ALL OF THIS INTO GAME!!!!!!!
        SDL_PumpEvents();
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT || keys[SDL_SCANCODE_ESCAPE]) {
                wants_to_quit = true;
            }
            if (keys[SDL_SCANCODE_LEFT]) {
                permanent->x_view_offset -= 24;
                //prepare_renderer(permanent, renderer); // this goes to show that just updating the walls should be a function, I dont want to prepare all other buffers just because
            }
            if (keys[SDL_SCANCODE_RIGHT]) {
                permanent->x_view_offset += 24;
                //prepare_renderer(permanent, renderer); // this goes to show that just updating the walls should be a function, I dont want to prepare all other buffers just because
            }
            if (keys[SDL_SCANCODE_UP]) {
                permanent->y_view_offset -= 12;
                //prepare_renderer(permanent, renderer); // this goes to show that just updating the walls should be a function, I dont want to prepare all other buffers just because
            }
            if (keys[SDL_SCANCODE_DOWN]) {
                permanent->y_view_offset += 12;
                //prepare_renderer(permanent, renderer); // this goes to show that just updating the walls should be a function, I dont want to prepare all other buffers just because
            }
            if (keys[SDL_SCANCODE_Z]) {
                for (u32 j = 0; j < ACTOR_BATCH; j++) {
                    if (permanent->actor_count < (2048 * 32) - ACTOR_BATCH) {
                        actor_add(permanent);
                        u32 i = permanent->actor_count;
                        grid_node * Start  = get_random_walkable_node(permanent->grid);
                        permanent->steer_data[i].location.x = Start->X * permanent->block_size.x;
                        permanent->steer_data[i].location.y = Start->Y * permanent->block_size.y;
                        permanent->steer_data[i].location.z = Start->Z * permanent->block_size.z_level;
                        permanent->anim_data[i].frame = rand_int(4);
                        float speed = 4 + rand_int(10); // px per seconds
                        permanent->steer_data[i].dx = rand_bool() ? -1 * speed : 1 * speed;
                        permanent->steer_data[i].dy = rand_bool() ? -1 * speed : 1 * speed;
                        permanent->anim_data[i].palette_index = (1.0f / 16.0f) * rand_int(16); // rand_float();
                        set_actor_batch_sizes(permanent, renderer);
                    } else {
                        printf("Wont be adding actors reached max already\n");
                    }
                }
            }
            if (keys[SDL_SCANCODE_X]) {
                for (u32 j = 0; j < ACTOR_BATCH; j++) {
                    actor_remove(permanent, rand_int2(0, permanent->actor_count - 1));
                    set_actor_batch_sizes(permanent, renderer);
                }
            }
            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    renderer->view.width = e.window.data1;
                    renderer->view.height = e.window.data2;
                    center_view(permanent, renderer);
                    prepare_renderer(permanent, renderer);
                }
            }
        }
        // END INPUT

#ifndef NO_HOT_RELOADING
        func(memory, renderer, last_frame_time_ms / 1000.0f, keys, e);
#else
        game_update_and_render(memory, renderer, last_frame_time_ms / 1000.0f, keys, e);
#endif

#ifndef IOS //IOS is being rendered with the animation callback instead.

        render(permanent, renderer, debug);
        u64 end_render_time = SDL_GetPerformanceCounter();
        last_frame_time_ms = ((float)(end_render_time - begin_render_time) / freq) * 1000.0f;
        END_PERFORMANCE_COUNTER(main_loop);
#endif
    }
    quit(renderer);
    return 1;
}
