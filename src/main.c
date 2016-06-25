#include "SDL.h"
#include "SDL_mixer.h"

#include "types.h"
#include "renderer.h"
#include "memory.h"
#include "random.h"

#include <math.h>




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

    //SDL_ASSERT(SDL_Init(SDL_INIT_VIDEO));
    //SDL_ASSERT(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO));

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

    renderer->window = SDL_CreateWindow("Hello World",
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SDL_WINDOWPOS_UNDEFINED,
                                      renderer->view.width, renderer->view.height,
                                      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    SDL_ASSERT(renderer->window != NULL);

    renderer->context = SDL_GL_CreateContext(renderer->window);
    SDL_ASSERT(renderer->context != NULL);

#ifdef GL3
    SDL_ASSERT(SDL_GL_SetSwapInterval(1) >= 0);
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



internal char *resource(const char *path, char *buffer) {
    strcpy(buffer, getResourcePath());
    strcat(buffer, path);
    return buffer;
}
internal void resource_sprite_atlas(const char *path) {
    char buffer[256];
    make_sprite_atlas(resource(path, buffer));

}
internal void resource_font(BM_Font *font, const char *path) {
    // lets try and parse a fnt file.
    char buffer[256];
    make_font(font, resource(path, buffer));
}

internal void resource_texture(Texture *t, const char *path) {
    char buffer[256];
    make_texture(t, resource(path, buffer));
}
internal void resource_shader(GLuint *shader, const char *vertPath, const char *fragPath) {
    char buffer[256];
    char buffer2[256];
    *shader = make_shader_program(resource(vertPath, buffer), resource(fragPath, buffer2));
}
internal void resource_ogg(Mix_Music **music, const char *path) {
    char buffer[256];
    *music = Mix_LoadMUS(resource(path, buffer));
    SDL_MIX_ASSERT(*music);
}
internal void resource_wav(Mix_Chunk **chunk, const char *path) {
    char buffer[256];
    *chunk = Mix_LoadWAV(resource(path, buffer));
    SDL_MIX_ASSERT(*chunk);
}
internal void load_resources(void) {
    resource_sprite_atlas("out.sho");
    resource_font(&renderer->assets.menlo_font, "menlo.fnt");

    resource_texture(&renderer->assets.menlo, "menlo.tga");
    resource_texture(&renderer->assets.sprite, "Untitled3.tga");
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


// TODO generalise these two into a reusable function

internal void set_actor_batch_sizes() {
    u32 used_batches = ceil(game->actor_count / 2048.0f);
    renderer->used_actor_batches = used_batches;

    if (used_batches == 1) {
        renderer->actors[0].count = game->actor_count;
    } else if (used_batches > 1) {
        for (u32 i = 0; i < used_batches-1; i++) {
            renderer->actors[i].count = 2048;
        }
        renderer->actors[used_batches-1].count = game->actor_count % 2048;
    } else {
        renderer->used_actor_batches = 0;
    }
}

internal void set_glyph_batch_sizes() {
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

internal void center_view() {
    int real_world_width = game->world_width * game->block_width;
    int real_world_depth = game->world_depth * game->block_depth;
    int real_world_height = game->world_height * game->block_height;

    int offset_x_blocks = (renderer->view.width - real_world_width) / 2;
    int offset_y_blocks = (renderer->view.height - (real_world_height+real_world_depth)) / 2;

    game->x_view_offset = offset_x_blocks;
    game->y_view_offset = offset_y_blocks;
}


int main(int argc, char **argv) {

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
    renderer->view.height = 960;

    initialize_SDL();
    initialize_GL();
    load_resources();

    game->world_width = 40;
    game->world_height = 3;
    game->world_depth = 10;

    game->block_width = 24;
    game->block_depth = 12;
    game->block_height = 96;

    center_view();

    renderer->walls.count = (game->world_width * game->world_height * game->world_depth);

    ASSERT(renderer->walls.count <= 2048 && "Make buffers larger for world blocks");

    u32 j =0;
    for (u32 x = 0; x< game->world_width ; x++) {
        for (u32 y = 0; y < game->world_height; y++) {
            for (u32 z = 0; z <  game->world_depth; z++) {
                game->walls[j].x = x * game->block_width;
                game->walls[j].y = y * game->block_height;
                game->walls[j].z = z * game->block_depth;
                game->walls[j].frame = rand_int(20) > 1 ? 3 : 0;
                j++;
            }
        }
    }

#define ACTOR_BATCH 200

    game->actor_count = ACTOR_BATCH;
    ASSERT(game->actor_count <= 16384);
    set_actor_batch_sizes();


    for (u32 i = 0; i< game->actor_count; i++) {
        game->actors[i].x = rand_int(game->world_width) * game->block_width;;
        game->actors[i].y = rand_int(game->world_height) * game->block_height;
        game->actors[i].z =  0;//rand_int(game->world_depth) * game->block_depth ;
        game->actors[i].frame = rand_int(4);
        int speed = 0;
        game->actors[i].dx = rand_bool() ? -1 * speed : 1 * speed;
        game->actors[i].dz = rand_bool() ? -1 * speed : 1 * speed;
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


    while (! wants_to_quit) {
        snprintf(actorCount, 64, "actors: %d", game->actor_count);

        game->glyph_count = 0;
        game->glyph_count += draw_text(frameCount, 0, 0, &renderer->assets.menlo_font);
        game->glyph_count += draw_text(actorCount, 0, 24, &renderer->assets.menlo_font);
        set_glyph_batch_sizes();

        u64 time = SDL_GetPerformanceCounter();

        SDL_PumpEvents();
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT || keys[SDL_SCANCODE_ESCAPE]) {
                 wants_to_quit = true;
            }
            if (keys[SDL_SCANCODE_INSERT]) {
                for (j = 0; j < ACTOR_BATCH; j++) {
                    if (game->actor_count < (2048 * 8) - ACTOR_BATCH) {
                        actor_add(game);
                        u32 i = game->actor_count;
                        game->actors[i].x = rand_int(game->world_width) * game->block_width;;
                        game->actors[i].y = rand_int(game->world_height) * game->block_height;
                        game->actors[i].z =  rand_int(game->world_depth) * game->block_depth ;
                        game->actors[i].frame = rand_int(4);
                        int speed = 1;// + rand_int(5);
                        game->actors[i].dx = rand_bool() ? -1 * speed : 1 * speed;
                        game->actors[i].dz = rand_bool() ? -1 * speed : 1 * speed;
                        game->actors[i].palette_index = rand_float();
                        set_actor_batch_sizes();
                    } else {
                        printf("Wont be adding actors reached max already\n");
                    }
                }

            }
            if (keys[SDL_SCANCODE_LEFT]) {
                game->x_view_offset-=5;
                prepare_renderer(); // this goes to show that just updating the walls should be a function, I dont want to prepare all other buffers just because
            }
            if (keys[SDL_SCANCODE_RIGHT]) {
                game->x_view_offset+=5;
                prepare_renderer();  // this goes to show that just updating the walls should be a function, I dont want to prepare all other buffers just because

            }
            if (keys[SDL_SCANCODE_UP]) {
                game->y_view_offset-=5;
                prepare_renderer(); // this goes to show that just updating the walls should be a function, I dont want to prepare all other buffers just because
            }
            if (keys[SDL_SCANCODE_DOWN]) {
                game->y_view_offset+=5;
                prepare_renderer();  // this goes to show that just updating the walls should be a function, I dont want to prepare all other buffers just because

            }

            if (keys[SDL_SCANCODE_DELETE]) {
                //printf("Want to remove an actor rand between 0-4  %d !\n", rand_int2(0, 4));
                for (j = 0; j < ACTOR_BATCH; j++) {
                    actor_remove(game, rand_int2(0,  game->actor_count-1));
                    set_actor_batch_sizes();
                }
            }
            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    renderer->view.width = e.window.data1;
                    renderer->view.height = e.window.data2;
                    prepare_renderer();
                    //glViewport(0, 0, renderer->Width, renderer->Height);
                }
            }
        }

        for (u32 i = 0; i < game->actor_count; i++) {
            if (game->actors[i].x <= 0 || game->actors[i].x >= ((game->world_width-1) * game->block_width)) {
                if (game->actors[i].x < 0) {
                    game->actors[i].x = 0;
                }
                if (game->actors[i].x >= ((game->world_width-1) * game->block_width)) {
                    game->actors[i].x = ((game->world_width-1) * game->block_width);
                }

                game->actors[i].dx *= -1;
            }
            game->actors[i].x += game->actors[i].dx;

            if (game->actors[i].z <= 0 || game->actors[i].z >= ((game->world_depth-1) * game->block_depth)) {
                if (game->actors[i].z < 0) {
                    game->actors[i].z = 0;
                }
                if (game->actors[i].z > ((game->world_depth-1) * game->block_depth)) {
                    game->actors[i].z = ((game->world_depth-1) * game->block_depth);
                }

                game->actors[i].dz *= -1;


            }
            game->actors[i].z += game->actors[i].dz;
        }

#ifndef IOS //IOS is being rendered with the animation callback instead.
        render(renderer->window);
        time = SDL_GetPerformanceCounter() - time;
        snprintf (frameCount, 64, "frame: %.2f",  ((float)time/(float)(SDL_GetPerformanceFrequency()) )*1000.0f );
#endif

    }
    quit();
    return 1;
}
