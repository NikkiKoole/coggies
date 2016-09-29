#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "GLKVector2.h"

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;
TTF_Font *font = NULL;

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 1024;
const int ACTOR_COUNT = 1250;

static SDL_Texture* loadTexture(char* path)
{
    SDL_Texture* newTexture = NULL;
    SDL_Surface* loadedSurface = IMG_Load(path);
    if (loadedSurface == NULL) {
        printf("Unable to load image: %s SDL_Image_Error: ", path, IMG_GetError());
    } else {
        newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        if (newTexture == NULL) {
            printf("Unable to create texture from : %s SDL_Error: %s", path, SDL_GetError());
        }
        SDL_FreeSurface(loadedSurface);
    }
    return newTexture;
}

static bool init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) goto SDL_Error;

    window = SDL_CreateWindow( "Steering test",
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               SCREEN_WIDTH,
                               SCREEN_HEIGHT,
                               SDL_WINDOW_SHOWN);

    if (window == NULL) goto SDL_Error;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) goto SDL_Error;

    SDL_SetRenderDrawColor(renderer, 0xFF, 0xAA, 0xFF, 0xFF);
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        printf("SDL_Image could not initialize");
        goto SDL_Error;
    }

    if( TTF_Init() == -1 ){
        printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
        goto SDL_Error;
    }

    texture = loadTexture("resources/textures/circle.png");
    if (texture == NULL) goto SDL_Error;

    font = TTF_OpenFont( "resources/fonts/monaco.ttf", 28 );
    if( font == NULL )
    {
        printf( "Failed to load lazy font! SDL_ttf Error: %s\n", TTF_GetError() );
        goto SDL_Error;
    }


    return true;

    SDL_Error:
    printf("Error in %s, SDl_Error: %s\n", __FUNCTION__, SDL_GetError());
    return false;
}

static void close_app() {
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
    SDL_DestroyWindow(window);
    window = NULL;
    IMG_Quit();
    SDL_Quit();
}

typedef struct {
    GLKVector2 location;
    GLKVector2 velocity;
    GLKVector2 acceleration;
} Actor;

static void actor_applyForce(Actor *a, GLKVector2 force) {
    a->acceleration = GLKVector2Add(a->acceleration, force);
}


int main() {
    init();
    srand(time(NULL));
    bool quit = false;
    SDL_Event e;

    SDL_Rect SrcR = {.x=0, .y=0, .w=24, .h=24};
    SDL_Rect DestR = {.x=100, .y=100, .w=24, .h=24};


    Actor actors[ACTOR_COUNT];
    for (int i=0; i< ACTOR_COUNT; i++) {

        actors[i].location     = GLKVector2Make((rand() % SCREEN_WIDTH), (rand() % SCREEN_HEIGHT));
        actors[i].velocity     = GLKVector2Make(0,0);
        actors[i].acceleration = GLKVector2Make(0,0);

    }


    while (!quit) {
        while( SDL_PollEvent( &e ) != 0 ) {
            if( e.type == SDL_QUIT ) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym) quit=true;
            }
        }
        SDL_RenderClear(renderer);


        Uint64 before = SDL_GetPerformanceCounter();
        /* // update actors */
        for (int i =0 ; i< ACTOR_COUNT ; i++) {
            actor_applyForce(&actors[i], GLKVector2Make((-2 + (float)(rand() % 5))/100.0f, (-2 + (float)(rand() % 5))/100.0f));
            actors[i].velocity = GLKVector2Add(actors[i].velocity, actors[i].acceleration);
            actors[i].location = GLKVector2Add(actors[i].location, actors[i].velocity);
            actors[i].acceleration = GLKVector2MultiplyScalar(actors[i].acceleration, 0);
            if (actors[i].location.x+12 < 0) actors[i].location.x = SCREEN_WIDTH-12;
            if (actors[i].location.x+12 > SCREEN_WIDTH) actors[i].location.x = -12;
            if (actors[i].location.y+12 < 0) actors[i].location.y = SCREEN_HEIGHT-12;
            if (actors[i].location.y+12 > SCREEN_HEIGHT) actors[i].location.y = -12;

        }
        Uint64 after = SDL_GetPerformanceCounter();
        // draw actors
        for (int i =0 ; i< ACTOR_COUNT ; i++) {
            DestR.x = actors[i].location.x;
            DestR.y = actors[i].location.y;
            SDL_RenderCopy(renderer, texture, &SrcR,  &DestR);
        }



        //// frame time text
        float update = (float)(after - before)/SDL_GetPerformanceFrequency();
        char buffer [50];
        sprintf(buffer, "%f ms", update*1000);
        SDL_Rect SrcText = {.x=0, .y=0, .w=0, .h=0};
        SDL_Rect DestText = {.x=10, .y=10, .w=0, .h=0};
        SDL_Surface* textSurface = TTF_RenderText_Solid( font, buffer, (SDL_Color){0,0,0,0} );
        SDL_Texture* mTexture = SDL_CreateTextureFromSurface( renderer, textSurface );
        SrcText.w = DestText.w = textSurface->w;
        SrcText.h = DestText.h = textSurface->h;
        SDL_FreeSurface(textSurface);
        SDL_RenderCopy(renderer, mTexture, &SrcText,  &DestText);





        SDL_RenderPresent(renderer);

    }
    close_app();
}
