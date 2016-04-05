#include "SDL.h"
#include "SDL_mixer.h"

#include "types.h"
#include "renderer.h"
#include "memory.h"
#include "random.h"

extern RenderState *RState;
extern GameState *GState;

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
    RState->view.width = displayMode.w;
    RState->view.height = displayMode.h;
    SDL_Log("%d,%d\n", RState->view.width, RState->view.height);
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

    RState->window = SDL_CreateWindow("Hello World",
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SDL_WINDOWPOS_UNDEFINED,
                                      RState->view.width, RState->view.height,
                                      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    SDL_ASSERT(RState->window != NULL);

    RState->context = SDL_GL_CreateContext(RState->window);
    SDL_ASSERT(RState->context != NULL);

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
    resource_texture(&RState->assets.tex1, "Untitled3.tga");
    resource_texture(&RState->assets.tex2, "palette2.tga");

#ifdef GL3
    resource_shader(&RState->assets.shader1, "gl330.vert", "gl330.frag");
#endif
#ifdef GLES
    resource_shader(&RState->assets.shader1, "gles20.vert", "gles20.frag");
#endif

    resource_ogg(&RState->assets.music1, "Stiekem.ogg");
    resource_wav(&RState->assets.wav1, "scratch.wav");
}

internal void quit(void) {
    SDL_DestroyWindow(RState->window);
    RState->window = NULL;
    SDL_Quit();
}

internal void update_frame(void *param) {
    render((SDL_Window *)param);
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

    RState->view.width = 1024; //1800;
    RState->view.height = 1024;

    initialize_SDL();
    initialize_GL();
    load_resources();
    RState->walls.count = 2000;

    for (u32 i = 0; i < RState->walls.count; i++) {
        GState->walls[i].x = rand_int(100);
        GState->walls[i].y = rand_int(10);
        GState->walls[i].z = 0; // todo make it 3d
        GState->walls[i].frame = 0;
    }

    RState->actors.count = 2000;
    for (u32 i = 0; i < RState->actors.count; i++) {
        GState->actors[i].x = rand_int(RState->view.width);
        GState->actors[i].y = rand_int(RState->view.height);
        GState->actors[i].z = 0; // todo make it 3d
        GState->actors[i].frame = rand_int(4);
        int speed = 1 + rand_int(5);
        GState->actors[i].dx = rand_bool() ? -1 * speed : 1 * speed;
        GState->actors[i].dy = rand_bool() ? -1 * speed : 1 * speed;
        GState->actors[i].palette_index = rand_float();
    }

    prepare_renderer();

    //Mix_PlayMusic(State->music1, -1);
    Mix_PlayChannel(-1, RState->assets.wav1, 0);


    b32 wants_to_quit = false;
    SDL_Event e;

#ifdef IOS
    SDL_iPhoneSetAnimationCallback(RState->Window, 1, update_frame, RState->Window);
    SDL_AddEventWatch(event_filter, NULL);
#endif

    const u8 *keys = SDL_GetKeyboardState(NULL);

    while (! wants_to_quit) {
        SDL_PumpEvents();
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT || keys[SDL_SCANCODE_ESCAPE]) {
                 wants_to_quit = true;
            }
            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    printf("resized!\n");
                    RState->view.width = e.window.data1;
                    RState->view.height = e.window.data2;
                    prepare_renderer();
                    //glViewport(0, 0, RState->Width, RState->Height);
                }
            }
        }
#ifndef IOS
        //usleep(500000);
        for (u32 i = 0; i < RState->actors.count; i++) {
            if (GState->actors[i].x <= 0 || GState->actors[i].x >= RState->view.width) {
                if (GState->actors[i].x < 0) {
                    GState->actors[i].x = 0;
                }
                if (GState->actors[i].x > RState->view.width) {
                    GState->actors[i].x = RState->view.width;
                }

                GState->actors[i].dx *= -1;
            }
            GState->actors[i].x += GState->actors[i].dx; //rand_int(State->view.width);

            if (GState->actors[i].y <= 0 || GState->actors[i].y >= RState->view.height) {
                if (GState->actors[i].y < 0) {
                    GState->actors[i].y = 0;
                }
                if (GState->actors[i].y > RState->view.height) {
                    GState->actors[i].y = RState->view.height;
                }

                GState->actors[i].dy *= -1;
            }
            GState->actors[i].y += GState->actors[i].dy; //rand_int(State->view.yheight);
            GState->actors[i].z = 0;                     // todo make it 3d
        }
        render(RState->window);
#endif
    }
    quit();
    return 1;
}
