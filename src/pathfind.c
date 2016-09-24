#include "pathfind.h"
#include "data_structures.h"

void init_grid(Grid *g, MemoryArena *Arena, LevelData *m) {
    g->width = m->x;
    g->height = m->y;
    g->depth = m->z_level;
    g->nodes = PUSH_ARRAY(Arena, g->width * g->height * g->depth, grid_node);
    //printf("x: %d, y: %d, z: %d\n", g->width, g->height, g->depth);

    for (int z = 0; z < g->depth; z++) {
        for (int y = 0; y < g->height; y++) {
            for (int x = 0; x < g->width; x++) {
                int i = (x + (g->width * y) + (g->width * g->height * z));
                ASSERT(i >= 0 && i <= g->width * g->height * g->depth);
                int j = -1;
                j =  (x + (g->width * y) + (g->width * g->height * z-1));

                g->nodes[i].X = x;
                g->nodes[i].Y = y;
                g->nodes[i].Z = z;
                g->nodes[i].f = 0.0f;
                g->nodes[i].g = 0.0f;
                g->nodes[i].h = 0.0f;
                g->nodes[i].walkable = 1;
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
                } else if (m->blocks[i].object == StairsUp1N ||
                           m->blocks[i].object == StairsUp1E ||
                           m->blocks[i].object == StairsUp1S ||
                           m->blocks[i].object == StairsUp1W) {

                   g->nodes[i].walkable = 1;
                } else if (j >= 0 && (m->blocks[j].object == StairsUp4N ||
                                      m->blocks[j].object == StairsUp4E ||
                                      m->blocks[j].object == StairsUp4S ||
                                      m->blocks[j].object == StairsUp4W)) {
                   g->nodes[j].walkable = 1;
                } //else {
                //    g->nodes->walkable = 0;
                //}

                g->nodes[i].type = m->blocks[i].object;
                g->nodes[i].opened = g->nodes[i].closed = 0;
                g->nodes[i].parent = NULL;
            }
        }
    }
}



#define FLATTEN_3D_INDEX(x, y, z, width, height) (x + (y * width) + (z * width * height))

grid_node *GetNodeAt(Grid *Grid, int x, int y, int z) {
    return &Grid->nodes[FLATTEN_3D_INDEX(x, y, z, Grid->width, Grid->height)];
}
internal int GridCanGoUpFrom(Grid *Grid, int x, int y, int z) {
    int from = GetNodeAt(Grid, x, y, z)->type;
    int to = GetNodeAt(Grid, x, y, z + 1)->type;

    int ladder = ((from == LadderUpDown || from == LadderUp) && (to == LadderDown || to == LadderUpDown));
    int stairs = (from == StairsUp1N || from == StairsUp1E || from == StairsUp1E || from == StairsUp1W);
        /* (from == StairsUp1N && GetNodeAt(Grid, x, y-3, z + 1)->type == StairsDown1S) || */
        /* (from == StairsUp1E && GetNodeAt(Grid, x+3, y, z + 1)->type == StairsDown1W) || */
        /* (from == StairsUp1W && GetNodeAt(Grid, x-3, y, z + 1)->type == StairsDown1E) || */
        /* (from == StairsUp1S && GetNodeAt(Grid, x, y+3, z + 1)->type == StairsDown1N); */

    //if (ladder) printf("can go up by ladder\n");
    //if (stairs) printf("can go up by stairs\n");

    return ladder || stairs; // TODO use new blockTypes
}
internal int GridCanGoDownFrom(Grid *Grid, int x, int y, int z) {
    int from = GetNodeAt(Grid, x, y, z)->type;
    int to = GetNodeAt(Grid, x, y, z - 1)->type;
    int ladder = ((from == LadderUpDown || from == LadderDown) && (to == LadderUp || to == LadderUpDown));
    //if (result) printf("can go down\n");
    from = GetNodeAt(Grid, x, y, z-1)->type;
    int stairs = (from == StairsUp4N || from == StairsUp4E || from == StairsUp4E || from == StairsUp4W);
    //int stairs = (from == StairsDown1N || from == StairsDown1E || from == StairsDown1E || from == StairsDown1W);
    /* int stairs = */
    /*     (from == StairsDown1N && GetNodeAt(Grid, x, y-3, z - 1)->type == StairsUp1S) || */
    /*     (from == StairsDown1E && GetNodeAt(Grid, x+3, y, z - 1)->type == StairsUp1W) || */
    /*     (from == StairsDown1W && GetNodeAt(Grid, x-3, y, z - 1)->type == StairsUp1E) || */
    /*     (from == StairsDown1S && GetNodeAt(Grid, x, y+3, z - 1)->type == StairsUp1N); */
    //if (ladder) printf("can go down by ladder\n");
    if (stairs) printf("can go down by stairs %d, %d, %d\n", x,y,z);
    return ladder || stairs;//result;
        //return ((from == 3 || from == 4) && (to == 2 || to == 4)); // TODO use new blockTypes
}
internal inline int InBounds(Grid *Grid, int x, int y, int z) {
    return ((x >= 0 && x < Grid->width) &&
            (y >= 0 && y < Grid->height) &&
            (z >= 0 && z < Grid->depth));
}
internal int GridWalkableAt(Grid *Grid, int x, int y, int z) {
    return (InBounds(Grid, x, y, z) && Grid->nodes[FLATTEN_3D_INDEX(x, y, z, Grid->width, Grid->height)].walkable);
}
internal inline int isJumpNode(Grid *Grid, int x, int y, int z, int dx, int dy) {
    return (GridWalkableAt(Grid, x - dx, y - dy, z) &&
            ((GridWalkableAt(Grid, x + dy, y + dx, z) &&
              !GridWalkableAt(Grid, x - dx + dy, y - dy + dx, z)) ||
             ((GridWalkableAt(Grid, x - dy, y - dx, z) &&
               !GridWalkableAt(Grid, x - dx - dy, y - dy - dx, z)))));
}
#define isWall(x, y, z) !GridWalkableAt(g, x, y, z)
#define isEmpty(x, y, z) GridWalkableAt(g, x, y, z)

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
                // TODO: add portals here
                int canGoUp = GridCanGoUpFrom(g, x, y, z);
                int canGoDown = GridCanGoDownFrom(g, x, y, z);

                if (canGoUp || canGoDown) {
                    //printf("setting up or down as jump at %d, %d, %d\n",x,y,z);
                    node->isJumpNode = 1;
                    if (canGoUp) {
                        node->distance[up] = 1;
                    } else {
                        node->distance[up] = 0;
                    }
                    if (canGoDown) {
                        node->distance[down] = 1;
                    } else {
                        node->distance[down] = 0;
                    }
                } else {
                    node->distance[up] = 0;
                    node->distance[down] = 0;
                }

                // TODO when neigbouring node is a UP or DOWN I am a jusmp node TOO,
                if (isJumpNode(g, x, y, z, 0, 1)) {
                    node->isJumpNode = 1;
                } else if (isJumpNode(g, x, y, z, 0, -1)) {
                    node->isJumpNode = 1;
                } else if (isJumpNode(g, x, y, z, 1, 0)) {
                    node->isJumpNode = 1;
                } else if (isJumpNode(g, x, y, z, -1, 0)) {
                    node->isJumpNode = 1;
                } else {
                    for (int dx = -1; dx < 1; dx++) {
                        for (int dy = -1; dy < 1; dy++) {
                            //if (dx != 0 && dy != 0) {
                                if (GridCanGoDownFrom(g, x+dx, y+dy, z) || GridCanGoUpFrom(g, x+dx, y+dy, z) ) {
                                    node->isJumpNode = 1;
                                }
                                //
                        }
                    }

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
                    if (node->isJumpNode) {
                        //if (node->isJumpNode && countMovingWest > 0) {
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
                    if (node->isJumpNode) {
                        //  if (node->isJumpNode && countMovingEast > 0) {
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
                    //if (node->isJumpNode && countMovingNorth > 0) {
                    if (node->isJumpNode) {
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
                    if (node->isJumpNode) {
                        //if (node->isJumpNode && countMovingSouth > 0) {
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
                               ((GetNodeAt(g, x - 1, y - 1, z)->distance[north] > 0) ||
                                (GetNodeAt(g, x - 1, y - 1, z)->distance[west] > 0))) {
                        node->distance[nw] = 1;
                    } else { // start incrementing
                        int jumpDistance = GetNodeAt(g, x - 1, y - 1, z)->distance[nw];
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
                               ((GetNodeAt(g, x + 1, y - 1, z)->distance[north] > 0) ||
                                GetNodeAt(g, x + 1, y - 1, z)->distance[east] > 0)) {
                        node->distance[ne] = 1;
                    } else {
                        int jumpDistance = GetNodeAt(g, x + 1, y - 1, z)->distance[ne];
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
                               (GetNodeAt(g, x - 1, y + 1, z)->distance[south] > 0 ||
                                GetNodeAt(g, x - 1, y + 1, z)->distance[west] > 0)) {
                        node->distance[sw] = 1;
                    } else {
                        int jumpDistance = GetNodeAt(g, x - 1, y + 1, z)->distance[sw];
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
                               (GetNodeAt(g, x + 1, y + 1, z)->distance[south] > 0 ||
                                GetNodeAt(g, x + 1, y + 1, z)->distance[east] > 0)) {
                        node->distance[se] = 1;
                    } else {
                        int jumpDistance = GetNodeAt(g, x + 1, y + 1, z)->distance[se];
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
}


inline internal grid_node* HeapPop(grid_node_heap *h) {
    grid_node * Result = HEAP_PEEKFIRST(h);
    HEAP_POP(h, grid_node*);
    return Result;
}
#define NODE_EQUALS(node1, node2) (node1->X == node2->X && node1->Y == node2->Y && node1->Z == node2->Z)



internal path_list * BacktrackPath(grid_node * Original, MemoryArena *Arena) {
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


internal jump_direction getDirection(grid_node *Current, grid_node *Parent) {
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



internal inline int diffXY(grid_node *Current, grid_node *Parent) {
    int dx = Current->X - Parent->X;
    int dy = Current->Y - Parent->Y;
    //if (dy) assert(dx==0);
    //if (dx) assert(dy==0);
    return ABS(dx+dy);
}

internal inline int otherIsInGeneralDirectionOfDiagonal(jump_direction diagonal, jump_direction other) {
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
grid_node * getNodeGivenDirectionAndDistance(grid_node * Current, jump_direction direction, Grid * Grid, int distance ){
    //    printf("distance: %d\n", distance);
    int x = Current->X;
    int y = Current->Y;
    int z = Current->Z;
    grid_node *Result;
    int stair_offset = 3; // TODO sometimes you want 4, in some cases things break though.
    grid_node * one_down;// = GetNodeAt(Grid,x, y, z - 1);

    switch(direction) {
    case north:
        Result = GetNodeAt(Grid,x, y-distance, z);
        break;
    case east:
        Result = GetNodeAt(Grid,x+distance, y, z);
        break;
    case south:
        Result = GetNodeAt(Grid,x, y+distance, z);
        break;
    case west:
        Result = GetNodeAt(Grid,x-distance, y, z);
        break;
    case ne:
        Result = GetNodeAt(Grid,x+distance, y-distance, z);
        break;
    case se:
        Result = GetNodeAt(Grid,x+distance, y+distance, z);
        break;
    case nw:
        Result = GetNodeAt(Grid,x-distance, y-distance, z);
        break;
    case sw:
        Result = GetNodeAt(Grid,x-distance, y+distance, z);
        break;
    case up:

        if (Current->type == StairsUp1N) {
            Result = GetNodeAt(Grid,x, y-stair_offset, z+distance);
        } else if (Current->type == StairsUp1E) {
            Result = GetNodeAt(Grid,x+stair_offset, y, z+distance);
        } else if (Current->type == StairsUp1S) {
            Result = GetNodeAt(Grid,x, y+stair_offset, z+distance);
        } else if (Current->type == StairsUp1W) {
            Result = GetNodeAt(Grid,x-stair_offset, y, z+distance);
        }else {
            Result = GetNodeAt(Grid,x, y, z+distance);
        }
        break;
    case down:
        one_down = GetNodeAt(Grid,x, y, z - 1);
        if (Current->type == StairsUp4S) {
            Result = GetNodeAt(Grid,x, y-stair_offset, z-distance);
        } else if (one_down->type == StairsUp4W) {
            Result = GetNodeAt(Grid,x + stair_offset, y, z-distance);
        } else if (Current->type == StairsUp4N) {
            Result = GetNodeAt(Grid,x, y+stair_offset, z-distance);
        } else if (one_down->type == StairsUp4E) {
            Result = GetNodeAt(Grid,x - stair_offset, y, z-distance);
        }else {
            Result = GetNodeAt(Grid,x, y, z-distance);
        }
        break;
    case invalid:
        ASSERT("THIS IS JUST WRONG!" && false)
        break;
    }

    return Result;
}
grid_node * getNodeGivenDirection(grid_node * Current, jump_direction direction, Grid * grid) {
    int distance = Current->distance[direction];
    return getNodeGivenDirectionAndDistance(Current, direction, grid, distance);
}

#define SQRT2_OVER_1 0.41421356237309515
#define SQRT2 1.4142135623730951

internal float ManHattan(int dx, int dy, int dz) {
    return (dx + dy + dz);
}
internal float Octile(int dx, int dy, int dz) {
    return (dx < dy ? SQRT2_OVER_1 * dx + dy + (dz) : SQRT2_OVER_1 * dy + dx + (dz));
}

path_list * FindPathPlus(grid_node * startNode, grid_node * endNode, Grid * Grid, MemoryArena * Arena) {
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
    jump_direction travelling_all[] = {north, nw, west, sw, south, se, east, ne, up, down};


    grid_node_heap * OpenList = PUSH_STRUCT(Arena, grid_node_heap);
    NEW_HEAP(OpenList, 5000, grid_node*, Arena);

    startNode->g = 0.0f;
    startNode->f = 0.0f;
    startNode->h = 0.0f;
    HEAP_PUSH(OpenList, startNode);
    // TODO: dirtylist
    startNode->opened = true;

    jump_direction *allowed;
    int allowedSize = 0;
    while (OpenList->count > 0) {
        //visited++;
        grid_node * Node = HeapPop(OpenList);
        grid_node *Parent = Node->parent;
        float givenCost;
        jump_point p = (jump_point){Node->X, Node->Y, Node->Z};
        path_list *path = NULL;
        //printf("looking at %d,%d,%d\n", Node->X, Node->Y, Node->Z);
        //DrawGrid(Grid, p, path);

        Node->closed = true;
        if (NODE_EQUALS(Node, endNode)) {
            printf("found path! using jps plus\n");
            return BacktrackPath(Node, Arena);
        }

        int dx;
        int dy;
        int dz;
        if (!Parent) {
            //printf("No parent, will look in all directions\n");
            SET_ALLOWED(travelling_all);
        } else {

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
            } else if (dx || dy) {
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
        //        printf("Node %d,%d,%d \n",Node->X, Node->Y, Node->Z);
        for (int i = 0; i < allowedSize; i++) {
            int direction = allowed[i];
            //printf("direction: %d\n",direction);
            grid_node * Successor = NULL;
            //printf("distance to next jump [%s] : %d\n", names[direction], Node->distance[direction]);
            if ((Node->Z == endNode->Z) &&
                (IS_CARDINAL(direction)) &&
                (getDirection(endNode, Node) == direction) &&
                ((diffXY(endNode, Node)) <= ABS(Node->distance[direction])))  {
                Successor = endNode;
                givenCost = Node->g + (diffXY(endNode, Node));
                //printf("first leg succesoor: %d,%d,%d\n", Successor->X, Successor->Y, Successor->Z);

            }  else if ((Node->Z == endNode->Z) &&
                       (!IS_CARDINAL(direction)) &&
                       (otherIsInGeneralDirectionOfDiagonal(direction, getDirection(endNode, Node)) ) &&
                       ((ABS(Node->X - endNode->X) <= ABS(Node->distance[direction])) ||
                        (ABS(Node->Y - endNode->Y) <= ABS(Node->distance[direction]))))  {
                //printf("%s \n", names[direction]);
                //printf("distance to next jump in that direction : %d\n",Node->distance[direction]);
                int minDiff = MIN(ABS(Node->X - endNode->X) , ABS(Node->Y - endNode->Y) );
                if (minDiff == 0) {
                    minDiff = ABS(Node->distance[direction]);
                }
                //int dx = ABS(Node->X - endNode->X);
                //int dy = ABS(Node->Y - endNode->Y);

                // hack MinDiff here could have been 0,
                // TODO something is wrong with this MAX(1, minDiff), in the book its just minDiff but that leads to trouble too
                // this leads to way to many jump points
                Successor = getNodeGivenDirectionAndDistance(Node, direction, Grid, minDiff );

                givenCost = Node->g + (SQRT2 * minDiff);
                //printf("second leg succesoor: %d,%d,%d\n", Successor->X, Successor->Y, Successor->Z);

            }else if (Node->distance[direction] > 0){
                // TODO: if successor is a closed portal, dont jump there
                Successor = getNodeGivenDirection(Node, direction, Grid);

                if (Node->Z != Successor->Z){
                    //printf("node %d,%d,%d successor: %d,%d,%d\n",Node->X,Node->Y,Node->Z, Successor->X, Successor->Y, Successor->Z);

                    // TODO add an extra jumppoint here? for stairs that arent ladders (move in Z and in X or Y)
                    // stairs that use multiple tiles need an jumppoint cause taking the stairs moves in the ground plain.
                    // just a fancy getNodeStairsEnd() or something.

                }

                givenCost = ABS(ManHattan(ABS(Node->X-Successor->X), ABS(Node->Y-Successor->Y),ABS(Node->Z-Successor->Z)));
                if (!IS_CARDINAL(direction)) {givenCost *= SQRT2;}
                givenCost += Node->g;
                //printf("third leg succesoor: %d,%d,%d\n", Successor->X, Successor->Y, Successor->Z);


            }

            if (Successor) {
                if (!Successor->opened || !Successor->closed) {
                    Successor->opened = true;
                    Successor->parent = Node;
                    Successor->g = givenCost;
                    Successor->f = givenCost + Octile(ABS(Successor->X-endNode->X), ABS(Successor->Y-endNode->Y),ABS( Successor->Z- endNode->Z));
                    HEAP_PUSH(OpenList, Successor);
                    // TODO: dirtylist
                    //printf("successor 1\n");
                } else if (givenCost < Successor->g) {
                    Successor->parent = Node;
                    Successor->g = givenCost;
                    Successor->f = givenCost + Octile(ABS(Successor->X-endNode->X), ABS(Successor->Y-endNode->Y),ABS( Successor->Z- endNode->Z));
                    HEAP_UPDATE_ITEM(OpenList, grid_node* , Successor);
                } else {
                    //printf("Big trouble\n %d, %d, %f >= %f\n", Successor->opened, Successor->closed, givenCost, Successor->g);
                }
            }
        }

    }
    printf("sad face\n");

    return NULL;
    //return NULL;
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

#define PUSH_COORD(x, y, CoordList) {                      \
        coord2d *N = NULL;                                                \
        if (CoordList->Free->Next != CoordList->Sentinel) {   \
            N = CoordList->Free->Next;                           \
            CoordList->Free->Next = N->Next;                     \
        } else {                                                    \
            N = PUSH_STRUCT(Arena, coord2d);                       \
        }                                                           \
        N->X = x;                                                   \
        N->Y = y;                                                   \
        DLIST_ADDLAST(CoordList, N);                            \
    }                                                               \





void interpolate(int x0, int y0, int x1, int y1, MemoryArena * Arena, coord_list * List) {
    int sx, sy, dx, dy, err, e2;

    dx = ABS(x1 - x0);
    dy = ABS(y1 - y0);
    sx = (x0 < x1) ? 1 : -1;
    sy = (y0 < y1) ? 1 : -1;

    err = dx - dy;

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
    }

}

path_list * SmoothenPath(path_list *compressed, MemoryArena * Arena, Grid * pg) {
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

    while (Node != compressed->Sentinel) {

        ex = Node->X;
        ey = Node->Y;
        ez = Node->Z;
        //printf("path node: %d,%d,%d\n",ex,ey,ez);
        if (sz == ez) {
            interpolate(sx,sy,ex,ey, Arena, interpolated);
            blocked = 0;
            coord2d * c = interpolated->Sentinel->Next->Next; //skipping the first
            while(c != interpolated->Sentinel) {
                grid_node * n= GetNodeAt(pg, c->X,c->Y,sz);
                if (! n->walkable) {
                    //printf("found blocking at : %d,%d,%d\n",n->X,n->Y,n->Z);
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
            //printf("Smoothing, is this the cause?: %d,%d,%d, or %d,%d,%d \n", sx,sy,sz,ex,ey,ez);
            //printf("smoothing Z: from: %d, %d, %d,|||| %d, %d, %d\n",Node->Prev->X, Node->Prev->Y, Node->Prev->Z, Node->X,Node->Y,Node->Z);
            if(sx == ex && sy == ey) {
                //ladder
                path_node * Point1 = PUSH_STRUCT(Arena, path_node);
                setXYZinStruct(ex, ey, Node->Prev->Z, Point1);
                DLIST_ADDLAST(Result, Point1);

                path_node * Point = PUSH_STRUCT(Arena, path_node);
                setXYZinStruct(ex, ey, ez, Point);
                DLIST_ADDLAST(Result, Point);
            } else {
                //stairs;
                // TODO something with this ez / sz
                ez = Node->Z;
                path_node * Point1 = PUSH_STRUCT(Arena, path_node);
                setXYZinStruct(Node->Prev->X, Node->Prev->Y, Node->Prev->Z, Point1);
                DLIST_ADDLAST(Result, Point1);

                path_node * Point = PUSH_STRUCT(Arena, path_node);
                setXYZinStruct( Node->X,Node->Y,Node->Z, Point);
                DLIST_ADDLAST(Result, Point);
            }

        }
        Node = Node->Next;
    }

     path_node * EndPoint = PUSH_STRUCT(Arena, path_node);
     setXYZinStruct(x1, y1, z1, EndPoint);
     DLIST_ADDLAST(Result, EndPoint);

    return Result;
}

path_list * ExpandPath(path_list *compressed, MemoryArena * Arena) {

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
            // done with interplated list I can recycle it now.
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
