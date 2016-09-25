#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "c89spec.h"
#include "../src/states.h"
#include "../src/pathfind.h"
#include "../src/memory.h"
#include "../src/level.h"

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

bool block_at_xyz_is(int x, int y, int z, Block b, LevelData * level) {
    return (level->blocks[FLATTEN_3D_INDEX(x,y,z, level->x, level->y)].object == b);
}


describe(leveldata) {
    it (can be read from a string) {
        Memory _memory;
        Memory *memory = &_memory;

        initialize_memory(memory);
        PermanentState *permanent = (PermanentState *)memory->permanent;
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
        Memory _memory;
        Memory *memory = &_memory;

        initialize_memory(memory);
        PermanentState *permanent = (PermanentState *)memory->permanent;
        World_Size size = (World_Size){6,1,1};

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


int main() {
    test(memory);
    test(leveldata);
    return summary();
}
