#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include <math.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#include "GLKVector2.h"
#pragma GCC diagnostic pop

//// reusing memory stuff


const int SCREEN_WIDTH = 1224;
const int SCREEN_HEIGHT = 1224;
const int ACTOR_COUNT = 10000;
const float METER_2_PIXEL = 24;
const float PIXEL_2_METER = 1.0f/24;
#define GRID_WIDTH 64
#define GRID_HEIGHT 64
#define MAX_IN_ACTOR_LOCATION_ARRAY 64

#include <sys/mman.h> //mmap
#define UNUSED(x) (void)(x)
#define KILOBYTES(value) ((value)*1024LL)
#define MEGABYTES(value) (KILOBYTES(value) * 1024LL)
#define GIGABYTES(value) (MEGABYTES(value) * 1024LL)
#define TERABYTES(value) (GIGABYTES(value) * 1024LL)

#define ASSERT(expression)                                                                               \
    if (!(expression)) {                                                                                 \
        printf("%s, function %s, file: %s, line:%d. \n", #expression, __FUNCTION__, __FILE__, __LINE__); \
        exit(0);                                                                                         \
    }

#define PUSH_STRUCT(arena, type) (type *) push_size_(arena, sizeof(type))

#define NEW_SLIST(List, Arena, Type)    {                   \
        Type *Sentinel = (Type *) PUSH_STRUCT(Arena, Type); \
        (List)->Sentinel = Sentinel;                        \
        (List)->Sentinel->Next = Sentinel;    }             \



#define SLIST_ADDFIRST(List, Node)              \
    (Node)->Next = (List)->Sentinel->Next;      \
    (List)->Sentinel->Next = (Node);            \

#define SLIST_EMPTY(List)                       \
    (List)->Sentinel->Next = (List)->Sentinel  \

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;
typedef bool b32;

typedef size_t memory_index;

typedef struct {
    memory_index size;
    u8 *base;
    memory_index used;
} MemoryArena;

typedef struct {
    MemoryArena arena;
} ScratchState;

typedef struct {
    b32 is_initialized;
    u32 permanent_size;
    void *permanent;
    u32 scratch_size;
    void *scratch;
    u32 debug_size;
    void *debug;
} Memory;

typedef struct {
    MemoryArena *arena;
    memory_index used;
} TempMemory;

typedef struct {
    GLKVector2 start;
    GLKVector2 end;
    float radius;
} Path;

typedef struct {
    u8 R;
    u8 G;
    u8 B;
} RGB;
typedef struct Actor{
    GLKVector2 location;
    GLKVector2 velocity;
    GLKVector2 acceleration;
    float mass;
    float max_speed;
    float max_force;
    Path path;
    RGB color;
} Actor;

typedef struct ActorItem {
    Actor *data;
    struct ActorItem *Next;
} ActorItem;

typedef struct {
    ActorItem * Sentinel;
    ActorItem * Free;
} ActorList;



typedef struct {
    ActorList grid[GRID_WIDTH][GRID_HEIGHT];
} Bins;

typedef struct {
     GLKVector2 location;
} ActorLocation;


typedef struct {
    ActorLocation locs[MAX_IN_ACTOR_LOCATION_ARRAY];
    int count;
} ActorLocationArray;

typedef struct {
    ActorLocationArray grid[GRID_WIDTH][GRID_HEIGHT];
} ArrayBins;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;
TTF_Font *font = NULL;

int example = 0;

static void *push_size_(MemoryArena *arena, memory_index size) {
    memory_index allignment = 4;
    memory_index result_pointer = (memory_index)arena->base + arena->used;
    memory_index allignment_offset = 0;
    memory_index allignment_mask = allignment - 1;

    if (result_pointer & allignment_mask) {
        allignment_offset = allignment - (result_pointer & allignment_mask);
    }

    size += allignment_offset;
    ASSERT(arena->used + size <= arena->size);
    arena->used += size;
    void *result = (void *)(result_pointer + allignment_offset);
    return result;
}

static TempMemory begin_temporary_memory(MemoryArena *arena) {
    TempMemory result = {.used = arena->used, .arena = arena};
    return result;
}

static  void end_temporary_memory(TempMemory temp) {
    MemoryArena *arena = temp.arena;
    arena->used = temp.used;
}

static void initialize_arena(MemoryArena *arena, memory_index size, u8 *base) {
    arena->size = size;
    arena->base = base;
    arena->used = 0;
}
static void initialize_memory(Memory *memory) {
    void *base_address = (void *)GIGABYTES(0);
    memory->permanent_size = MEGABYTES(32);
    memory->scratch_size = MEGABYTES(16);
    memory->debug_size = MEGABYTES(16);

    u64 total_storage_size =
        memory->permanent_size +
        memory->scratch_size +
        memory->debug_size;

    memory->permanent = mmap(base_address, total_storage_size,
                             PROT_READ | PROT_WRITE,
                             MAP_ANON | MAP_PRIVATE,
                             -1, 0);
    memory->scratch = (u8 *)(memory->permanent) + memory->permanent_size;
    memory->debug = (u8 *)(memory->scratch) + memory->scratch_size;


    memory->is_initialized = false;

    ScratchState *scratch = (ScratchState *)memory->scratch;
    initialize_arena(&scratch->arena,
                     memory->scratch_size - sizeof(ScratchState),
                     (u8 *)memory->scratch + sizeof(ScratchState));
}

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
        actor_applyForce(&actors[i], GLKVector2Make(0, 98 * METER_2_PIXEL * delta_sec * actors[i].mass));

        actors[i].velocity.x *= 0.99;
        actors[i].velocity.y *= 0.99;

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

static GLKVector2 seek_return(Actor *actor, GLKVector2 target) {
    GLKVector2 desired = GLKVector2Subtract(target, actor->location);
    if (desired.x != 0 || desired.y != 0) {
        desired = GLKVector2Normalize(desired);
    }
    desired = GLKVector2MultiplyScalar(desired, actor->max_speed);
    GLKVector2 steer = GLKVector2Subtract(desired, actor->velocity);
    steer = GLKVector2Limit(steer, actor->max_force);
    return steer;
}

static void wrap(Path *p, Actor *actor) {
    if (actor->location.x > p->end.x) {
        actor->location.x = p->start.x;
        actor->location.y = p->start.y;

    }
}

static GLKVector2 follow_path_return(Actor *actor, Path *path) {
    GLKVector2 predict = GLKVector2Make(actor->velocity.x, actor->velocity.y);
    if (predict.x != 0 || predict.y != 0 ) {
        predict = GLKVector2Normalize(predict);
    }
    predict = GLKVector2MultiplyScalar(predict, 50); // will predict 50 frames into future.
    GLKVector2 predict_pos = GLKVector2Add(actor->location, predict);

    //look at line segment:
    GLKVector2 a = GLKVector2Make(path->start.x, path->start.y);
    GLKVector2 b = GLKVector2Make(path->end.x,   path->end.y);
    printf("path: %f,%f -> %f,%f\n ",path->start.x, path->start.y,path->end.x,   path->end.y);
    GLKVector2 normal_point = get_normal_point(predict_pos, a, b);
    //printf("normla point: %f, %f\n", normal_point.x, normal_point.y);

    GLKVector2 dir = GLKVector2Subtract(b, a);
    if (dir.x != 0 || dir.y != 0) {
        dir = GLKVector2Normalize(dir);
    }
    dir = GLKVector2MultiplyScalar(dir, GLKVector2Length(actor->velocity)); // this 10 could be based on velocity instead a magic number.

    GLKVector2 target = GLKVector2Add(normal_point, dir);
    float distance = GLKVector2Distance(predict_pos, normal_point);

    GLKVector2 steer = GLKVector2Make(0,0);
    if (distance > path->radius) {
        //seek(&actors[i], target);
        GLKVector2 desired = GLKVector2Subtract(target, actor->location);
        if (desired.x != 0 || desired.y != 0) {
            desired = GLKVector2Normalize(desired);
        }
        desired = GLKVector2MultiplyScalar(desired, actor->max_speed);
        steer = GLKVector2Subtract(desired, actor->velocity);
        steer = GLKVector2Limit(steer, actor->max_force);
    }
    return steer;
}

static void follow_path_no_separation(Actor *actors, Path *path, bool mouse_left_down, bool mouse_right_down ) {
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
            GLKVector2 a = GLKVector2Make(path->start.x, path->start.y);
            GLKVector2 b = GLKVector2Make(path->end.x,   path->end.y);
            GLKVector2 normal_point = get_normal_point(predict_pos, a, b);
            //printf("normla point: %f, %f\n", normal_point.x, normal_point.y);

            GLKVector2 dir = GLKVector2Subtract(b, a);
            if (dir.x != 0 || dir.y != 0) {
                dir = GLKVector2Normalize(dir);
            }
            dir = GLKVector2MultiplyScalar(dir, GLKVector2Length(actors[i].velocity)); // this 10 could be based on velocity instead a magic number.

            GLKVector2 target = GLKVector2Add(normal_point, dir);
            float distance = GLKVector2Distance(predict_pos, normal_point);

            if (distance > path->radius) {
                seek(&actors[i], target);
            }

             actors[i].velocity = GLKVector2Add(actors[i].velocity, actors[i].acceleration);
             actors[i].location = GLKVector2Add(actors[i].location, actors[i].velocity);
             actors[i].acceleration = GLKVector2MultiplyScalar(actors[i].acceleration, 0);

             wrap(path, &actors[i]);

        }
}

static GLKVector2 separate(Actor *actor, Actor *group) {
    float desired_separation = 12;
    GLKVector2 sum = GLKVector2Make(0,0);
    int count = 0;

    for (int i = 0; i < ACTOR_COUNT; i++) {
        float d = GLKVector2DistanceSquared(actor->location, group[i].location);
        //printf("i: %d, d: %f\n",i, d);
        if ((d>0) && (d < (desired_separation * desired_separation))) {
            GLKVector2 diff = GLKVector2Subtract(actor->location, group[i].location);
            if (diff.x != 0 || diff.y != 0) {
                diff = GLKVector2Normalize(diff);
            }
            diff = GLKVector2DivideScalar(diff, d);
            sum = GLKVector2Add(sum, diff);
            count++;
        }
    }

    // then average it.
    if (count > 0) {
        sum = GLKVector2DivideScalar(sum, count);
        if (sum.x != 0 || sum.y != 0) {
            sum = GLKVector2Normalize(sum);
        }
        sum = GLKVector2MultiplyScalar(sum, actor->max_speed);

        sum = GLKVector2Subtract(sum, actor->velocity);
        sum = GLKVector2Limit(sum, actor->max_force);

    }
    return sum;
}

static GLKVector2 separate_bins(Actor *actor, Bins *bins) {
    float desired_separation = 24;
    GLKVector2 sum = GLKVector2Make(0,0);
    int count = 0;

    int cell_x = actor->location.x / (SCREEN_WIDTH / GRID_WIDTH);
    int cell_y = actor->location.y / (SCREEN_HEIGHT / GRID_HEIGHT);

    //printf("\n");
    for (int nx = -1; nx < 2; nx++) {
        int this_x = nx + cell_x;
        if (this_x < 0 || this_x == GRID_WIDTH) continue;
        for (int ny = -1; ny < 2; ny++) {
            int this_y = ny + cell_y;
            if (this_y < 0 || this_y == GRID_HEIGHT) continue;

            ActorList *list = &(bins->grid)[this_x][this_y];
            for (ActorItem *node = list->Sentinel->Next;
                 node != list->Sentinel;
                 node = node->Next) {

                float d = GLKVector2DistanceSquared(actor->location, node->data->location);
                if ((d>0) && (d < (desired_separation * desired_separation))) {
                    GLKVector2 diff = GLKVector2Subtract(actor->location, node->data->location);
                    if (diff.x != 0 || diff.y != 0) {
                        diff = GLKVector2Normalize(diff);
                    }
                    diff = GLKVector2DivideScalar(diff, d);
                    sum = GLKVector2Add(sum, diff);
                    count++;
                    if (count > 3) continue;
                }
            }
        }
    }

    // then average it.
    if (count > 0) {
        sum = GLKVector2DivideScalar(sum, count);
        if (sum.x != 0 || sum.y != 0) {
            sum = GLKVector2Normalize(sum);
        }
        sum = GLKVector2MultiplyScalar(sum, actor->max_speed);

        sum = GLKVector2Subtract(sum, actor->velocity);
        sum = GLKVector2Limit(sum, actor->max_force);

    }
    return sum;

}




static GLKVector2 separate_abins(Actor *actor, ArrayBins abins) {
    float desired_separation = 12;
    GLKVector2 sum = GLKVector2Make(0,0);
    int count = 0;

    int cell_x = actor->location.x / (SCREEN_WIDTH / GRID_WIDTH);
    int cell_y = actor->location.y / (SCREEN_HEIGHT / GRID_HEIGHT);

    for (int nx = -1; nx < 2; nx++) {
        int this_x = nx + cell_x;
        if (this_x < 0 || this_x == GRID_WIDTH) continue;
        for (int ny = -1; ny < 2; ny++) {
            int this_y = ny + cell_y;
            if (this_y < 0 || this_y == GRID_HEIGHT) continue;

            for (int i = 0; i < (abins.grid)[this_x][this_y].count; i++) {
                GLKVector2 loc = (abins.grid)[this_x][this_y].locs[i].location;

                float d = GLKVector2DistanceSquared(actor->location, loc);
                if ((d>0) && (d < (desired_separation * desired_separation))) {
                    GLKVector2 diff = GLKVector2Subtract(actor->location, loc);
                    if (diff.x != 0 || diff.y != 0) {
                        diff = GLKVector2Normalize(diff);
                    }
                    diff = GLKVector2DivideScalar(diff, d);
                    sum = GLKVector2Add(sum, diff);
                    count++;
                    if (count > 3) continue;
                }
            }
        }
    }

    // then average it.
    if (count > 0) {
        sum = GLKVector2DivideScalar(sum, count);
        if (sum.x != 0 || sum.y != 0) {
            sum = GLKVector2Normalize(sum);
        }
        sum = GLKVector2MultiplyScalar(sum, actor->max_speed);

        sum = GLKVector2Subtract(sum, actor->velocity);
        sum = GLKVector2Limit(sum, actor->max_force);

    }
    return sum;

}

#define PI 3.14159265358979323846


int main() {
#ifdef __SSE2__
    printf("SSE2 found!\n");
#endif
    UNUSED(forces);
    UNUSED(seekExample);
    UNUSED(flee);
    UNUSED(arrive);
    UNUSED(steer_within_walls);
    UNUSED(follow_path_no_separation);
    UNUSED(separate);
    UNUSED(separate_bins);
    UNUSED(separate_abins);

    Memory _memory;
    Memory *memory = &_memory;
    initialize_memory(memory);
    ScratchState *scratch = (ScratchState *)memory->scratch;


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

    Bins bins;
    ArrayBins abins;


    for (int x = 0; x< GRID_WIDTH; x++) {
        for (int y = 0; y< GRID_HEIGHT; y++) {
            NEW_SLIST(&(bins.grid)[x][y], &scratch->arena, ActorItem);
        }
    }

    for (int i=0; i< ACTOR_COUNT; i++) {
        actors[i].location     = GLKVector2Make(612 + 400 * cos(((PI*2)/ACTOR_COUNT)*i), 612 + 400 * sin(((PI*2)/ACTOR_COUNT)*i));//GLKVector2Make((rand() % (SCREEN_WIDTH)), (rand() % (SCREEN_HEIGHT)));
        actors[i].velocity     = GLKVector2Make(0,0);
        actors[i].acceleration = GLKVector2Make(0,0);
        actors[i].mass = 1.0f;
        actors[i].max_speed = 5.0f;//+(rand() % 5)*1.0f;
        actors[i].color = (RGB){.R=rand()%0xff, .G=rand()%0xff, .B=rand()%0xff};
        actors[i].max_force = 0.3f;
        actors[i].path = (Path){.start=GLKVector2Make(612 + 512 * cos(((PI*2)/ACTOR_COUNT)*i), 612 + 512 * sin(((PI*2)/ACTOR_COUNT)*i) ),
            .end=GLKVector2Make(612 + -512 * cos(((PI*2)/ACTOR_COUNT)*i), 612 + -512 * sin(((PI*2)/ACTOR_COUNT)*i)),
            .radius=20};
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

        // clear memory
        TempMemory temp_mem = begin_temporary_memory(&scratch->arena);

        for (int x = 0; x< GRID_WIDTH; x++) {
            for (int y = 0; y< GRID_HEIGHT; y++) {
                SLIST_EMPTY(&(bins.grid)[x][y]);
            }
        }

        for (int x = 0; x< GRID_WIDTH; x++) {
            for (int y = 0; y< GRID_HEIGHT; y++) {
                abins.grid[x][y].count = 0;
            }

        }

        for (int i =0 ; i < ACTOR_COUNT ; i++) {
            int cell_x = actors[i].location.x / (SCREEN_WIDTH / GRID_WIDTH);
            int cell_y = actors[i].location.y / (SCREEN_HEIGHT / GRID_HEIGHT);
            /* if (cell_x < 0) cell_x = 0; */
            /* if (cell_x >= GRID_WIDTH) cell_x = GRID_WIDTH-1; */
            /* if (cell_y < 0) cell_y = 0; */
            /* if (cell_y > GRID_HEIGHT ) cell_y = GRID_HEIGHT-1; */


            ASSERT(cell_x < GRID_WIDTH);
            ASSERT(cell_y < GRID_HEIGHT);
            ASSERT(cell_x >= 0);
            ASSERT(cell_y >= 0);

            ActorItem *item = PUSH_STRUCT(&scratch->arena, ActorItem);
            item->data = &actors[i];
            SLIST_ADDFIRST(&(bins.grid)[cell_x][cell_y], item);

            if (abins.grid[cell_x][cell_y].count < MAX_IN_ACTOR_LOCATION_ARRAY) {
                abins.grid[cell_x][cell_y].locs[abins.grid[cell_x][cell_y].count].location = GLKVector2Make(actors[i].location.x, actors[i].location.y);
                abins.grid[cell_x][cell_y].count++;
            }
        }

        for (int i =0 ; i< ACTOR_COUNT ; i++) {
            //GLKVector2 separate_force = separate(&actors[i], actors);
            GLKVector2 separate_force = separate_abins(&actors[i], abins);
            //GLKVector2 separate_force = separate_bins(&actors[i], &bins);
            //GLKVector2 seek_force  = seek_return(&actors[i], GLKVector2Make(mouseX, mouseY));
            GLKVector2 seek_force  = seek_return(&actors[i], actors[i].path.end);
            //GLKVector2 seek_force  = follow_path_return(&actors[i], &actors[i].path);


            separate_force = GLKVector2MultiplyScalar(separate_force, 3);
            seek_force = GLKVector2MultiplyScalar(seek_force, 1);

            actor_applyForce(&actors[i], separate_force);
            actor_applyForce(&actors[i], seek_force);

            actors[i].velocity = GLKVector2Add(actors[i].velocity, actors[i].acceleration);
            actors[i].velocity = GLKVector2Limit(actors[i].velocity, actors[i].max_speed);
            actors[i].location = GLKVector2Add(actors[i].location, actors[i].velocity);
            actors[i].acceleration = GLKVector2MultiplyScalar(actors[i].acceleration, 0);


            float d = GLKVector2Distance(actors[i].location, actors[i].path.end);
            if (d < 20) {
                //printf("swapping path end and begin: %f\n", d);
                GLKVector2 temp = actors[i].path.start;
                actors[i].path.start = actors[i].path.end;
                actors[i].path.end = temp;

            }
            //


            wrap_around_actors_in_screen(&actors[i]);
        }

        end_temporary_memory(temp_mem);
        // end memory


        Uint64 after = SDL_GetPerformanceCounter();


        SDL_SetRenderDrawColor(renderer, 0xFF, 0xAA, 0xFF, 0xFF);

        // draw actors
        for (int i =0 ; i< ACTOR_COUNT ; i++) {
            DestR.x = actors[i].location.x;
            DestR.y = actors[i].location.y;
            DestR.w = actors[i].mass * 12;
            DestR.h = actors[i].mass * 12;
            //SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_SetRenderDrawColor(renderer, actors[i].color.R, actors[i].color.G, actors[i].color.B, 255);
            //SDL_RenderCopy(renderer, texture, &SrcR,  &DestR);
            SDL_RenderFillRect(renderer  , &DestR);
            //SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            //SDL_RenderDrawLine(renderer, actors[i].path.start.x, actors[i].path.start.y, actors[i].path.end.x, actors[i].path.end.y);
        }

        float update = (float)(after - before)/SDL_GetPerformanceFrequency();

        //// frame time text

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
        //SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        //SDL_RenderDrawLine(renderer, path.start.x, path.start.y, path.end.x, path.end.y);
        // end 'path'
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xAA, 0xFF, 0xFF);

        delta_sec = update;
        SDL_RenderPresent(renderer);

    }
    close_app();
}
