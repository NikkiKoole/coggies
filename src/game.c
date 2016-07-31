
#include "SDL.h"

#include <stdio.h>

void game_update_and_render(void);

static void stuff(void) {
    int a = 12;
}

extern void game_update_and_render(void) {
    printf("getting in that function!\n");

}
