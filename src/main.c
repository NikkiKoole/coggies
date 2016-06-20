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
internal void resource_font(const char *path) {
    // lets try and parse a fnt file.
    char buffer[256];
    make_font(resource(path, buffer));
}

internal void resource_texture(GLuint *tex, const char *path) {
    char buffer[256];
    make_texture(tex, resource(path, buffer));
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
    resource_font("menlobm.fnt");
    resource_texture(&renderer->assets.tex1, "Untitled3.tga");
    resource_texture(&renderer->assets.tex2, "palette2.tga");

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

internal void draw_ui() {
    //draw_text(fg, bg, text);//
    //draw_line();//
    //draw_button("some label");

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
    printf("Memory is initialized\n");


    UNUSED(argc);
    UNUSED(argv);

    renderer->view.width = 1920; //1800;
    renderer->view.height = 960;

    initialize_SDL();
    initialize_GL();
    load_resources();
    renderer->walls.count = 2000;



    u32 j =0;
    for (u32 x = 0; x< 15; x++) {
        for (u32 y = 0; y < 10; y++) {
            for (u32 z = 0; z <  5; z++) {
                game->walls[j].x = x;
                game->walls[j].y = y;
                game->walls[j].z = z;
                game->walls[j].frame = 0;
                j++;
            }
        }
    }


    game->actor_count = 150;//2048 * 8 ;b
    ASSERT(game->actor_count <= 16384);
    set_actor_batch_sizes();


    for (u32 i = 0; i< game->actor_count; i++) {
        game->actors[i].x = rand_int(renderer->view.width);
        game->actors[i].y = rand_int(renderer->view.height);
        game->actors[i].z =  0;//rand_int(50); ; // todo make it 3d
        game->actors[i].frame = rand_int(3);
        int speed = 1 + rand_int(5);
        game->actors[i].dx = rand_bool() ? -1 * speed : 1 * speed;
        game->actors[i].dy = rand_bool() ? -1 * speed : 1 * speed;
        game->actors[i].palette_index = rand_float();
    }

    prepare_renderer();

    //Mix_PlayMusic(State->music1, -1);
    Mix_PlayChannel(-1, renderer->assets.wav1, 0);


    b32 wants_to_quit = false;
    SDL_Event e;

#ifdef IOS
    SDL_iPhoneSetAnimationCallback(renderer->Window, 1, update_frame, renderer->Window);
    SDL_AddEventWatch(event_filter, NULL);
#endif

    const u8 *keys = SDL_GetKeyboardState(NULL);

    while (! wants_to_quit) {
        SDL_PumpEvents();
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT || keys[SDL_SCANCODE_ESCAPE]) {
                 wants_to_quit = true;
            }
            if (keys[SDL_SCANCODE_INSERT]) {
                //printf("Want to add a new actor!\n");
                for (j = 0; j < 250; j++) {
                    if (game->actor_count < (2048 * 8) - 250) {
                        actor_add(game);
                        u32 i = game->actor_count;
                        game->actors[i].x = rand_int(renderer->view.width);
                        game->actors[i].y = rand_int(renderer->view.height);
                        game->actors[i].z =  0;//rand_int(50); ; // todo make it 3d
                        game->actors[i].frame = rand_int(3);
                        int speed = 1 + rand_int(5);
                        game->actors[i].dx = rand_bool() ? -1 * speed : 1 * speed;
                        game->actors[i].dy = rand_bool() ? -1 * speed : 1 * speed;
                        game->actors[i].palette_index = rand_float();
                        set_actor_batch_sizes();
                    } else {
                        printf("Wont be adding actors reached max already\n");
                    }
                }
                printf("actor count: %d \n", game->actor_count);

            }
            if (keys[SDL_SCANCODE_DELETE]) {
                //printf("Want to remove an actor rand between 0-4  %d !\n", rand_int2(0, 4));
                for (j = 0; j < 250; j++) {
                    actor_remove(game, rand_int2(0,  game->actor_count-1));
                    set_actor_batch_sizes();
                }
                printf("actor count: %d \n", game->actor_count);

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

        //usleep(500000);
        for (u32 i = 0; i < game->actor_count; i++) {
            if (game->actors[i].x <= 0 || game->actors[i].x >= renderer->view.width) {
                if (game->actors[i].x < 0) {
                    game->actors[i].x = 0;
                }
                if (game->actors[i].x > renderer->view.width) {
                    game->actors[i].x = renderer->view.width;
                }

                game->actors[i].dx *= -1;
            }
            game->actors[i].x += game->actors[i].dx; //rand_int(State->view.width);

            if (game->actors[i].y <= 0 || game->actors[i].y >= renderer->view.height) {
                if (game->actors[i].y < 0) {
                    game->actors[i].y = 0;
                }
                if (game->actors[i].y > renderer->view.height) {
                    game->actors[i].y = renderer->view.height;
                }

                game->actors[i].dy *= -1;


            }
            game->actors[i].y += game->actors[i].dy; //rand_int(State->view.yheight);
            game->actors[i].z = 0;                     // todo make it 3d
        }
#ifndef IOS //IOS is being rendered with the animation callback instead.
        render(renderer->window);
#endif
    }
    quit();
    return 1;
}
