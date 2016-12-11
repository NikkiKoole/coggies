#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Surface * screen;

int offsets[20*2][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0},
    { 1, 1, 1, 1, 1, 1, 0, 0},
    { 0, 0, 0, 1, 1, 1, 1, 1},
    { 2, 2, 2, 2, 1, 1, 1, 0},
    { 0, 0, 1, 1, 1, 2, 2, 2},
    { 3, 3, 3, 2, 2, 1, 1, 1},
    { 0, 1, 1, 1, 2, 2, 3, 3},
    { 4, 4, 4, 3, 3, 2, 2, 1},
    { 0, 1, 2, 2, 3, 3, 4, 4},
    { 5, 5, 5, 4, 3, 3, 2, 1},
    { 0, 1, 2, 3, 3, 4, 5, 5},
    { 6, 6, 6, 5, 4, 3, 2, 1},
    { 0, 1, 2, 3, 4, 5, 6, 6},
    { 7, 7, 6, 6, 5, 4, 3, 1},
    { 0, 1, 3, 4, 5, 6, 6, 7},
    { 8, 8, 7, 6, 6, 4, 3, 2},
    { 0, 2, 3, 4, 6, 6, 7, 8},
    { 9, 9, 8, 8, 6, 5, 3, 2},
    { 0, 2, 3, 5, 6, 8, 8, 9},
    {10,10, 9, 8, 7, 5, 4, 2},
    { 0, 2, 4, 5, 7, 8, 9,10},
    {11,11,10, 9, 8, 6, 4, 2},
    { 0, 2, 4, 6, 8, 9,10,11},
    {12,12,11,10, 8, 7, 5, 2},
    { 0, 2, 5, 7, 8,10,11,12},
    {13,13,12,11, 9, 7, 5, 3},
    { 0, 3, 5, 7, 9,11,12,13},
    {14,14,13,12,10, 8, 5, 3},
    { 0, 3, 5, 8,10,12,13,14},
    {15,15,14,12,10, 8, 6, 3},
    { 0, 3, 6, 8,10,12,14,15},
    {16,16,15,13,11, 9, 6, 3},
    { 0, 3, 6, 9,11,13,15,16},
    {17,17,16,14,12, 9, 7, 3},
    { 0, 3, 7, 9,12,14,16,17},
    {18,18,17,15,13,10, 7, 4},
    { 0, 4, 7,10,13,15,17,18},
    {19,19,18,16,13,11, 7, 4},
    { 0, 4, 7,11,13,16,18,19}
};

static SDL_Texture* loadTexture(const char* path)
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

static void app_start() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) goto SDL_Error;
    window = SDL_CreateWindow( "Skeleton Joy!",
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


    return;

 SDL_Error:
    printf("Error in %s, SDl_Error: %s\n", __FUNCTION__, SDL_GetError());
}

static void app_end() {
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
    SDL_DestroyWindow(window);
    window = NULL;
    IMG_Quit();
    SDL_Quit();
}

SDL_Point sp_rotated_image(int x, int y, int rotation, SDL_Point pivot) {
    SDL_Point result = (SDL_Point){x,y};
    result.x -= pivot.x;
    result.y -= pivot.y;

    if (rotation >= 0 && rotation < 8) {

    }else if (rotation >= 8 && rotation < 16) {
        result.x += 1;
    }else if (rotation >= 16 && rotation < 24) {
        result.x += 1;
        result.y += 1;
    }else if (rotation >= 24 && rotation < 32) {
        result.y += 1;
    }
    return result;
}

SDL_Point ep_rotated_image(int x, int y, int rotation, int length, SDL_Point pivot) {
    SDL_Point result = (SDL_Point){x,y};
    if (rotation >= 0 && rotation < 8) {
        result.x += offsets[(length*2)+0][rotation % 8];
        result.y += offsets[(length*2)+1][rotation % 8];
    } else if (rotation >= 8 && rotation < 16) {
        result.x -= offsets[(length*2)+1][rotation % 8];
        result.y += offsets[(length*2)+0][rotation % 8];
    } else if (rotation >= 16 && rotation < 24) {
        result.x -= offsets[(length*2)+0][rotation % 8];
        result.y -= offsets[(length*2)+1][rotation % 8];
    } else if (rotation >= 24 && rotation < 32) {
        result.x += offsets[(length*2)+1][rotation % 8];
        result.y -= offsets[(length*2)+0][rotation % 8];
    }
    return result;
}

SDL_Point draw_body_part(SDL_Texture* leg, int x, int y, int rotation, int length, SDL_Point pivot, int color) {

    //draw start and end as extra dots, to verify them.


    SDL_Point ep1 = ep_rotated_image(x, y, rotation, (length-1), pivot);
    /* SDL_Rect end; */
    /* end.x = ep1.x; */
    /* end.y = ep1.y; */
    /* end.w = 1; */
    /* end.h = 1; */
    /* SDL_SetRenderDrawColor( renderer, 255, 0, 255, 155 ); */
    /* SDL_RenderFillRect( renderer, &end); */

    SDL_Point sp1 = sp_rotated_image(x, y, rotation, pivot);
    float angle = (rotation / 8) * 90;
    if (color == 0) {
        SDL_SetTextureColorMod(leg,224,183,194);
    } else if (color == 1) {
        SDL_SetTextureColorMod(leg,204,163,174);

    }
    //SDL_SetTextureAlphaMod(leg,50);
    //SDL_SetTextureBlendMode(leg,SDL_BLENDMODE_BLEND);
    SDL_Rect src = {.x=(rotation % 8)*48, .y=(length-1)*48, .w=48, .h=48};
    SDL_Rect dest = {.x=sp1.x, .y=sp1.y, .w=48, .h=48};
    SDL_RenderCopyEx(renderer, leg, &src,  &dest, angle, &pivot , false);


    SDL_Rect start;
    start.x = x;
    start.y = y;
    start.w = 1;
    start.h = 1;
    SDL_SetRenderDrawColor( renderer, 0, 0, 255, 155 );
    SDL_RenderFillRect( renderer, &start );

    return ep1;
}

typedef struct {
    int bone_lengths[3];
    int bone_shape[3];
} LegDefinition;


int main() {
    app_start();

    SDL_Texture* tex1 = loadTexture("resources/parts1.png");
    SDL_Texture* tex2 = loadTexture("resources/parts2.png");
    SDL_Texture* tex3 = loadTexture("resources/parts3.png");
    SDL_Texture* tex4 = loadTexture("resources/parts4.png");
    SDL_Texture* tex5 = loadTexture("resources/parts5.png");
    SDL_Texture* textures[5] = {tex1, tex2, tex3, tex4, tex5};

    bool quit = false;
    SDL_Event e;

    LegDefinition leg = {.bone_lengths={16, 16, 5}, .bone_shape={3, 2, 1}};



    // man walking downstairs  plate 15, legs
    #define FRAME_COUNT 10
    int animation[FRAME_COUNT][3] = {
        {4,13,3},
        {4,12,2},
        {5,11,2},
        {6,10,1},
        {7,8,0},
        {7,9,0},
        {6,10,0},
        {6,11,0},
        {5,12,1},
        {5,12,2},
    };




    /* // man walking upstairs plate 14, legs */
    /* #define FRAME_COUNT 12 */
    /* int animation[FRAME_COUNT][3] = { */
    /*     {4,12,1}, */
    /*     {4,11,1}, */
    /*     {5,11,0}, */
    /*     {6,10,0}, */
    /*     {6,10,0}, */
    /*     {7,10,0}, */
    /*     {8,10,0}, */
    /*     {9,10,1}, */
    /*     {8,11,2}, */
    /*     {7,11,3}, */
    /*     {6,12,3}, */
    /*     {5,12,2}, */
    /* }; */


    // ascending stairs (old man) // plate13, legs
/* #define FRAME_COUNT 12 */
/*     int animation[FRAME_COUNT][3] = { */
/*         {7,11,5}, */
/*         {6,12,4}, */
/*         {5,11,2}, */
/*         {5,10,0}, */
/*         {5,10,1}, */
/*         {6,10,1}, */
/*         {7,9,1}, */
/*         {7,9,2}, */
/*         {8,9,3}, */
/*         {8,8,4}, */
/*         {8,8,5}, */
/*         {9,9,5}, */

/*     }; */




    // walkcycle, legs
    /* #define FRAME_COUNT 10 */
/*     int animation[FRAME_COUNT][3] = { */
/*         {6,7,31}, */
/*         {7,8,0}, */
/*         {7,9,0}, */
/*         {9,9,0}, */
/*         {9,10,1}, */
/*         {10,11,2}, */
/*         {10,12,4}, */
/*         {8,12,4}, */
/*         {7,10,3}, */
/*         {6,8,0}, */
/*     }; */

    while (!quit) {
        while( SDL_PollEvent( &e ) != 0 ) {
            if( e.type == SDL_QUIT ) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) quit=true;
                if (e.key.keysym.sym == SDLK_s){
                    SDL_Surface *sshot = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
                    SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
                    IMG_SavePNG(sshot, "out.png");
                    SDL_FreeSurface(sshot);
                }
            }
        }
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);

        SDL_Point pivot = (SDL_Point){24,24};



        for (int i = 0; i < FRAME_COUNT; i++) {
            SDL_Point ep1 = (SDL_Point){100 + (i * 40), 100};
            for (int j = 0 ; j < 3; j++) {
                ep1 = draw_body_part(textures[leg.bone_shape[j]], ep1.x, ep1.y, animation[i][j], leg.bone_lengths[j], pivot, 1);
            }

        }
#if 1
         for (int i = 0; i < FRAME_COUNT; i++) {
            SDL_Point ep1 = (SDL_Point){100 + (i * 40), 104};
            for (int j = 0 ; j < 3; j++) {
                ep1 = draw_body_part(textures[leg.bone_shape[j]], ep1.x, ep1.y, animation[(i+(FRAME_COUNT/2))%FRAME_COUNT][j], leg.bone_lengths[j], pivot, 0);
            }

        }
#endif
        SDL_RenderPresent(renderer);

    }
    app_end();
}
