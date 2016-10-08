#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "c89spec.h"
#include "../src/states.h"
#include "../src/pathfind.h"
#include "../src/memory.h"
#include "../src/level.h"
#include "../src/random.h"
#include "../src/data_structures.h"

bool block_at_xyz_is(int x, int y, int z, Block b, LevelData * level) {
    return (level->blocks[FLATTEN_3D_INDEX(x,y,z, level->x, level->y)].object == b);
}

bool four_diagonals_are_jumps(Grid *grid, int x, int y, int z) {
    return (GetNodeAt(grid, x-1, y-1, z)->isJumpNode &&
            GetNodeAt(grid, x-1, y+1, z)->isJumpNode &&
            GetNodeAt(grid, x+1, y+1, z)->isJumpNode &&
            GetNodeAt(grid, x+1, y-1, z)->isJumpNode);
}
bool four_cardinals_are_jumps(Grid *grid, int x, int y, int z) {
    return (GetNodeAt(grid, x-1, y, z)->isJumpNode &&
            GetNodeAt(grid, x-1, y, z)->isJumpNode &&
            GetNodeAt(grid, x, y+1, z)->isJumpNode &&
            GetNodeAt(grid, x, y-1, z)->isJumpNode);
}

int total_jumppoints(Grid *grid) {
    int result = 0;
    for (int z = 0; z < grid->depth; z++) {
        for (int y = 0; y < grid->height; y++) {
            for (int x = 0; x< grid->width; x++) {
                if (GetNodeAt(grid, x, y, z)->isJumpNode) result++;
            }
        }
    }
    return result;
}


#define TEST_RUN_PATHFINDER(times) {                                    \
        make_level_str(permanent, &permanent->level , size, string);    \
        permanent->grid = PUSH_STRUCT(&permanent->arena, Grid);         \
        init_grid(permanent->grid, &permanent->arena, &permanent->level); \
        preprocess_grid(permanent->grid);                               \
        TempMemory temp_mem = begin_temporary_memory(&scratch->arena);  \
        for (int i = 0; i < times; i++) {                               \
            grid_node * Start = get_random_walkable_node(permanent->grid); \
            grid_node * End = get_random_walkable_node(permanent->grid); \
            path_list * Path = FindPathPlus(Start, End, permanent->grid, &scratch->arena); \
            if (!Path) printf("FAILED from: %d,%d,%d to %d,%d,%d\n", Start->X, Start->Y, Start->Z, End->X, End->Y, End->Z); \
            expect(Path);                                               \
            for (int i = 0; i < permanent->grid->width * permanent->grid->height * permanent->grid->depth;i++) { \
                permanent->grid->nodes[i].f = 0;                        \
                permanent->grid->nodes[i].g = 0;                        \
                permanent->grid->nodes[i].opened = 0;                   \
                permanent->grid->nodes[i].closed = 0;                   \
                permanent->grid->nodes[i].Next = NULL;                  \
                permanent->grid->nodes[i].parent = NULL;                \
                                                                        \
            }                                                           \
            end_temporary_memory(temp_mem);                             \
        }                                                               \
    }                                                                   \

#define INIT_MEMORY_PATHFINDER()                                        \
    Memory _memory;                                                     \
    Memory *memory = &_memory;                                          \
    initialize_memory(memory);                                          \
    PermanentState *permanent = (PermanentState *)memory->permanent;    \
    ScratchState *scratch = (ScratchState *)memory->scratch;            \
    UNUSED(scratch);UNUSED(permanent);                                  \


grid_node* get_random_walkable_node(Grid *grid) {
    grid_node *result;// = GetNodeAt(grid, 0,0,0);
    do {
        result = GetNodeAt(grid, rand_int(grid->width), rand_int(grid->height), rand_int(grid->depth));
    } while (!result->walkable);
    return result;
}


void initialize_memory(Memory *memory) {
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

    ASSERT(sizeof(PermanentState) <= memory->permanent_size);
    PermanentState *permanent = (PermanentState *)memory->permanent;
    ASSERT(sizeof(ScratchState) <= memory->scratch_size);
    ScratchState *scratch = (ScratchState *)memory->scratch;
    ASSERT(sizeof(DebugState) <= memory->debug_size);
    DebugState *debug = (DebugState *)memory->debug;

    initialize_arena(&permanent->arena,
                     memory->permanent_size - sizeof(PermanentState),
                     (u8 *)memory->permanent + sizeof(PermanentState));

    initialize_arena(&scratch->arena,
                     memory->scratch_size - sizeof(ScratchState),
                     (u8 *)memory->scratch + sizeof(ScratchState));

    initialize_arena(&debug->arena,
                     memory->debug_size - sizeof(DebugState),
                     (u8 *)memory->debug + sizeof(DebugState));
}



describe(memory) {
    it (memory can be initialized) {
        Memory _memory;
        Memory *memory = &_memory;

        initialize_memory(memory);
        PermanentState *permanent = (PermanentState *)memory->permanent;
        ScratchState *scratch = (ScratchState *)memory->scratch;
        DebugState *debug = (DebugState *)memory->debug;
        expect(&permanent->arena.size > 0);
        expect(&scratch->arena.size > 0);
        expect(&debug->arena.size > 0);

    }
}

typedef struct ListItem {
    int value;
    struct ListItem * Next;
} ListItem;

typedef struct {
    ListItem * Sentinel;
} List;

describe(slist) {
    it(can be reset using end temporary memory) {
        Memory _memory;
        Memory *memory = &_memory;

        initialize_memory(memory);
        ScratchState *scratch = (ScratchState *)memory->scratch;
        expect(&scratch->arena.size > 0);

        List l;
        NEW_SLIST(&l, &scratch->arena, ListItem);

        TempMemory temp_mem = begin_temporary_memory(&scratch->arena);

        int test_total = 0;
        for (int i = 0; i < 10; i++) {
            ListItem *item = PUSH_STRUCT(&scratch->arena, ListItem);
            item->value = 2;
            SLIST_ADDFIRST(&l, item);
            test_total += 2;
        }

        int total = 0;

        ListItem * node = l.Sentinel;

        do {
            total += node->value;
            node = node->Next;
        } while(node != l.Sentinel);

        expect(total == test_total);

        int for_loop_total = 0;
        for (ListItem * node = l.Sentinel->Next; node != l.Sentinel; node = node->Next) {
            for_loop_total += node->value;
        }
        expect(for_loop_total == total);


        end_temporary_memory(temp_mem);

        SLIST_EMPTY(&l);

        test_total = 0;
        total = 0;
        for (int i = 0; i < 10; i++) {
            ListItem *item = PUSH_STRUCT(&scratch->arena, ListItem);
            item->value = 4;
            SLIST_ADDFIRST(&l, item);
            test_total += 4;
        }

        node = l.Sentinel;

        do {
            total += node->value;
            node = node->Next;
        } while(node != l.Sentinel);

        expect(total == test_total);
    }
}

describe(leveldata) {
    it (can be read from a string) {
        INIT_MEMORY_PATHFINDER();
        World_Size size = (World_Size){5,1,0};

        char string[] =
            "+-----+\n"
            "|..#..|\n"
            "+-----+";
        read_level_str(permanent, &permanent->level , size, string);

        for (int i = 0; i < 5; i++) {
            if (i != 2) {
                expect(block_at_xyz_is(i,0,0, Floor, &permanent->level));
            } else {
                expect(block_at_xyz_is(i,0,0, WallBlock, &permanent->level));
            }
        }
    }
    it (can create a multifloor level with stairs) {
        INIT_MEMORY_PATHFINDER();
        World_Size size = (World_Size){6,1,2};

        char string[] =
            "+------+\n"
            "|.U===.|\n"
            "+------+\n"
            "|......|\n"
            "+------+";

        make_level_str(permanent, &permanent->level , size, string);
        expect(block_at_xyz_is(1,0,0, StairsUp1E, &permanent->level));
        expect(block_at_xyz_is(4,0,0, StairsUp4E, &permanent->level));
    }
}

describe(grid_preprocessor) {
    it (places jump points around a wall) {

        INIT_MEMORY_PATHFINDER();
        World_Size size = (World_Size){6,3,1};
        char string[] =
            "+------+\n"
            "|......|\n"
            "|..##..|\n"
            "|......|\n"
            "+------+";
        /*
          "+------+\n"
          "|.J..J.|\n"
          "|..##..|\n"
          "|.J..J.|\n"
          "+------+";
         */

        make_level_str(permanent, &permanent->level , size, string);
        permanent->grid = PUSH_STRUCT(&permanent->arena, Grid);
        init_grid(permanent->grid, &permanent->arena, &permanent->level);
        expect(permanent->grid->width == 6 && permanent->grid->height == 3);
        preprocess_grid(permanent->grid);
        expect(GetNodeAt(permanent->grid, 1, 0, 0)->isJumpNode);
        expect(GetNodeAt(permanent->grid, 4, 0, 0)->isJumpNode);
        expect(GetNodeAt(permanent->grid, 1, 2, 0)->isJumpNode);
        expect(GetNodeAt(permanent->grid, 4, 2, 0)->isJumpNode);
    }
    it (places jumppoints at wallcorners and void corners) {
        Memory _memory;
        Memory *memory = &_memory;

        initialize_memory(memory);
        PermanentState *permanent = (PermanentState *)memory->permanent;
        World_Size size = (World_Size){12,11,1};
        char string[] =
            "+------------+\n"
            "|............|\n"
            "|.##########.|\n"
            "|.#........#.|\n"
            "|.#........#.|\n"
            "|.#...  ...#.|\n"
            "|.#...  ...#.|\n"
            "|.#...  ...#.|\n"
            "|.#........#.|\n"
            "|.#........#.|\n"
            "|.##########.|\n"
            "|............|\n"
            "+------------+";

        /*
          "+------------+\n"
          "|J..........J|\n"
          "|.##########.|\n"
          "|.#........#.|\n"
          "|.#..J..J..#.|\n"
          "|.#...  ...#.|\n"
          "|.#...  ...#.|\n"
          "|.#...  ...#.|\n"
          "|.#..J..J..#.|\n"
          "|.#........#.|\n"
          "|.##########.|\n"
          "|J..........J|\n"
          "+------------+";
         */

        make_level_str(permanent, &permanent->level , size, string);
        permanent->grid = PUSH_STRUCT(&permanent->arena, Grid);
        init_grid(permanent->grid, &permanent->arena, &permanent->level);
        preprocess_grid(permanent->grid);
        expect(GetNodeAt(permanent->grid, 0, 0, 0)->isJumpNode);
        expect(GetNodeAt(permanent->grid, 11, 0, 0)->isJumpNode);
        expect(GetNodeAt(permanent->grid, 11, 10, 0)->isJumpNode);
        expect(GetNodeAt(permanent->grid, 0, 10, 0)->isJumpNode);
        expect(GetNodeAt(permanent->grid, 4, 3, 0)->isJumpNode);
        expect(GetNodeAt(permanent->grid, 7, 3, 0)->isJumpNode);
        expect(GetNodeAt(permanent->grid, 4, 7, 0)->isJumpNode);
        expect(GetNodeAt(permanent->grid, 7, 7, 0)->isJumpNode);

    }


    it (places jump points around ladders and on laddders) {
        // I found out I need to have jumppoints around ladders and stairs to
        // give people a way around them when possible.
        // otherwise people would end up  walking through them even when they
        // werent using them to go up or down.
        // i am notyet sure if I need diagonal around or cardianl around them
        INIT_MEMORY_PATHFINDER();
        /* Memory _memory; */
        /* Memory *memory = &_memory; */

        /* initialize_memory(memory); */
        /* PermanentState *permanent = (PermanentState *)memory->permanent; */
        World_Size size = (World_Size){6,3,2};
        char string[] =
            "+------+\n"
            "|......|\n"
            "|..S...|\n"
            "|......|\n"
            "+------+\n"
            "|......|\n"
            "|..E...|\n"
            "|......|\n"
            "+------+";

        /*
          "+------+\n"
          "|.J.J..|\n"
          "|..J...|\n"
          "|.J.J..|\n"
          "+------+\n";
          "|.J.J..|\n"
          "|..J...|\n"
          "|.J.J..|\n"
          "+------+";
         */

        make_level_str(permanent, &permanent->level , size, string);
        permanent->grid = PUSH_STRUCT(&permanent->arena, Grid);
        init_grid(permanent->grid, &permanent->arena, &permanent->level);
        expect(permanent->grid->width == 6 && permanent->grid->height == 3);
        preprocess_grid(permanent->grid);

        expect(four_diagonals_are_jumps(permanent->grid,2,1,0));
        expect(four_diagonals_are_jumps(permanent->grid,2,1,1));
        expect(four_cardinals_are_jumps(permanent->grid,2,1,0));
        expect(four_cardinals_are_jumps(permanent->grid,2,1,1));

        expect(GetNodeAt(permanent->grid, 2, 1, 0)->isJumpNode);
        expect(GetNodeAt(permanent->grid, 2, 1, 1)->isJumpNode);
        expect(total_jumppoints(permanent->grid) == 18);

    }
}

describe(pathfinder) {
    it(finds its way through a small narrow single floored maze (1000x) ) {

        INIT_MEMORY_PATHFINDER();

        World_Size size = (World_Size){6,7,1};
        char string[] =
            "+------+\n"
            "|.#....|\n"
            "|.#..#.|\n"
            "|.####.|\n"
            "|...#..|\n"
            "|.###.#|\n"
            "|...#.#|\n"
            "|.#....|\n"
            "+------+";

        TEST_RUN_PATHFINDER(1000);
    }
    it(finds its way through a forest  (1000x) ) {

        INIT_MEMORY_PATHFINDER();

        World_Size size = (World_Size){11,8,1};
        char string[] =
            "+-----------+\n"
            "|...#.....#.|\n"
            "|.#....#....|\n"
            "|...#....#..|\n"
            "|.#....#....|\n"
            "|....#...#..|\n"
            "|.#....#....|\n"
            "|...#....#..|\n"
            "|#....#....#|\n"
            "+-----------+";

        TEST_RUN_PATHFINDER(1000);
    }

    it(can use ladders (1000x)) {

        INIT_MEMORY_PATHFINDER();
        World_Size size = (World_Size){8,10,3};

         char string[] =
            "+--------+\n"
            "|        |\n"
            "| ###### |\n"
            "| #....# |\n"
            "| ##.### |\n"
            "| #....# |\n"
            "| #....# |\n"
            "| #.S..# |\n"
            "| #....# |\n"
            "| ###### |\n"
            "|        |\n"
            "+--------+\n"
            "|        |\n"
            "| ###### |\n"
            "| #....# |\n"
            "| ##.### |\n"
            "| #....# |\n"
            "| #....# |\n"
            "| #.Z..# |\n"
            "| #....# |\n"
            "| ###### |\n"
            "|        |\n"
            "+--------+\n"
            "|        |\n"
            "| ###### |\n"
            "| #....# |\n"
            "| ##.### |\n"
            "| #....# |\n"
            "| #....# |\n"
            "| #.E..# |\n"
            "| #....# |\n"
            "| ###### |\n"
            "|        |\n"
             "+--------+";

         TEST_RUN_PATHFINDER(1000);

    }


    it(finds its way through a multifloor building (1000x) ) {
        INIT_MEMORY_PATHFINDER();

        World_Size size = (World_Size){23,10,5};
        char string[] =
            "+-----------------------+\n"
            "|.......................|\n"
            "|.#####################.|\n"
            "|.#...................#.|\n"
            "|.#...................#.|\n"
            "|.#...................#.|\n"
            "|.#...................#.|\n"
            "|.#.......U===........#.|\n"
            "|.#...................#.|\n"
            "|.##########.##########.|\n"
            "|.......................|\n"
            "+-----------------------+\n"
            "|                       |\n"
            "| ##################### |\n"
            "| #...................# |\n"
            "| ##.######.#######.### |\n"
            "| #....#.......#......# |\n"
            "| #..#.#.......#......# |\n"
            "| #.S..#.......#....S.# |\n"
            "| #....#.......#......# |\n"
            "| ##################### |\n"
            "|                       |\n"
            "+-----------------------+\n"
            "|                       |\n"
            "| ##################### |\n"
            "| #...................# |\n"
            "| ##.######.#######.### |\n"
            "| #....#.......#......# |\n"
            "| #.#..#.......#......# |\n"
            "| #.Z..#.......#....E.# |\n"
            "| #....#.......#......# |\n"
            "| ##################### |\n"
            "|                       |\n"
            "+-----------------------+\n"
            "|                       |\n"
            "| ##################### |\n"
            "| #...................# |\n"
            "| ##.######.#######.### |\n"
            "| #....#.......#......# |\n"
            "| #....#.......#......# |\n"
            "| #.Z#.#.......#......# |\n"
            "| #....#.......#......# |\n"
            "| ##################### |\n"
            "|                       |\n"
            "+-----------------------+\n"
            "|                       |\n"
            "| ##################### |\n"
            "| #...................# |\n"
            "| ##.######.#######.### |\n"
            "| #....#.......#......# |\n"
            "| #....#.......#......# |\n"
            "| #.Z..#.......#......# |\n"
            "| #.#..#.......#......# |\n"
            "| ##################### |\n"
            "|                       |\n"
            "+-----------------------+";

        TEST_RUN_PATHFINDER(1000);

    }
}
//}

int main() {
    test(memory);
    test(leveldata);
    test(grid_preprocessor);
    test(pathfinder);
    test(slist);
    return summary();
}
