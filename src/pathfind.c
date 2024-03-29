#include "pathfind.h"
#include "data_structures.h"
#include "random.h"

internal inline int in_bounds(Grid *Grid, int x, int y, int z) {
    return ((x >= 0 && x < Grid->width) &&
            (y >= 0 && y < Grid->height) &&
            (z >= 0 && z < Grid->depth));
}

internal int can_move_down(Grid *Grid, int x, int y, int z) {
    if (z -1 < 0) return false;
    int from = get_node_at(Grid, x, y, z-1)->type;
    return (in_bounds(Grid, x, y, z) && (from == Stairs4N ||
                                        from == Stairs4E ||
                                        from == Stairs4S ||
                                        from == Stairs4W ||
                                        from == EscalatorDown4N ||
                                        from == EscalatorDown4E ||
                                        from == EscalatorDown4S ||
                                        from == EscalatorDown4W ));
}


void init_grid(Grid *g, MemoryArena *Arena, LevelData *m) {
    g->width = m->x;
    g->height = m->y;
    g->depth = m->z_level;
    g->nodes = PUSH_ARRAY(Arena, g->width * g->height * g->depth, grid_node);

    for (int z = 0; z < g->depth; z++) {
        for (int y = 0; y < g->height; y++) {
            for (int x = 0; x < g->width; x++) {
                int i = (x + (g->width * y) + (g->width * g->height * z));
                ASSERT(i >= 0 && i <= g->width * g->height * g->depth);
                int j = -1;
                j =  (x + (g->width * y) + (g->width * g->height * (z-1)));

                g->nodes[i].X = x;
                g->nodes[i].Y = y;
                g->nodes[i].Z = z;
                g->nodes[i].f = 0.0f;
                g->nodes[i].g = 0.0f;
                g->nodes[i].h = 0.0f;
                //->nodes[i].cost = 0.0f;


                g->nodes[i].walkable = 0;
                g->nodes[i].type = m->blocks[i].object;
                //g->nodes[i].walkable = m->blocks[i] == WallBlock ? 0 :  m->data[i];
                if (m->blocks[i].object == Floor) {
                    g->nodes[i].walkable = 1;
                    //} //else if (m->blocks[i].object == Nothing) {
                //g->nodes[i].walkable = 0;
                } else if (m->blocks[i].object == WallBlock) {
                    g->nodes[i].walkable = 0;
                } else if (m->blocks[i].object == LadderUp ||
                           m->blocks[i].object == LadderDown ||
                           m->blocks[i].object == LadderUpDown) {
                    g->nodes[i].walkable = 1;
                    g->nodes[i].h = 10.0f;
                } else if (m->blocks[i].object == Stairs1N ||
                           m->blocks[i].object == Stairs1E ||
                           m->blocks[i].object == Stairs1S ||
                           m->blocks[i].object == Stairs1W) {
                    //g->nodes[i].g = 1000.0f;
                    //printf("setting cost to 1000\n");
                    g->nodes[i].h = 10.0f;

                    g->nodes[i].walkable = 1;

                } else if (m->blocks[i].object == EscalatorUp1N ||
                           m->blocks[i].object == EscalatorUp1E ||
                           m->blocks[i].object == EscalatorUp1S ||
                           m->blocks[i].object == EscalatorUp1W) {
                    //g->nodes[i].g = 1000.0f;
                    //printf("setting cost to 1000\n");
                    g->nodes[i].h = 10.0f;

                    g->nodes[i].walkable = 1;

                } else if (can_move_down(g,x,y,z)) {
                    g->nodes[i].walkable = 1;
                    g->nodes[i].h = 10.0f;
                }/* else if (m->blocks[i].meta_object == StairsDownMeta) { */
                /*     g->nodes[i].walkable = 1; */
                /*     g->nodes[i].type = m->blocks[i].meta_object; */
                /* } */

                g->nodes[i].opened = g->nodes[i].closed = 0;
                g->nodes[i].parent = NULL;
            }
        }
    }
}



#define FLATTEN_3D_INDEX(x, y, z, width, height) (x + (y * width) + (z * width * height))

grid_node *get_node_at(Grid *Grid, int x, int y, int z) {
    return &Grid->nodes[FLATTEN_3D_INDEX(x, y, z, Grid->width, Grid->height)];
}



/* internal int isStairsDownMeta(Grid *Grid, int x, int y, int z) { */
/*     return (InBounds(Grid, x, y, z) && Grid->nodes[FLATTEN_3D_INDEX(x, y, z, Grid->width, Grid->height)].type == StairsDownMeta); */
/* } */


internal int is_stair_or_ladder(Grid *Grid, int x, int y, int z) {
    int type = get_node_at(Grid, x,y,z)->type;
    int up = in_bounds(Grid, x, y, z) &&
        (type == LadderUp || type == LadderUpDown || type == LadderDown ||
         type == Stairs1N || type == Stairs1E || type == Stairs1S || type == Stairs1W ||
         type == EscalatorUp1N || type == EscalatorUp1E || type == EscalatorUp1S || type == EscalatorUp1W );

    if (up) return 1;
    if (z-1 < 0) return false;

    type = get_node_at(Grid, x,y,z-1)->type;
    int down = in_bounds(Grid, x, y, z-1) && (type == Stairs4N ||
                                             type == Stairs4E ||
                                             type == Stairs4S ||
                                             type == Stairs4W ||
                                             type == EscalatorDown4N ||
                                             type == EscalatorDown4E ||
                                             type == EscalatorDown4S ||
                                             type == EscalatorDown4W);
    return down;
}


/* internal int is_stairs(Grid *Grid, int x, int y, int z) { */
/*     grid_node *node = &Grid->nodes[FLATTEN_3D_INDEX(x, y, z, Grid->width, Grid->height)]; */
/*     return (in_bounds(Grid, x, y, z) && (node->type == Stairs1N || node->type == Stairs1E || node->type == Stairs1S || node->type == Stairs1W)); */
/* } */
/* internal int isAnyLadder(Grid *Grid, int x, int y, int z) { */
/*     grid_node *node = &Grid->nodes[FLATTEN_3D_INDEX(x, y, z, Grid->width, Grid->height)]; */
/*     return (in_bounds(Grid, x, y, z) && (node->type == LadderUp || node->type == LadderDown || node->type == LadderUpDown )); */
/* } */

internal int grid_walkable_at(Grid *Grid, int x, int y, int z) {
    return (in_bounds(Grid, x, y, z) && Grid->nodes[FLATTEN_3D_INDEX(x, y, z, Grid->width, Grid->height)].walkable);
}
internal inline int is_jumpnode(Grid *Grid, int x, int y, int z, int dx, int dy) {
    return (grid_walkable_at(Grid, x - dx, y - dy, z) &&
            ((grid_walkable_at(Grid, x + dy, y + dx, z) &&
              !grid_walkable_at(Grid, x - dx + dy, y - dy + dx, z)) ||
             ((grid_walkable_at(Grid, x - dy, y - dx, z) &&
               !grid_walkable_at(Grid, x - dx - dy, y - dy - dx, z)))));
}
#define isWall(x, y, z) !grid_walkable_at(g, x, y, z)
#define isEmpty(x, y, z) grid_walkable_at(g, x, y, z)

typedef enum jump_direction {
    north,
    east,
    south,
    west,
    ne,
    se,
    sw,
    nw,
    up,
    down,
    invalid
} jump_direction;


internal void display_processed(Grid *g){
    printf("\033[2J");        /*  clear the screen  */
    printf("\033[H");         /*  position cursor at top-left corner */

    //int drawMap[g->width][g->height][g->depth];
    for (int z =0 ; z <g->depth; z++) {
        for (int y = 0; y < g->height; y++) {
            for (int x = 0; x < g->width; x++) {
                grid_node *n = get_node_at(g, x, y, z);
                //printf("%d", n->distance[sw] > 0 ? n->distance[sw] : 9 );
                if (n->type == WallBlock) {
                    printf("#");
                }
                else if (n->is_jumpnode) {
                   printf("J");
                }
                else if (n->type == Nothing ){
                    printf(" ");
                } else if (n->type == Floor ){
                    printf(".");
                } else if (n->type == Stairs1N || n->type == Stairs1E || n->type == Stairs1S || n->type == Stairs1W){
                    printf("U");
                } else {
                    printf("?");
                }
            }
            printf("\n");
        }
    }
}
internal int grid_can_go_up_from(Grid *Grid, int x, int y, int z) {
    if (z+1 >= Grid->depth ) return 0;
    int from = get_node_at(Grid, x, y, z)->type;
    int to = get_node_at(Grid, x, y, z + 1)->type;
    int ladder = ((from == LadderUpDown ||
                   from == LadderUp) && (to == LadderDown || to == LadderUpDown));
    int stairs = (from == Stairs1N ||
                  from == Stairs1E ||
                  from == Stairs1S ||
                  from == Stairs1W ||
                  from == EscalatorUp1N ||
                  from == EscalatorUp1E ||
                  from == EscalatorUp1S ||
                  from == EscalatorUp1W );
    return ladder || stairs;
}

internal int grid_can_go_down_from(Grid *Grid, int x, int y, int z) {
    if (z-1 <0 ) return 0;

    int from = get_node_at(Grid, x, y, z)->type;
    int to = get_node_at(Grid, x, y, z - 1)->type;
    int ladder = ((from == LadderUpDown || from == LadderDown) && (to == LadderUp || to == LadderUpDown));
    from = get_node_at(Grid, x, y, z-1)->type;
    int stairs = (from == Stairs4N ||
                  from == Stairs4E ||
                  from == Stairs4S ||
                  from == Stairs4W ||
                  from == EscalatorDown4N ||
                  from == EscalatorDown4E ||
                  from == EscalatorDown4S ||
                  from == EscalatorDown4W );
    return ladder || stairs;
}
void preprocess_grid(Grid *g) {
    int depth = g->depth;
    int width = g->width;
    int height = g->height;
    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int index = FLATTEN_3D_INDEX(x, y, z, width, height);
                grid_node *node = &g->nodes[index];
                if (!node->walkable) {
                    continue;
                }

                int canGoUp = grid_can_go_up_from(g, x, y, z);
                int canGoDown = grid_can_go_down_from(g, x, y, z);

                if (canGoUp || canGoDown) {
                    node->is_jumpnode = 1;
                    node->distance[up] = canGoUp ? 1 : 0;
                    node->distance[down] = canGoDown ? 1 : 0;
                } else {
                    node->distance[up] = 0;
                    node->distance[down] = 0;
                }

                // points around corners or voids turn to jumppoints
                if (is_jumpnode(g, x, y, z, 0, 1) ||
                    is_jumpnode(g, x, y, z, 0, -1) ||
                    is_jumpnode(g, x, y, z, 1, 0) ||
                    is_jumpnode(g, x, y, z, -1, 0)) {
                    node->is_jumpnode = 1;
                }

                // diagonally around stairs and ladders I place jumppoints
                if (is_stair_or_ladder(g,x-1,y-1,z) ||
                    is_stair_or_ladder(g,x+1,y+1,z) ||
                    is_stair_or_ladder(g,x+1,y-1,z) ||
                    is_stair_or_ladder(g,x-1,y+1,z)) {
                    node->is_jumpnode = 1;
                }
                if (is_stair_or_ladder(g,x-1,y,z) ||
                    is_stair_or_ladder(g,x+1,y,z) ||
                    is_stair_or_ladder(g,x,y-1,z) ||
                    is_stair_or_ladder(g,x,y+1,z)) {
                    node->is_jumpnode = 1;
                }
            }
        }
    }

    for (int z = 0; z < depth; z++) {
        // first we do east and west
        for (int y = 0; y < height; y++) {
            {
                // distance  west
                int countMovingWest = -1;
                int jumpPointLastSeen = 0;
                for (int x = 0; x < width; x++) {
                    int index = FLATTEN_3D_INDEX(x, y, z, width, height);
                    grid_node *node = &g->nodes[index];

                    if (isWall(x, y, z)) {
                        countMovingWest = -1;
                        jumpPointLastSeen = 0;
                        node->distance[west] = 0;
                        continue;
                    }
                    countMovingWest++;
                    if (jumpPointLastSeen) {
                        node->distance[west] = countMovingWest;
                    } else {
                        node->distance[west] = -countMovingWest;
                    }
                    if (node->is_jumpnode) {
                        //if (node->is_jumpnode && countMovingWest > 0) {
                        countMovingWest = 0;
                        jumpPointLastSeen = 1;
                    }
                }
            }
            {
                // distance east
                int countMovingEast = -1;
                int jumpPointLastSeen = 0;
                for (int x = width - 1; x >= 0; x--) {
                    int index = FLATTEN_3D_INDEX(x, y, z, width, height);
                    grid_node *node = &g->nodes[index];
                    if (isWall(x, y, z)) {
                        countMovingEast = -1;
                        jumpPointLastSeen = 0;
                        node->distance[east] = 0;
                        continue;
                    }
                    countMovingEast++;
                    if (jumpPointLastSeen) {
                        node->distance[east] = countMovingEast;
                    } else {
                        node->distance[east] = -countMovingEast;
                    }
                    if (node->is_jumpnode) {
                        //  if (node->is_jumpnode && countMovingEast > 0) {
                        countMovingEast = 0;
                        jumpPointLastSeen = 1;
                    }
                }
            }
        } // for y
        // north/south
        for (int x = 0; x < width; x++) {
            {
                int countMovingNorth = -1;
                int jumpPointLastSeen = 0;
                for (int y = 0; y < height; y++) {
                    int index = FLATTEN_3D_INDEX(x, y, z, width, height);
                    grid_node *node = &g->nodes[index];

                    if (isWall(x, y, z)) {
                        countMovingNorth = -1;
                        jumpPointLastSeen = 0;
                        node->distance[north] = 0;
                        continue;
                    }
                    countMovingNorth++;
                    if (jumpPointLastSeen) {
                        node->distance[north] = countMovingNorth;
                    } else {
                        node->distance[north] = -countMovingNorth;
                    }
                    //if (node->is_jumpnode && countMovingNorth > 0) {
                    if (node->is_jumpnode) {
                        countMovingNorth = 0;
                        jumpPointLastSeen = 1;
                    }
                }
            }
            {
                int countMovingSouth = -1;
                int jumpPointLastSeen = 0;
                for (int y = height - 1; y >= 0; y--) {
                    int index = FLATTEN_3D_INDEX(x, y, z, width, height);
                    grid_node *node = &g->nodes[index];
                    if (isWall(x, y, z)) {
                        countMovingSouth = -1;
                        jumpPointLastSeen = 0;
                        node->distance[south] = 0;
                        continue;
                    }
                    countMovingSouth++;
                    if (jumpPointLastSeen) {
                        node->distance[south] = countMovingSouth;
                    } else {
                        node->distance[south] = -countMovingSouth;
                    }
                    if (node->is_jumpnode) {
                        //if (node->is_jumpnode && countMovingSouth > 0) {
                        countMovingSouth = 0;
                        jumpPointLastSeen = 1;
                    }
                }
            }
        } // for x
        // diagonally north east and north west
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (isEmpty(x, y, z)) {
                    int index = FLATTEN_3D_INDEX(x, y, z, width, height);
                    grid_node *node = &g->nodes[index];
                    // North West
                    if (y == 0 || x == 0 || isWall(x, y - 1, z) || isWall(x - 1, y, z) || isWall(x - 1, y - 1, z)) {
                        node->distance[nw] = 0;
                    } else if (isEmpty(x - 1, y, z) && isEmpty(x, y - 1, z) &&
                               ((get_node_at(g, x - 1, y - 1, z)->distance[north] > 0) ||
                                (get_node_at(g, x - 1, y - 1, z)->distance[west] > 0))) {
                        node->distance[nw] = 1;
                    } else { // start incrementing
                        int jumpDistance = get_node_at(g, x - 1, y - 1, z)->distance[nw];
                        if (jumpDistance > 0) {
                            node->distance[nw] = 1 + jumpDistance;
                        } else { // jumpDistance <= 0
                            node->distance[nw] = -1 + jumpDistance;
                        }
                    }

                    // North East
                    if (y == 0 || x == width - 1 || (isWall(x, y - 1, z) || isWall(x + 1, y, z) || isWall(x + 1, y - 1, z))) {
                        node->distance[ne] = 0;
                    } else if (isEmpty(x, y - 1, z) && isEmpty(x + 1, y, z) &&
                               ((get_node_at(g, x + 1, y - 1, z)->distance[north] > 0) ||
                                get_node_at(g, x + 1, y - 1, z)->distance[east] > 0)) {
                        node->distance[ne] = 1;
                    } else {
                        int jumpDistance = get_node_at(g, x + 1, y - 1, z)->distance[ne];
                        if (jumpDistance > 0) {
                            node->distance[ne] = 1 + jumpDistance;
                        } else { // jumpDistance <= 0
                            node->distance[ne] = -1 + jumpDistance;
                        }
                    }
                }
            }
        } // y
        // south west and south east
        for (int y = height - 1; y >= 0; y--) {
            for (int x = 0; x < width; x++) {
                if (isEmpty(x, y, z)) {
                    // South West
                    int index = FLATTEN_3D_INDEX(x, y, z, width, height);
                    grid_node *node = &g->nodes[index];
                    if (y == height - 1 || x == 0 || isWall(x, y + 1, z) || isWall(x - 1, y, z) || isWall(x - 1, y + 1, z)) {
                        node->distance[sw] = 0;
                    } else if (isEmpty(x, y + 1, z) && isEmpty(x - 1, y, z) &&
                               (get_node_at(g, x - 1, y + 1, z)->distance[south] > 0 ||
                                get_node_at(g, x - 1, y + 1, z)->distance[west] > 0)) {
                        node->distance[sw] = 1;
                    } else {
                        int jumpDistance = get_node_at(g, x - 1, y + 1, z)->distance[sw];
                        if (jumpDistance > 0) {
                            node->distance[sw] = 1 + jumpDistance;
                        } else {
                            node->distance[sw] = -1 + jumpDistance;
                        }
                    }

                    // South East
                    if (y == height - 1 || x == width - 1 || isWall(x, y + 1, z) || isWall(x + 1, y, z) || isWall(x + 1, y + 1, z)) {
                        node->distance[se] = 0;
                    } else if (isEmpty(x, y + 1, z) && isEmpty(x + 1, y, z) &&
                               (get_node_at(g, x + 1, y + 1, z)->distance[south] > 0 ||
                                get_node_at(g, x + 1, y + 1, z)->distance[east] > 0)) {
                        node->distance[se] = 1;
                    } else {
                        int jumpDistance = get_node_at(g, x + 1, y + 1, z)->distance[se];
                        if (jumpDistance > 0) {
                            node->distance[se] = 1 + jumpDistance;
                        } else {
                            node->distance[se] = -1 + jumpDistance;
                        }
                    }
                }
            }
        }
    }
    //display_processed(g);
}







inline internal grid_node* heap_pop(grid_node_heap *h) {
    grid_node * Result = HEAP_PEEKFIRST(h);
    HEAP_POP(h, grid_node*);
    return Result;
}
#define NODE_EQUALS(node1, node2) (node1->X == node2->X && node1->Y == node2->Y && node1->Z == node2->Z)

internal path_list * backtrack_path(grid_node * Original, MemoryArena *Arena) {
    path_list * Result = PUSH_STRUCT(Arena, path_list);
    NEW_DLIST(Result, Arena, path_node);

    if (Original) {
         while (Original->parent) {
            path_node * Node = PUSH_STRUCT(Arena, path_node);
            //printf("backtracking: %d, %d\n", Original->X, Original->Y);
            Node->X = Original->X;
            Node->Y = Original->Y;
            Node->Z = Original->Z;
            DLIST_ADDFIRST(Result, Node);
            Original = Original->parent;
        }
        path_node * Node = PUSH_STRUCT(Arena, path_node);
        Node->X = Original->X;
        Node->Y = Original->Y;
        Node->Z = Original->Z;
        DLIST_ADDFIRST(Result, Node);
    }
    return Result;
}

#define SET_ALLOWED(travelling) allowed = travelling; allowedSize = sizeof(travelling)/sizeof(allowed[0]);
#define IS_CARDINAL(direction) (direction == north || direction == south || direction == west || direction == east || direction == up || direction == down)


internal jump_direction get_direction(grid_node *Current, grid_node *Parent) {
    int dx = Current->X - Parent->X;
    int dy = Current->Y - Parent->Y;
    int dz = Current->Z - Parent->Z;
    jump_direction Result = invalid;
    if (dx && dy) {
        // diagonal
        if (dx < 0 && dy < 0) {
            Result = nw;
        } else if (dx < 0 && dy > 0) {
            Result = sw;
        } else if (dx > 0 && dy < 0) {
            Result = ne;
        } else if (dx > 0 && dy > 0) {
            Result = se;
        }
        //cardinal
    } else if (dx || dy) {
        if (dy > 0) {
            Result = south;
        } else if (dy < 0) {
            Result = north;
        } else if (dx > 0) {
            Result = east;
        } else if (dx < 0) {
            Result = west;
        }
    } else if (dz) {
        //travelling up down.
        if (dz < 0){
            Result = down;
        } else {
            Result = up;
        }
    }
    return Result;
}



internal inline int diff_XY(grid_node *Current, grid_node *Parent) {
    int dx = Current->X - Parent->X;
    int dy = Current->Y - Parent->Y;
    //if (dy) assert(dx==0);
    //if (dx) assert(dy==0);
    return ABS(dx+dy);
}

internal inline int other_is_in_general_diagonal(jump_direction diagonal, jump_direction other) {
    if (diagonal == ne) {
        return (other == north || other == ne || other == east);
    } else if (diagonal == nw) {
        return (other == north || other == nw || other == west);
    } else if (diagonal == sw) {
        return (other == south || other == sw || other == west);
    } else if (diagonal == se) {
        return (other == east || other == se || other == east);
    }
    return 0;
}

internal grid_node * get_node_given_dir_and_dist(grid_node * Current, jump_direction direction, Grid * Grid, int distance ){
    //    printf("distance: %d\n", distance);
    int x = Current->X;
    int y = Current->Y;
    int z = Current->Z;
    grid_node *Result = NULL;
    int stair_offset = 4; // TODO sometimes you want 4, in some cases things break though.

    grid_node * one_down;// = get_node_at(Grid,x, y, z - 1);



    switch(direction) {
    case north:
        Result = get_node_at(Grid,x, y-distance, z);
        break;
    case east:
        Result = get_node_at(Grid,x+distance, y, z);
        break;
    case south:
        Result = get_node_at(Grid,x, y+distance, z);
        break;
    case west:
        Result = get_node_at(Grid,x-distance, y, z);
        break;
    case ne:
        Result = get_node_at(Grid,x+distance, y-distance, z);
        break;
    case se:
        Result = get_node_at(Grid,x+distance, y+distance, z);
        break;
    case nw:
        Result = get_node_at(Grid,x-distance, y-distance, z);
        break;
    case sw:
        Result = get_node_at(Grid,x-distance, y+distance, z);
        break;
    case up:
        if (Current->type == Stairs1N || Current->type == EscalatorUp1N ) {
            Result = get_node_at(Grid,x, y-stair_offset, z+distance);
        } else if (Current->type == Stairs1E || Current->type == EscalatorUp1E) {
            Result = get_node_at(Grid,x+stair_offset, y, z+distance);
        } else if (Current->type == Stairs1S || Current->type == EscalatorUp1S) {
            Result = get_node_at(Grid,x, y+stair_offset, z+distance);
        } else if (Current->type == Stairs1W || Current->type == EscalatorUp1W) {
            Result = get_node_at(Grid,x-stair_offset, y, z+distance);
        }else {
            Result = get_node_at(Grid,x, y, z+distance);
        }
        break;
    case down:
        //stair_offset = 3;
        one_down = get_node_at(Grid,x, y, z - 1);
        if (one_down->type == Stairs4S || one_down->type == EscalatorDown4S) {
            Result = get_node_at(Grid,x, y-stair_offset, z-distance);
        } else if (one_down->type == Stairs4W || one_down->type == EscalatorDown4W) {
            Result = get_node_at(Grid,x + stair_offset, y, z-distance);
        } else if (one_down->type == Stairs4N || one_down->type == EscalatorDown4N) {
            Result = get_node_at(Grid,x, y+stair_offset, z-distance);
        } else if (one_down->type == Stairs4E || one_down->type == EscalatorDown4E) {
            Result = get_node_at(Grid,x - stair_offset, y, z-distance);
        }else {
            //printf("derp huh!\n");
            Result = get_node_at(Grid,x, y, z-distance);
        }
        break;
    case invalid:
        ASSERT("THIS IS JUST WRONG!" && false)
        break;
    }

    return Result;
}

internal grid_node * get_node_given_dir(grid_node * Current, jump_direction direction, Grid * grid) {
    int distance = Current->distance[direction];
    return get_node_given_dir_and_dist(Current, direction, grid, distance);
}

#define SQRT2_OVER_1 0.41421356237309515
#define SQRT2 1.4142135623730951

internal float manhatten(int dx, int dy, int dz) {
    return (dx + dy + dz);
}

internal float octile(int dx, int dy, int dz) {
    return (dx < dy ? SQRT2_OVER_1 * dx + dy + (dz) : SQRT2_OVER_1 * dy + dx + (dz));
}




path_list * find_path(grid_node * startNode, grid_node * endNode, Grid * Grid, MemoryArena * Arena) {
    jump_direction travelling_south[] = {west, sw, south, se, east, up, down};
    jump_direction travelling_southEast[] = {south, se, east, up, down};
    jump_direction travelling_east[] = {south, se, east, ne, north, up, down};
    jump_direction travelling_northEast[] = {east, ne, north, up, down};
    jump_direction travelling_north[] = {east, ne, north, nw, west, up, down};
    jump_direction travelling_northWest[] = {north, nw, west, up, down};
    jump_direction travelling_west[] = {north, nw, west, sw, south, up, down};
    jump_direction travelling_southWest[] = {west, sw, south, up, down};
    jump_direction travelling_up[] = {north, nw, west, sw, south, se, east, ne, up};
    jump_direction travelling_down[] = {north, nw, west, sw, south, se, east, ne, down};

    //jump_direction travelling_up[] = {up};
    //jump_direction travelling_down[] = {down};


    jump_direction travelling_all[] = {north, nw, west, sw, south, se, east, ne, up, down};

    //TODO: this heap, maybe it should be put outside this function, and be reused.
    grid_node_heap * OpenList = PUSH_STRUCT(Arena, grid_node_heap);
    NEW_HEAP(OpenList, 1024*1024/2, grid_node*, Arena); // the way the heap works it can write values quite far into it.

    startNode->g = 0.0f;
    startNode->f = 0.0f;
    startNode->h = 0.0f;
    HEAP_PUSH(OpenList, startNode);
    // TODO: dirtylist
    startNode->opened = true;

    jump_direction *allowed = NULL;
    int allowedSize = 0;
    while (OpenList->count > 0) {
        grid_node * Node = heap_pop(OpenList);
        grid_node *Parent = Node->parent;
        float givenCost;

        Node->closed = true;
        if (NODE_EQUALS(Node, endNode)) {
            //printf("found path! using jps plus\n");
            return backtrack_path(Node, Arena);
        }

        int dx;
        int dy;
        int dz;



        if (!Parent) {
            //printf("No parent, will look in all directions\n");
            SET_ALLOWED(travelling_all);
        } else {

            /* if (Node->X == 1 && Node->Y == 1) { */
            /*     printf("Do i find the one?\n"); */
            /* } */

            dx = Node->X - Parent->X;
            dy = Node->Y - Parent->Y;
            dz = Node->Z - Parent->Z;
            //            printf("parent, delta %d %d %d\n", dx, dy, dz);

            if (dx && dy) {
                // diagonal
                if (dx < 0 && dy < 0) {
                    SET_ALLOWED(travelling_northWest);
                } else if (dx < 0 && dy > 0) {
                    SET_ALLOWED(travelling_southWest);
                } else if (dx > 0 && dy < 0) {
                    SET_ALLOWED(travelling_northEast);
                } else if (dx > 0 && dy > 0) {
                    SET_ALLOWED(travelling_southEast);
                }
                //cardinal
            } else if ((dx || dy)) {
                if (dy > 0) {
                    SET_ALLOWED(travelling_south);
                } else if (dy < 0) {
                    SET_ALLOWED(travelling_north);
                } else if (dx > 0) {
                    SET_ALLOWED(travelling_east);
                } else if (dx < 0) {
                    SET_ALLOWED(travelling_west);
                }
            } else if (dz) {
                //travelling up down.

                if (dz < 0){
                    SET_ALLOWED(travelling_down);
                } else {
                    SET_ALLOWED(travelling_up);
                }
            } else {

            }
        }
        for (int direction_index = 0; direction_index < allowedSize; direction_index++) {
            int direction = allowed[direction_index];
            grid_node * Successor = NULL;


            if ((Node->Z == endNode->Z) &&
                (IS_CARDINAL(direction)) &&
                ((int) get_direction(endNode, Node) == direction) &&
                ((diff_XY(endNode, Node)) <= ABS(Node->distance[direction])))  {
                Successor = endNode;
                givenCost = Node->g + (diff_XY(endNode, Node));
                //givenCost += Node->cost;
            }  else if ((Node->Z == endNode->Z) &&
                       (!IS_CARDINAL(direction)) &&
                        (other_is_in_general_diagonal((jump_direction)direction, get_direction(endNode, Node)) ) &&
                       ((ABS(Node->X - endNode->X) <= ABS(Node->distance[direction])) ||
                        (ABS(Node->Y - endNode->Y) <= ABS(Node->distance[direction]))))  {

                int minDiff = MIN(ABS(Node->X - endNode->X) , ABS(Node->Y - endNode->Y) );
                if (minDiff == 0) {
                    minDiff = ABS(Node->distance[direction]);
                }

                Successor = get_node_given_dir_and_dist(Node, (jump_direction)direction, Grid, minDiff );
                givenCost = Node->g + (SQRT2 * minDiff);
                //givenCost += Node->cost;

            }else if (Node->distance[direction] > 0){

                Successor = get_node_given_dir(Node, (jump_direction)direction, Grid);
                givenCost = ABS(manhatten(ABS(Node->X-Successor->X), ABS(Node->Y-Successor->Y),ABS(Node->Z-Successor->Z)));
                if (!IS_CARDINAL(direction)) {givenCost *= SQRT2;}
                givenCost += Node->g;
            }

            if (Successor) {
                if ((!Successor->opened || !Successor->closed)) {
                    Successor->opened = true;
                    Successor->parent = Node;
                    Successor->g = givenCost;
                    Successor->f = givenCost + octile(ABS(Successor->X-endNode->X), ABS(Successor->Y-endNode->Y),ABS( Successor->Z- endNode->Z));
                    Successor->f += Successor->h; // TODO can i reuse one of my f,g,h values? H perhaps?
                    HEAP_PUSH(OpenList, Successor);
                    //printf("pushing %d %d %d\n", Successor->X, Successor->Y, Successor->Z);

                    // TODO: dirtylist
                    //printf("successor 1\n");
                } else if (givenCost < Successor->g) {
                    Successor->parent = Node;
                    Successor->g = givenCost;
                    Successor->f = givenCost + octile(ABS(Successor->X-endNode->X), ABS(Successor->Y-endNode->Y),ABS( Successor->Z- endNode->Z));

                    HEAP_UPDATE_ITEM(OpenList, grid_node* , Successor);
                } else {
                    //printf("Big trouble\n %d, %d, %f >= %f\n", Successor->opened, Successor->closed, givenCost, Successor->g);
                }
            }
        }
    }

    printf("FAILED: no path found from %d,%d,%d to %d,%d,%d.\n", startNode->X, startNode->Y, startNode->Z, endNode->X, endNode->Y, endNode->Z);

    return NULL;
}

typedef struct coord2d {
    int X;
    int Y;
    struct coord2d * Next;
    struct coord2d * Prev;
} coord2d;

typedef struct coord_list {
    coord2d * Sentinel;
    coord2d * Free;
} coord_list;

#define setXYZinStruct(x,y,z, Struct) {Struct->X = x; Struct->Y = y; Struct->Z = z;}

#define PUSH_COORD(x, y, CoordList) {                       \
        coord2d *N = NULL;                                  \
        if (CoordList->Free->Next != CoordList->Sentinel) { \
            N = CoordList->Free->Next;                      \
            CoordList->Free->Next = N->Next;                \
        } else {                                            \
            N = PUSH_STRUCT(Arena, coord2d);                \
        }                                                   \
        N->X = x;                                           \
        N->Y = y;                                           \
        DLIST_ADDLAST(CoordList, N);                        \
    }                                                       \

internal void interpolate(int x0, int y0, int x1, int y1, MemoryArena * Arena, coord_list * List) {
    int sx, sy, dx, dy, err, e2;

    dx = ABS(x1 - x0);
    dy = ABS(y1 - y0);
    sx = (x0 < x1) ? 1 : -1;
    sy = (y0 < y1) ? 1 : -1;

    err = dx - dy;

    //int steps = 0;
    while(1) {
        PUSH_COORD(x0, y0, List);

        if (x0 == x1 && y0 == y1) {
            break;
        }
        e2 = 2 * err;
        if (e2 > -dy) {
            err = err - dy;
            x0 = x0 + sx;
        }
        if (e2 < dx) {
            err = err + dx;
            y0 = y0 + sy;
        }
        //steps++;
        //printf("steps in interpolation: %d\n", steps);
    }

}

path_list * smooth_path(path_list *compressed, MemoryArena * Arena, Grid * pg) {
    int x0 = compressed->Sentinel->Next->X;
    int y0 = compressed->Sentinel->Next->Y;
    int z0 = compressed->Sentinel->Next->Z;

    int x1 = compressed->Sentinel->Prev->X;
    int y1 = compressed->Sentinel->Prev->Y;
    int z1 = compressed->Sentinel->Prev->Z;


    path_list *Result = PUSH_STRUCT(Arena, path_list);
    NEW_DLIST(Result, Arena, path_node);

    int sx = x0;
    int sy = y0;
    int sz = z0;

    int ex, ey, ez;

    path_node * Point = PUSH_STRUCT(Arena, path_node);
    setXYZinStruct(sx, sy, sz, Point);
    DLIST_ADDLAST(Result, Point);
    path_node * Node = compressed->Sentinel->Next->Next; //skipping the first one.
    int blocked = 0;

    coord_list * interpolated = PUSH_STRUCT(Arena, coord_list);
    NEW_DLIST_FREE(interpolated, Arena, coord2d);

    //int steps = 0;
    while (Node != compressed->Sentinel) {
        //steps++;
        //printf("steps in smooth: %d\n", steps);
        ex = Node->X;
        ey = Node->Y;
        ez = Node->Z;
        if (sz == ez) {
            interpolate(sx,sy,ex,ey, Arena, interpolated);
            blocked = 0;
            coord2d * c = interpolated->Sentinel->Next->Next; //skipping the first
            while(c != interpolated->Sentinel) {
                grid_node * n= get_node_at(pg, c->X,c->Y,sz);
                if (! n->walkable ) {
                    blocked = 1;
                    break;
                }
                // SO I DONT SMOOTH AROUND STAIRS
                if (n->type >= StairsMeta && n->type <= EscalatorDown4W) {
                    blocked = 1;
                    break;
                }
                c = c->Next;
            }
            FREE_DLIST(interpolated);
            if (blocked) {
                path_node * lastValidPoint = PUSH_STRUCT(Arena, path_node);
                setXYZinStruct(Node->Prev->X, Node->Prev->Y, Node->Prev->Z, lastValidPoint);
                DLIST_ADDLAST(Result, lastValidPoint);
                sx = lastValidPoint->X;
                sy = lastValidPoint->Y;
                sz = lastValidPoint->Z;
            }
        } else { //different z, just push both points
            if(sx == ex && sy == ey) {
                printf("ladder right?\n");
                //ladder
                sz = Node->Z;
                path_node * Point1 = PUSH_STRUCT(Arena, path_node);
                setXYZinStruct(ex, ey, Node->Prev->Z, Point1);
                DLIST_ADDLAST(Result, Point1);

                path_node * Point2 = PUSH_STRUCT(Arena, path_node);
                setXYZinStruct(ex, ey, ez, Point2);
                DLIST_ADDLAST(Result, Point2);
            } else {
                //stairs;
                sz = Node->Z;
                printf("yeah alright\n");
                path_node * Point1 = PUSH_STRUCT(Arena, path_node);
                setXYZinStruct(Node->Prev->X, Node->Prev->Y, Node->Prev->Z, Point1);
                DLIST_ADDLAST(Result, Point1);
                
                path_node * Point2 = PUSH_STRUCT(Arena, path_node);
                setXYZinStruct( Node->X,Node->Y,Node->Z, Point2);
                DLIST_ADDLAST(Result, Point2);
            }

        }
        Node = Node->Next;
    }

     path_node * EndPoint = PUSH_STRUCT(Arena, path_node);
     setXYZinStruct(x1, y1, z1, EndPoint);
     DLIST_ADDLAST(Result, EndPoint);

    return Result;
}

path_list * expand_path(path_list *compressed, MemoryArena * Arena) {

    path_node * Node = compressed->Sentinel->Next;
    int x1, y1, z1;
    int x2, y2, z2;
    coord_list * interpolated = PUSH_STRUCT(Arena, coord_list);
    NEW_DLIST_FREE(interpolated, Arena, coord2d);

    path_list *  Result =  PUSH_STRUCT(Arena, path_list);
    NEW_DLIST(Result, Arena, path_node);

    while(Node->Next != compressed->Sentinel) {
        x1 = Node->X;
        y1 = Node->Y;
        z1 = Node->Z;
        x2 = Node->Next->X;
        y2 = Node->Next->Y;
        z2 = Node->Next->Z;
        if (z1 == z2) {
            interpolate(x1,y1,x2,y2, Arena, interpolated);
            coord2d * c = interpolated->Sentinel->Next;
            while (c != interpolated->Sentinel) {
                path_node * Point = PUSH_STRUCT(Arena, path_node);
                setXYZinStruct(c->X, c->Y, z1, Point);
                DLIST_ADDLAST(Result, Point);
                c = c->Next;
            }
            FREE_DLIST(interpolated);
        } else {
            path_node * Point1 = PUSH_STRUCT(Arena, path_node);
            setXYZinStruct(x2, y2, z1, Point1);
            DLIST_ADDLAST(Result, Point1);

            path_node * Point = PUSH_STRUCT(Arena, path_node);
            setXYZinStruct(x2, y2, z2, Point);
            DLIST_ADDLAST(Result, Point);
        }
        Node = Node->Next;
    }

    return Result;
}
