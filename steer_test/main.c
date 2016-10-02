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

int example = 0;



const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 1024;
const int ACTOR_COUNT = 10;
const float METER_2_PIXEL = 24;
const float PIXEL_2_METER = 1.0f/24;


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
    float mass;
    float max_speed;
    float max_force;
} Actor;



static void wrap_around_actors_in_screen(Actor *actor) {
    const int actor_width = actor->mass * 24;
    const int actor_height = actor->mass * 24;

    if (actor->location.x+ (actor_width/2) < 0) actor->location.x = SCREEN_WIDTH- (actor_width/2);
    if (actor->location.x+ (actor_width/2) > SCREEN_WIDTH) actor->location.x = - (actor_width/2);
    if (actor->location.y+ (actor_height/2) < 0) actor->location.y = SCREEN_HEIGHT-(actor_height/2);
    if (actor->location.y+(actor_height/2) > SCREEN_HEIGHT) actor->location.y = -(actor_height/2);
}

static void bounce_agains_edges(Actor *actor) {
    if (actor->location.x > SCREEN_WIDTH) {
        actor->location.x = SCREEN_WIDTH;
        actor->velocity.x *= -1;
    } else if (actor->location.x < 0) {
        actor->location.x = 0;
        actor->velocity.x *= -1;
    }

    if (actor->location.y > SCREEN_HEIGHT) {
        actor->location.y = SCREEN_HEIGHT;
        actor->velocity.y *= -1;

    }

}
static void actor_applyForce(Actor *a, GLKVector2 force) {
    force = GLKVector2DivideScalar(force, a->mass);
    a->acceleration = GLKVector2Add(a->acceleration, force);
}

static void forces(Actor *actors, float delta_sec, bool mouse_left_down, bool mouse_right_down){
    for (int i =0 ; i< ACTOR_COUNT ; i++) {
        // apply gravity

        float c = 0.02f;


        actor_applyForce(&actors[i], GLKVector2Make(0, 98 * METER_2_PIXEL * delta_sec * actors[i].mass));

        actors[i].velocity.x *= 0.99;
        actors[i].velocity.y *= 0.99;

        /* if (actors[i].velocity.x != 0 || actors[i].velocity.y != 0 ) { */
        /*     // if velocity = 0,0 normlaize will give infinity and nan on next run */
        /*     GLKVector2 friction =  GLKVector2Make(actors[i].velocity.x, actors[i].velocity.y); */
        /*     friction = GLKVector2MultiplyScalar(friction, -1.0f); */
        /*     friction = GLKVector2Normalize(friction); */
        /*     friction = GLKVector2MultiplyScalar(friction, c); */
        /*     actor_applyForce(&actors[i], friction); */
        /* } */

        if (mouse_left_down) {
            actor_applyForce(&actors[i], GLKVector2Make(0.1, 0));
        }
        if (mouse_right_down) {
            actor_applyForce(&actors[i], GLKVector2Make(-0.1, 0));
        }
        actors[i].velocity = GLKVector2Add(actors[i].velocity, actors[i].acceleration);
        actors[i].location = GLKVector2Add(actors[i].location, actors[i].velocity);
        actors[i].acceleration = GLKVector2MultiplyScalar(actors[i].acceleration, 0);

        bounce_agains_edges(&actors[i]);
        //wrap_around_actors_in_screen(&actors[i]);
    }
}

static __inline__ GLKVector2 GLKVector2Limit(GLKVector2 vector, float max) {
    if (GLKVector2Length(vector) > max) {
        return GLKVector2MultiplyScalar(GLKVector2Normalize(vector),max);
    } else {
        return  vector;
    }
}
static __inline__ float map(float input, float input_start, float input_end, float output_start, float output_end) {
    return  output_start + ((output_end - output_start) / (input_end - input_start)) * (input - input_start);
}

static void seekExample(Actor *actors, int mouseX, int mouseY) {
    GLKVector2 target = GLKVector2Make(mouseX, mouseY);
    for (int i =0 ; i< ACTOR_COUNT ; i++) {
        GLKVector2 desired = GLKVector2Subtract(target, actors[i].location);
        desired = GLKVector2Normalize(desired);
        desired = GLKVector2MultiplyScalar(desired, actors[i].max_speed);
        GLKVector2 steer = GLKVector2Subtract(desired, actors[i].velocity);
        steer = GLKVector2Limit(steer, actors[i].max_force);
        actor_applyForce(&actors[i], steer);

        actors[i].velocity = GLKVector2Add(actors[i].velocity, actors[i].acceleration);
        actors[i].location = GLKVector2Add(actors[i].location, actors[i].velocity);
        actors[i].acceleration = GLKVector2MultiplyScalar(actors[i].acceleration, 0);
    }

}

static void flee(Actor *actors, int mouseX, int mouseY) {
    GLKVector2 target = GLKVector2Make(mouseX, mouseY);
    for (int i =0 ; i< ACTOR_COUNT ; i++) {
        GLKVector2 desired = GLKVector2Subtract(target, actors[i].location);
        desired = GLKVector2Normalize(desired);
        desired = GLKVector2MultiplyScalar(desired, actors[i].max_speed);
        GLKVector2 steer = GLKVector2Subtract(desired, actors[i].velocity);
        steer = GLKVector2Limit(steer, actors[i].max_force);
        steer = GLKVector2MultiplyScalar(steer, -1);
        actor_applyForce(&actors[i], steer);


        actors[i].velocity = GLKVector2Add(actors[i].velocity, actors[i].acceleration);
        actors[i].location = GLKVector2Add(actors[i].location, actors[i].velocity);
        actors[i].acceleration = GLKVector2MultiplyScalar(actors[i].acceleration, 0);
    }
}

static void arrive(Actor *actors, int mouseX, int mouseY) {
    GLKVector2 target = GLKVector2Make(mouseX, mouseY);
    for (int i =0 ; i< ACTOR_COUNT ; i++) {
        GLKVector2 desired = GLKVector2Subtract(target, actors[i].location);
        float d = GLKVector2Length(desired);
        if (d != 0.0f) {
            desired = GLKVector2Normalize(desired);
        }
        if (d < 100) {
            float m = map(d, 0, 100, 0, actors[i].max_speed);
            desired = GLKVector2MultiplyScalar(desired, m);
        } else {
            desired = GLKVector2MultiplyScalar(desired,  actors[i].max_speed);
        }

        GLKVector2 steer = GLKVector2Subtract(desired, actors[i].velocity);
        steer = GLKVector2Limit(steer, actors[i].max_force);
        actor_applyForce(&actors[i], steer);

        actors[i].velocity = GLKVector2Add(actors[i].velocity, actors[i].acceleration);
        actors[i].location = GLKVector2Add(actors[i].location, actors[i].velocity);
        actors[i].acceleration = GLKVector2MultiplyScalar(actors[i].acceleration, 0);
    }
}

static void steer_within_walls(Actor *actors) {
    for (int i =0 ; i< ACTOR_COUNT ; i++) {
        GLKVector2 desired;
        bool wall_check = false;
        float d = 50;
        if (actors[i].location.x < d) {
            wall_check = true;
            desired = GLKVector2Make(actors[i].max_speed, actors[i].velocity.y);
        } else if (actors[i].location.x > SCREEN_WIDTH -d) {
            wall_check = true;
            desired = GLKVector2Make(-actors[i].max_speed, actors[i].velocity.y);
        }

        if (actors[i].location.y < d) {
            wall_check = true;
            desired = GLKVector2Make(actors[i].velocity.x, actors[i].max_speed);
        } else if (actors[i].location.y > SCREEN_HEIGHT-d) {
            wall_check = true;
            desired = GLKVector2Make(actors[i].velocity.x, -actors[i].max_speed);
        }

        if (wall_check) {
            desired = GLKVector2Normalize(desired);
            desired = GLKVector2MultiplyScalar(desired, actors[i].max_speed);
            GLKVector2 steer = GLKVector2Subtract(desired, actors[i].velocity);
            steer = GLKVector2Limit(steer, actors[i].max_force);
            actor_applyForce(&actors[i], steer);
        }




        actors[i].velocity = GLKVector2Add(actors[i].velocity, actors[i].acceleration);
        actors[i].location = GLKVector2Add(actors[i].location, actors[i].velocity);
        actors[i].acceleration = GLKVector2MultiplyScalar(actors[i].acceleration, 0);
    }
}

static GLKVector2 get_normal_point(GLKVector2 p, GLKVector2 a, GLKVector2 b) {
    GLKVector2 ap = GLKVector2Subtract(p, a);
    GLKVector2 ab = GLKVector2Subtract(b, a);
    if (ab.x != 0 || ab.y != 0) {
        ab = GLKVector2Normalize(ab);
    }
    ab = GLKVector2MultiplyScalar(ab, GLKVector2DotProduct(ap, ab));
    GLKVector2 normal_point = GLKVector2Add(a, ab);
    return normal_point;

}


typedef struct {
    GLKVector2 start;
    GLKVector2 end;
    float radius;

} Path;


static void seek(Actor *actor, GLKVector2 target) {
    GLKVector2 desired = GLKVector2Subtract(target, actor->location);

    if (GLKVector2Length(desired) == 0) return;

    if (desired.x != 0 || desired.y != 0) {
        desired = GLKVector2Normalize(desired);
    }
    desired = GLKVector2MultiplyScalar(desired, actor->max_speed);

    GLKVector2 steer = GLKVector2Subtract(desired, actor->velocity);
    steer = GLKVector2Limit(steer, actor->max_force);
    actor_applyForce(actor, steer);
}


static void wrap(Path *p, Actor *actor) {
    if (actor->location.x > p->end.x) {
        actor->location.x = p->start.x;
        actor->location.y = p->start.y;

    }
}


int main() {
    init();
    srand(time(NULL));
    bool quit = false;
    SDL_Event e;
    bool mouse_left_down = false;
    bool mouse_right_down = false;
    int mouseX, mouseY;


    SDL_Rect SrcR = {.x=0, .y=0, .w=24, .h=24};
    SDL_Rect DestR = {.x=100, .y=100, .w=24, .h=24};


    Actor actors[ACTOR_COUNT];
    float delta_sec = 0;

    Path path;
    path.start = GLKVector2Make(100,100);
    path.end = GLKVector2Make(800,800);
    path.radius = 20;

    for (int i=0; i< ACTOR_COUNT; i++) {
        actors[i].location     = GLKVector2Make((rand() % (SCREEN_WIDTH)), (rand() % (SCREEN_HEIGHT)));
        actors[i].velocity     = GLKVector2Make(0,0);
        actors[i].acceleration = GLKVector2Make(0,0);
        actors[i].mass = 1.0f;
        actors[i].max_speed = 1.0f+(rand() % 5)*1.0f;
        actors[i].max_force = 0.3f;//1.0f+(rand() % 5)*1.0f;
    }


    while (!quit) {

        SDL_GetMouseState(&mouseX, &mouseY);

        while( SDL_PollEvent( &e ) != 0 ) {
            if( e.type == SDL_QUIT ) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) quit=true;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    mouse_left_down = true;
                }
                if (e.button.button == SDL_BUTTON_RIGHT) {
                    mouse_right_down = true;
                }
            }
            if (e.type == SDL_MOUSEBUTTONUP) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    mouse_left_down = false;
                }
                if (e.button.button == SDL_BUTTON_RIGHT) {
                    mouse_right_down = false;
                }
            }
        }
        //printf("mouse: %d,%d\n",mouseX,mouseY);
        SDL_RenderClear(renderer);

        Uint64 before = SDL_GetPerformanceCounter();

        for (int i =0 ; i< ACTOR_COUNT ; i++) {

            if (mouse_left_down) {
                actor_applyForce(&actors[i], GLKVector2Make(1, 0));
            }
            if (mouse_right_down) {
                actor_applyForce(&actors[i], GLKVector2Make(-1, 0));
            }


            GLKVector2 predict = GLKVector2Make(actors[i].velocity.x, actors[i].velocity.y);
            if (predict.x != 0 || predict.y != 0 ) {
                predict = GLKVector2Normalize(predict);
            }
            predict = GLKVector2MultiplyScalar(predict, 50); // will predict 50 frames into future.
            GLKVector2 predict_pos = GLKVector2Add(actors[i].location, predict);

            //look at line segment:
            GLKVector2 a = GLKVector2Make(path.start.x, path.start.y);
            GLKVector2 b = GLKVector2Make(path.end.x,   path.end.y);
            GLKVector2 normal_point = get_normal_point(predict_pos, a, b);
            //printf("normla point: %f, %f\n", normal_point.x, normal_point.y);

            GLKVector2 dir = GLKVector2Subtract(b, a);
            if (dir.x != 0 || dir.y != 0) {
                dir = GLKVector2Normalize(dir);
            }
            dir = GLKVector2MultiplyScalar(dir, GLKVector2Length(actors[i].velocity)); // this 10 could be based on velocity instead a magic number.

            GLKVector2 target = GLKVector2Add(normal_point, dir);
            float distance = GLKVector2Distance(predict_pos, normal_point);

            if (distance > path.radius) {
                seek(&actors[i], target);
            }

             actors[i].velocity = GLKVector2Add(actors[i].velocity, actors[i].acceleration);
             actors[i].location = GLKVector2Add(actors[i].location, actors[i].velocity);
             actors[i].acceleration = GLKVector2MultiplyScalar(actors[i].acceleration, 0);

             wrap(&path, &actors[i]);

        }

        Uint64 after = SDL_GetPerformanceCounter();


        SDL_SetRenderDrawColor(renderer, 0xFF, 0xAA, 0xFF, 0xFF);

        // draw actors
        for (int i =0 ; i< ACTOR_COUNT ; i++) {
            DestR.x = actors[i].location.x;
            DestR.y = actors[i].location.y;
            DestR.w = actors[i].mass * 24;
            DestR.h = actors[i].mass * 24;
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

        // 'path'
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawLine(renderer, path.start.x, path.start.y, path.end.x, path.end.y);
        // end 'path'

        SDL_SetRenderDrawColor(renderer, 0xFF, 0xAA, 0xFF, 0xFF);

        delta_sec = update;
        SDL_RenderPresent(renderer);

    }
    close_app();
}
