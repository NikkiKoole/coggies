#include "pathfind.h"
#include "data_structures.h"

#include <unistd.h> //for usleep, only used in drawGrid wich should go away eventually

void DisplayPreProcessed(grid *g){
    printf("\033[2J");        /*  clear the screen  */
    printf("\033[H");         /*  position cursor at top-left corner */

    int drawMap[g->width][g->height][g->depth];
    for (int z =0 ; z <g->depth; z++) {
        for (int y = 0; y < g->height; y++) {
            for (int x = 0; x < g->width; x++) {
                if (GetNodeAt(g, x, y, z)->walkable == 0) {
                    printf("## ");
                } else if (GetNodeAt(g, x, y, z)->isJumpNode) {
                    printf(" J ");
                } else {
                    int distance = GetNodeAt(g, x, y, z)->distance[se];
                    if (distance < 0) {distance = 99;}
                    printf("%02d ", distance);
                }

            }
            printf("\n");
        }
    }
}


void DrawGrid(grid * g, jump_point p, path_list *path ) {
    printf("\033[2J");        /*  clear the screen  */
    printf("\033[H");         /*  position cursor at top-left corner */

    int drawMap[g->width][g->height][g->depth];

    for (int z =0 ; z <g->depth; z++) {
        for (int y = 0; y < g->height; y++) {
            for (int x = 0; x < g->width; x++) {
                if (GetNodeAt(g, x, y, z)->walkable == 1) {
                    drawMap[x][y][z] = '.';
                }
                if (GetNodeAt(g, x, y, z)->walkable == 2) {
                    drawMap[x][y][z] = 'U';
                }
                if (GetNodeAt(g, x, y, z)->walkable == 3){
                    drawMap[x][y][z] = 'D';
                }
                if (GetNodeAt(g, x, y, z)->walkable == 4) {
                    drawMap[x][y][z] = 'Z';
                }
                if (GetNodeAt(g, x, y, z)->walkable == 0) {
                    drawMap[x][y][z] = '#';
                }
                if (GetNodeAt(g, x, y, z)->opened) {
                    drawMap[x][y][z] = 'o';
                }
                if (GetNodeAt(g, x, y, z)->closed) {
                    drawMap[x][y][z] = ':';
                }
                //if (GetNodeAt(g, x, y, z)->isJumpNode) {
                //    drawMap[x][y][z] = 'j';
                //}

            }
        }
    }
    if (p.X != -1) {
        drawMap[p.X][p.Y][p.Z] = 'J';
    }

    if (path) {
        path_node * done= path->Sentinel->Next;
        while (done->Next != path->Sentinel) {
            drawMap[done->X][done->Y][done->Z] = 'p';
            done = done->Next;
        }

    }

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"


    for (int z = 0; z< g->depth; z++) {
        for (int y = 0; y < g->height; y++) {
            for (int x = 0; x < g->width; x++) {
                char c = drawMap[x][y][z];
                if (c == 'o') {
                    printf(KGRN);
                } else if (c == ':') {
                    printf(KBLU);
                } else if (c == 'p') {
                    printf(KMAG);
                }
                else {
                    printf(KNRM);
                }
                printf( "%c",drawMap[x][y][z]);
            }
            printf("\n");
    }
    }
    fflush(stdout);
    printf(KNRM);

}


void interpolate(int x0, int y0, int x1, int y1, memory_arena * Arena, coord_list * List) {
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

#define setXYZinStruct(x,y,z, Struct) {Struct->X = x; Struct->Y = y; Struct->Z = z;}

path_list * ExpandPath(path_list *compressed, memory_arena * Arena) {

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

path_list * SmoothenPath(path_list *compressed, memory_arena * Arena, grid * pg) {
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
        if (sz == ez) {
            interpolate(sx,sy,ex,ey, Arena, interpolated);
            blocked = 0;
            coord2d * c = interpolated->Sentinel->Next->Next; //skipping the first
            while(c != interpolated->Sentinel) {
                grid_node * n= GetNodeAt(pg, c->X,c->Y,sz);
                if (! n->walkable) {
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
            path_node * Point1 = PUSH_STRUCT(Arena, path_node);
            setXYZinStruct(ex, ey, Node->Prev->Z, Point1);
            DLIST_ADDLAST(Result, Point1);

            path_node * Point = PUSH_STRUCT(Arena, path_node);
            setXYZinStruct(ex, ey, ez, Point);
            DLIST_ADDLAST(Result, Point);
        }
        Node = Node->Next;
    }

     path_node * EndPoint = PUSH_STRUCT(Arena, path_node);
     setXYZinStruct(x1, y1, z1, EndPoint);
     DLIST_ADDLAST(Result, EndPoint);

    return Result;
}



path_list * BacktrackPath(grid_node * Original, memory_arena *Arena) {
    path_list * Result = PUSH_STRUCT(Arena, path_list);
    NEW_DLIST(Result, Arena, path_node);

    if (Original) {
         while (Original->parent) {
            path_node * Node = PUSH_STRUCT(Arena, path_node);
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

void InitGrid(grid * g, memory_arena * Arena, map_file *m) {
    g->nodes = PUSH_ARRAY(Arena, m->width*m->height*m->depth, grid_node);
    g->width = m->width;
    g->height = m->height;
    g->depth = m->depth;

    for (int z =0; z < m->depth; z++) {
        for (int y = 0; y < m->height; y++) {
            for (int x = 0; x < m->width; x++) {
                int i = (x + (m->width * y) + (m->width * m->height * z));
                g->nodes[i].X = x;
                g->nodes[i].Y = y;
                g->nodes[i].Z = z;
                g->nodes[i].f = 0.0f;
                g->nodes[i].g = 0.0f;
                g->nodes[i].h = 0.0f;
                g->nodes[i].walkable = m->data[i] == 9 ? 0 :  m->data[i];
                g->nodes[i].opened = g->nodes[i].closed = 0;
                g->nodes[i].parent = NULL;
            }
        }
    }
}


float ManHattan(int dx, int dy, int dz) {
    return (dx + dy + dz);
}

float Octile(int dx, int dy, int dz) {
    return (dx < dy ? SQRT2_OVER_1 * dx + dy + (dz) : SQRT2_OVER_1 * dy + dx + (dz));
}

inline static int InBounds(grid *Grid, int x, int y, int z) {
    return ((x >= 0 && x < Grid->width) &&
            (y >= 0 && y < Grid->height) &&
            (z >= 0 && z < Grid->depth));
}

int
GridWalkableAt(grid *Grid, int x, int y, int z) {
    return (InBounds(Grid, x,y,z) && Grid->nodes[FLATTEN_3D_INDEX(x,y,z, Grid->width, Grid->height)].walkable);
}

grid_node*
GetNodeAt(grid *Grid, int x, int y, int z) {
    return  &Grid->nodes[FLATTEN_3D_INDEX(x,y,z, Grid->width, Grid->height)];
}

int GridCanGoUpFrom(grid* Grid, int X, int Y, int Z) {
    int fromValue =   GetNodeAt(Grid, X, Y, Z)->walkable;
    int toValue =   GetNodeAt(Grid, X, Y, Z+1)->walkable;
    return ((fromValue == 2 || fromValue == 4) && (toValue == 3 || toValue == 4));
}

int GridCanGoDownFrom(grid* Grid, int X, int Y, int Z) {
    int fromValue = GetNodeAt(Grid, X, Y, Z)->walkable;
    int toValue = GetNodeAt(Grid, X, Y, Z-1)->walkable;
    return ((fromValue == 3 || fromValue == 4) && (toValue == 2 || toValue == 4));
}


inline static grid_node* HeapPop(grid_node_heap *h) {
    grid_node * Result = HEAP_PEEKFIRST(h);
    HEAP_POP(h, grid_node*);
    return Result;
}

grid_node_list *
gridGetNeighbors(grid *Grid, grid_node_list *List, memory_arena *Arena, grid_node * Node, diagonal_movement Diagonal) {
    int X = Node->X;
    int Y = Node->Y;
    int Z = Node->Z;
    int s0=0, d0=0;
    int s1=0, d1=0;
    int s2=0, d2=0;
    int s3=0, d3=0;

    // first do north, south , east , west
    // north
    if (GridWalkableAt(Grid, X, Y-1, Z)) {
        SLIST_ADDFIRST(List, GetNodeAt(Grid, X, Y-1, Z));
        s0 = true;
    }
    // east
    if (GridWalkableAt(Grid, X+1, Y, Z)) {
        SLIST_ADDFIRST(List, GetNodeAt(Grid, X+1, Y, Z));
        s1 = true;
    }
    // south
    if (GridWalkableAt(Grid, X, Y+1, Z)) {
        SLIST_ADDFIRST(List, GetNodeAt(Grid, X, Y+1, Z));
        s2 = true;
    }
    // west
    if (GridWalkableAt(Grid, X-1, Y, Z)) {
        SLIST_ADDFIRST(List, GetNodeAt(Grid, X-1, Y, Z));
        s3 = true;
    }

     // up
    if (GridCanGoUpFrom(Grid, X,Y,Z)) {
        SLIST_ADDFIRST(List, GetNodeAt(Grid, X, Y, Z+1));
    }
    // down
    if (GridCanGoDownFrom(Grid,X,Y,Z )) {
        SLIST_ADDFIRST(List, GetNodeAt(Grid, X, Y, Z-1));
    }

    if (Diagonal == Never) {
        return List;
    }
    if (Diagonal == OnlyWhenNoObstacles) {
        d0 = s3 && s0;
        d1 = s0 && s1;
        d2 = s1 && s2;
        d3 = s2 && s3;
    } else if (Diagonal == IfAtMostOneObstacle) {
        d0 = s3 || s0;
        d1 = s0 || s1;
        d2 = s1 || s2;
        d3 = s2 || s3;
    } else if (Diagonal == Always) {
        d0 = d1 = d2 = d3 = true;
    }

    // ↖
    if (d0 && GridWalkableAt(Grid, X-1, Y-1, Z)) {
        SLIST_ADDFIRST(List, GetNodeAt(Grid, X-1, Y-1, Z));
    }
    // ↗
    if (d1 && GridWalkableAt(Grid, X+1, Y-1, Z)) {
        SLIST_ADDFIRST(List, GetNodeAt(Grid, X+1, Y-1, Z));
    }
    // ↘
    if (d2 && GridWalkableAt(Grid, X+1, Y+1, Z)) {
        SLIST_ADDFIRST(List, GetNodeAt(Grid, X+1, Y+1, Z));
    }
    // ↙
    if (d3 && GridWalkableAt(Grid, X-1, Y+1, Z)) {
        SLIST_ADDFIRST(List, GetNodeAt(Grid, X-1, Y+1, Z));
    }
    return List;
}



path_list* FindPathAStar(grid_node *Start, grid_node *  End, grid * Grid, memory_arena * Arena, diagonal_movement Diagonal){

    grid_node_heap *OpenList = PUSH_STRUCT(Arena, grid_node_heap);
    NEW_HEAP(OpenList, 5000, grid_node*, Arena);

    grid_node* Node;
    grid_node* Neighbor;
    grid_node_list *NeighborList;
    float Weight = 1.0;

    Start->g = 0;
    Start->f = 0;
    Start->opened = true;
    HEAP_PUSH(OpenList, Start);

    NeighborList =  PUSH_STRUCT(Arena, grid_node_list);
    NEW_SLIST(NeighborList, Arena, grid_node);

    while(OpenList->count > 0) {
        Node = HeapPop(OpenList);
        Node->closed = true;

        if (NODE_EQUALS(Node, End)) {
            printf("Found path you fool!\n");
            return BacktrackPath(Node, Arena);
        }

        gridGetNeighbors(Grid, NeighborList, Arena, Node, Diagonal);

        int X, Y, Z;
        float NextGivenCost;

        for (Neighbor = NeighborList->Sentinel->Next;
             Neighbor != NeighborList->Sentinel;
             Neighbor = Neighbor->Next) {

            if (Neighbor->closed) {
                continue;
            }
            X = Neighbor->X;
            Y = Neighbor->Y;
            Z = Neighbor->Z;
            NextGivenCost = Node->g + ((X - Node->X == 0 || Y - Node->Y == 0) ? 1 : SQRT2);
            if (!Neighbor->opened || NextGivenCost < Neighbor->g) {
                Neighbor->g = NextGivenCost;
                //neighbor->h = neighbor->h || weight * ManHattan(x,y,z, endX, endY, endZ);
                Neighbor->h = Neighbor->h || Weight * Octile(ABS(X- End->X),ABS(Y-End->Y),ABS(Z-End->Z));
                Neighbor->f = Neighbor->g + Neighbor->h;
                Neighbor->parent = Node;

                if (!Neighbor->opened) {
                    Neighbor->opened = true;
                    HEAP_PUSH(OpenList, Neighbor);
                } else {
                    HEAP_UPDATE_ITEM(OpenList, grid_node* , Neighbor);
                }
            }
        }
        NeighborList->Sentinel->Next = NeighborList->Sentinel; //resetting the list

    }
    return NULL;
}


// things needed for jps

jump_point
jump_diagonally_if_no_obstacles(grid * Grid, grid_node * End, int x, int y, int z, int PX, int PY, int PZ) {
    int dx = x - PX;
    int dy = y - PY;
    int dz = z - PZ;

    if (!GridWalkableAt(Grid, x,y,z)) {
        return (jump_point){-1,-1,-1};
    }

    if (NODE_EQUALS(GetNodeAt(Grid, x, y, z), End)) {
        return (jump_point) {x,y,z};
    }

    if (GridCanGoUpFrom(Grid, x, y, z) || GridCanGoDownFrom(Grid, x, y, z)    ) {
       return (jump_point) {x,y,z};
    }

    if (dx != 0 && dy != 0) {
        jump_point Result1;
        jump_point Result2;
        Result1 = jump_diagonally_if_no_obstacles(Grid, End,x + dx, y,z, x, y,z);
        Result2 = jump_diagonally_if_no_obstacles(Grid, End,x, y + dy,z, x, y,z);
        if (Result1.X != -1 || Result2.X != -1) {
            return  (jump_point) {x,y,z};
        };
    } else {
        if (dx != 0) {
            if ((GridWalkableAt(Grid, x, y - 1, z) && !GridWalkableAt(Grid, x - dx, y - 1, z)) ||
                (GridWalkableAt(Grid, x, y + 1, z) && !GridWalkableAt(Grid, x - dx, y + 1, z))) {
                return (jump_point) {x,y,z};
            }
        }
        else if (dy != 0) {
            if ((GridWalkableAt(Grid, x - 1, y, z) && !GridWalkableAt(Grid, x - 1, y - dy, z)) ||
                (GridWalkableAt(Grid, x + 1, y, z) && !GridWalkableAt(Grid, x + 1, y - dy, z))) {
                return (jump_point) {x,y,z};
            }
        }
    }
    if (GridWalkableAt(Grid, x + dx, y, z+dz) && GridWalkableAt(Grid, x, y + dy, z+dz)) {
        return jump_diagonally_if_no_obstacles(Grid, End, x+dx, y+dy, z+dz, x, y, z+dz);
    } else {
        return (jump_point){-1,-1,-1};
    }

}




jump_point
jump_never_diagonally(grid * Grid, grid_node * End, int x, int y, int z, int PX, int PY, int PZ) {
    int dx = x - PX;
    int dy = y - PY;
    int dz = z - PZ;

    if (!GridWalkableAt(Grid, x,y,z)) {
        return (jump_point){-1,-1,-1};
    }

    if (NODE_EQUALS(GetNodeAt(Grid, x, y, z), End)) {
        return (jump_point) {x,y,z};
    }

    if (GridCanGoUpFrom(Grid, x, y, z) || GridCanGoDownFrom(Grid, x, y, z)    ) {
       return (jump_point) {x,y,z};
    }

    if (dx !=0 ) {
        if ((GridWalkableAt(Grid,x, y-1, z) && !GridWalkableAt(Grid,x-dx, y-1, z))  ||
            (GridWalkableAt(Grid,x, y+1, z) && !GridWalkableAt(Grid,x-dx, y+1, z)) ) {
            return (jump_point) {x,y,z};
        }
    } else if (dy != 0  ) {
        if ((GridWalkableAt(Grid, x - 1, y, z) && !GridWalkableAt(Grid, x - 1, y - dy, z)) ||
            (GridWalkableAt(Grid, x + 1, y, z) && !GridWalkableAt(Grid, x + 1, y - dy, z))) {
            return (jump_point){x, y,z};
        }
        //When moving vertically, must check for horizontal jump points
        jump_point Result1;
        jump_point Result2;
        Result1 = jump_never_diagonally(Grid, End,x + 1, y,z, x, y,z);
        Result2 = jump_never_diagonally(Grid, End,x - 1, y,z, x, y,z);
        if (Result1.X != -1 || Result2.X != -1) {
            return  (jump_point) {x,y,z};
        };
    } else {

    }
    return jump_never_diagonally(Grid, End, x+dx, y+dy, z+dz, x, y, z+dz);
}

neighbor_list *
find_neighbors_diagonally_if_no_obstacles(grid * Grid, neighbor_list *List, grid_node_list *GridNodeNeighbors, grid_node *Node, memory_arena *Arena) {
    grid_node *parent = Node->parent;
    int x = Node->X;
    int y = Node->Y;
    int z = Node->Z;
    int px, py, pz, dx, dy, dz;

    if (parent) {
        px = parent->X;
        py = parent->Y;
        pz = parent->Z;
        dx = (x - px) / MAX(ABS(x - px), 1);
        dy = (y - py) / MAX(ABS(y - py), 1);
        dz = (z - pz) / MAX(ABS(z - pz), 1);

        if (dx != 0 && dy != 0) {
            if (GridWalkableAt(Grid, x, y+dy, z)) {
                PUSH_NEIGHBOR(x, y+dy, z, List);
            }
            if (GridWalkableAt(Grid, x+dx, y, z)) {
                PUSH_NEIGHBOR(x+dx, y, z, List);
            }
            if (GridWalkableAt(Grid, x, y+dy, z) && GridWalkableAt(Grid, x+dx, y, z)) {
                PUSH_NEIGHBOR(x+dx, y+dy, z, List);
            }
        }
        else {
            int isNextWalkable;
            if (dx != 0) {
                isNextWalkable = GridWalkableAt(Grid, x+dx, y, z);
                int isTopWalkable = GridWalkableAt(Grid, x, y+1, z);
                int isBottomWalkable = GridWalkableAt(Grid, x, y-1, z);
                if (isNextWalkable) {
                    PUSH_NEIGHBOR(x+dx, y, z, List);
                    if (isTopWalkable) {
                        PUSH_NEIGHBOR(x+dx, y+1, z, List);
                    }
                    if (isBottomWalkable) {
                        PUSH_NEIGHBOR(x+dx, y-1, z, List);
                    }
                }
                if (isTopWalkable) {
                    PUSH_NEIGHBOR(x, y+1, z, List);
                }
                if (isBottomWalkable) {
                    PUSH_NEIGHBOR(x, y-1, z, List);
                }
            } else if (dy != 0) {
                isNextWalkable = GridWalkableAt(Grid, x, y+dy, z);
                int isRightWalkable = GridWalkableAt(Grid, x+1, y, z);
                int isLeftWalkable = GridWalkableAt(Grid, x-1, y, z);
                if (isNextWalkable) {
                    PUSH_NEIGHBOR(x, y+dy, z, List);
                    if (isRightWalkable) {
                        PUSH_NEIGHBOR(x+1, y+dy, z, List);
                    }
                    if (isLeftWalkable) {
                        PUSH_NEIGHBOR(x-1, y+dy, z, List);
                    }
                }
                if (isRightWalkable) {
                    PUSH_NEIGHBOR(x+1, y, z, List);
                }
                if (isLeftWalkable) {
                    PUSH_NEIGHBOR(x-1, y, z, List);
                }

            }
            if (dz != 0) {
                FIND_NEIGHBORS_NORTH_SOUTH(Grid, x, y, z, List);
                FIND_NEIGHBORS_EAST_WEST(Grid, x, y, z, List);

                if (dz == 1) {
                    if (GridCanGoUpFrom(Grid, x,y,z)) {
                        PUSH_NEIGHBOR(x, y, z+1, List);
                    }
                } else if (dz == -1) {
                    if (GridCanGoDownFrom(Grid, x,y,z)) {
                        PUSH_NEIGHBOR(x, y, z-1, List);
                    }
                }
            }
        }
    } else {
        gridGetNeighbors(Grid, GridNodeNeighbors, Arena, Node, OnlyWhenNoObstacles);
        grid_node * Neighbor = GridNodeNeighbors->Sentinel->Next;
        while(Neighbor != GridNodeNeighbors->Sentinel) {
            PUSH_NEIGHBOR(Neighbor->X, Neighbor->Y, Neighbor->Z, List);
            Neighbor = Neighbor->Next;
        }
        GridNodeNeighbors->Sentinel->Next = GridNodeNeighbors->Sentinel;
    }
    return List;
}

neighbor_list *
find_neighbors_never_diagonally(grid * Grid, neighbor_list *List, grid_node_list *GridNodeNeighbors, grid_node *Node, memory_arena *Arena) {
    grid_node *parent = Node->parent;
    int x = Node->X;
    int y = Node->Y;
    int z = Node->Z;
    int px, py, pz, dx, dy, dz;

    if (parent) {
        px = parent->X;
        py = parent->Y;
        pz = parent->Z;
        dx = (x - px) / MAX(ABS(x - px), 1);
        dy = (y - py) / MAX(ABS(y - py), 1);
        dz = (z - pz) / MAX(ABS(z - pz), 1);

        if (dx != 0 ) {
            FIND_NEIGHBORS_NORTH_SOUTH(Grid, x, y, z, List);

            if (GridWalkableAt(Grid, x+dx, y, z)) {
                PUSH_NEIGHBOR(x+dx, y, z, List);
            }

            FIND_NEIGHBORS_UP_DOWN(Grid, x, y, z, List);

        } else if (dy != 0 ) {
            FIND_NEIGHBORS_EAST_WEST(Grid, x, y, z, List);

            if (GridWalkableAt(Grid, y+dy, y, z)) {
                PUSH_NEIGHBOR(x, y+dy, z, List);
            }
            FIND_NEIGHBORS_UP_DOWN(Grid, x, y, z, List);
        }
        if (dz != 0) {
            FIND_NEIGHBORS_NORTH_SOUTH(Grid, x, y, z, List);
            FIND_NEIGHBORS_EAST_WEST(Grid, x, y, z, List);

            if (dz == 1) {
                if (GridCanGoUpFrom(Grid, x,y,z)) {
                    PUSH_NEIGHBOR(x, y, z+1, List);
                }
            } else if (dz == -1) {
                if (GridCanGoDownFrom(Grid, x,y,z)) {
                    PUSH_NEIGHBOR(x, y, z-1, List);
                }
            }
        }
    } else {
        //grid_node_list * neighborNodes =
        gridGetNeighbors(Grid, GridNodeNeighbors, Arena, Node, Never);
        grid_node * Neighbor = GridNodeNeighbors->Sentinel->Next;
        while(Neighbor != GridNodeNeighbors->Sentinel) {
            PUSH_NEIGHBOR(Neighbor->X, Neighbor->Y, Neighbor->Z, List);
            Neighbor = Neighbor->Next;
        }
        GridNodeNeighbors->Sentinel->Next = GridNodeNeighbors->Sentinel;
    }

    return List;
}

#define JUMP_ALWAYS_DIAGONALLY(jump) {                                  \
        if (!GridWalkableAt(Grid, x,y,z)) {                             \
            return (jump_point){-1,-1,-1};                              \
        }                                                               \
        if (NODE_EQUALS(GetNodeAt(Grid, x, y, z), End)) {               \
            return (jump_point) {x,y,z};                                \
        }                                                               \
        if (GridCanGoUpFrom(Grid, x, y, z) || GridCanGoDownFrom(Grid, x, y, z)    ) { \
            return (jump_point) {x,y,z};                                \
        }                                                               \
        if (dx != 0 && dy != 0) {                                       \
            if ((GridWalkableAt(Grid, x - dx, y + dy, z) && !GridWalkableAt(Grid, x - dx, y, z)) || \
                (GridWalkableAt(Grid, x + dx, y - dy, z) && !GridWalkableAt(Grid, x, y - dy, z))) { \
                return (jump_point) {x,y,z};                            \
            }                                                           \
            jump_point Result1;                                         \
            jump_point Result2;                                         \
            Result1 = jump(Grid, End,x + dx, y,z, x, y,z);              \
            Result2 = jump(Grid, End,x, y + dy,z, x, y,z);              \
            if (Result1.X != -1 || Result2.X != -1) {                   \
                return  (jump_point) {x,y,z};                           \
            };                                                          \
        }                                                               \
        else {                                                          \
            if( dx != 0 ) {                                             \
                if((GridWalkableAt(Grid, x + dx, y + 1, z ) && !GridWalkableAt(Grid, x, y + 1, z)) || \
                   (GridWalkableAt(Grid, x + dx, y - 1, z) && !GridWalkableAt(Grid, x, y - 1, z))) { \
                    return (jump_point){x, y,z};                        \
                }                                                       \
            }                                                           \
            else {                                                      \
                if((GridWalkableAt(Grid, x + 1, y + dy,z ) && !GridWalkableAt(Grid, x + 1, y, z)) || \
                   (GridWalkableAt(Grid, x - 1, y + dy,z) && !GridWalkableAt(Grid, x - 1, y, z))) { \
                    return (jump_point){x, y,z};                        \
                }                                                       \
            }                                                           \
        }                                                               \
    }                                                                   \


jump_point
jump_always_diagonally(grid * Grid, grid_node * End, int x, int y, int z, int PX, int PY, int PZ) {
    int dx = x - PX;
    int dy = y - PY;
    int dz = z - PZ;

    JUMP_ALWAYS_DIAGONALLY(jump_always_diagonally);

    return jump_always_diagonally(Grid, End, x+dx, y+dy, z+dz, x, y, z+dz);
}

#define FIND_NEIGHBORS_ALWAYS_DIAGONALLY_IF_PARENT {            \
        px = parent->X;                                         \
        py = parent->Y;                                         \
        pz = parent->Z;                                         \
        dx = (x - px) / MAX(ABS(x - px), 1);                    \
        dy = (y - py) / MAX(ABS(y - py), 1);                    \
        dz = (z - pz) / MAX(ABS(z - pz), 1);                    \
        if (dx != 0 && dy != 0) {                               \
            if (GridWalkableAt(Grid, x, y+dy, z)) {             \
                PUSH_NEIGHBOR(x, y+dy, z, List);                \
            }                                                   \
            if (GridWalkableAt(Grid, x+dx, y, z)) {             \
                PUSH_NEIGHBOR(x+dx, y, z, List);                \
            }                                                   \
            if (GridWalkableAt(Grid, x+dx, y+dy, z)) {          \
                PUSH_NEIGHBOR(x+dx, y+dy, z, List);             \
            }                                                   \
            if (!GridWalkableAt(Grid, x-dx, y, z)) {            \
                PUSH_NEIGHBOR(x-dx, y+dy, z, List);             \
            }                                                   \
            if (!GridWalkableAt(Grid, x, y-dy, z)) {            \
                PUSH_NEIGHBOR(x, y-dy, z, List);                \
            }                                                   \
            FIND_NEIGHBORS_UP_DOWN(Grid, x, y, z, List);        \
        }                                                       \
        else {                                                  \
            if (dx == 0) {                                      \
                if (GridWalkableAt(Grid, x, y+dy, z)) {         \
                    PUSH_NEIGHBOR(x, y+dy, z, List);            \
                }                                               \
                if (!GridWalkableAt(Grid, x+1, y, z)) {         \
                    PUSH_NEIGHBOR(x+1, y+dy, z, List);          \
                }                                               \
                if (!GridWalkableAt(Grid, x-1, y, z)) {         \
                    PUSH_NEIGHBOR(x-1, y+dy, z, List);          \
                }                                               \
                FIND_NEIGHBORS_UP_DOWN(Grid, x, y, z, List);    \
            } else if(dy == 0) {                                \
                if (GridWalkableAt(Grid, x+dx, y+dy, z)) {      \
                    PUSH_NEIGHBOR(x+dx, y, z, List);            \
                }                                               \
                if (!GridWalkableAt(Grid, x, y+1, z)) {         \
                    PUSH_NEIGHBOR(x+dx, y+1, z, List);          \
                }                                               \
                if (!GridWalkableAt(Grid, x, y-1, z)) {         \
                    PUSH_NEIGHBOR(x+dx, y-1, z, List);          \
                }                                               \
                FIND_NEIGHBORS_UP_DOWN(Grid, x, y, z, List);    \
            }                                                   \
        }                                                       \
        if (dz != 0) {                                          \
            for (int i = -1 ; i < 2; i++) {                     \
                for (int j = -1; j < 2; j++) {                  \
                    if (GridWalkableAt(Grid, x+i,y+j,z)) {      \
                        PUSH_NEIGHBOR(x+i, y+j, z, List);       \
                    }                                           \
                }                                               \
            }                                                   \
            if (dz == 1) {                                      \
                if (GridCanGoUpFrom(Grid, x,y,z)) {             \
                    PUSH_NEIGHBOR(x, y, z+1, List);             \
                }                                               \
            } else if (dz == -1) {                              \
                if (GridCanGoDownFrom(Grid, x,y,z)) {           \
                    PUSH_NEIGHBOR(x, y, z-1, List);             \
                }                                               \
            }                                                   \
        }                                                       \
    }                                                           \

neighbor_list *
find_neighbors_always_diagonally(grid * Grid, neighbor_list *List, grid_node_list *GridNodeNeighbors, grid_node *Node, memory_arena *Arena) {
    //neighbor_list *List =  PUSH_STRUCT(Arena, neighbor_list);
    //NEW_SLIST(List, Arena, neighbor);

    grid_node *parent = Node->parent;
    int x = Node->X;
    int y = Node->Y;
    int z = Node->Z;
    int px, py, pz, dx, dy, dz;

    if (parent) {
        FIND_NEIGHBORS_ALWAYS_DIAGONALLY_IF_PARENT

    } else {
        //grid_node_list * neighborNodes =
        gridGetNeighbors(Grid, GridNodeNeighbors, Arena, Node, Always);
        grid_node * Neighbor = GridNodeNeighbors->Sentinel->Next;
        while(Neighbor != GridNodeNeighbors->Sentinel) {
            PUSH_NEIGHBOR(Neighbor->X, Neighbor->Y, Neighbor->Z, List);
            Neighbor = Neighbor->Next;
        }
        GridNodeNeighbors->Sentinel->Next = GridNodeNeighbors->Sentinel;
    }
    return List;
}

jump_point
jump_if_at_most_one_obstacle(grid * Grid, grid_node * End, int x, int y, int z, int PX, int PY, int PZ) {
    int dx = x - PX;
    int dy = y - PY;
    int dz = z - PZ;

    JUMP_ALWAYS_DIAGONALLY(jump_if_at_most_one_obstacle);

    if (GridWalkableAt(Grid, x + dx, y, z+dz) || GridWalkableAt(Grid, x, y + dy, z+dz)) {
        return jump_if_at_most_one_obstacle(Grid, End, x+dx, y+dy, z+dz, x, y, z+dz);
    } else {
        return (jump_point){-1,-1,-1};;
    }

}

neighbor_list *
find_neighbors_if_at_most_one_obstacle(grid * Grid, neighbor_list *List, grid_node_list *GridNodeNeighbors, grid_node *Node, memory_arena *Arena) {
    //neighbor_list *List =  PUSH_STRUCT(Arena, neighbor_list);
    //NEW_SLIST(List, Arena, neighbor);

    grid_node *parent = Node->parent;
    int x = Node->X;
    int y = Node->Y;
    int z = Node->Z;
    int px, py, pz, dx, dy, dz;

    if (parent) {
        FIND_NEIGHBORS_ALWAYS_DIAGONALLY_IF_PARENT
    } else {
        gridGetNeighbors(Grid, GridNodeNeighbors, Arena, Node, IfAtMostOneObstacle);
        grid_node * Neighbor = GridNodeNeighbors->Sentinel->Next;
        while(Neighbor != GridNodeNeighbors->Sentinel) {
            PUSH_NEIGHBOR(Neighbor->X, Neighbor->Y, Neighbor->Z, List);
            Neighbor = Neighbor->Next;
        }
        GridNodeNeighbors->Sentinel->Next = GridNodeNeighbors->Sentinel;
    }
    return List;
}



void identifySuccessors(grid_node * Node, neighbor_list * Neighbors,  grid_node_list *GridNodeNeighbors, grid_node_heap * OpenList, grid * Grid, grid_node * End, memory_arena * Arena,  JPS JumpType) {
    JumpType.find_neighbors(Grid, Neighbors, GridNodeNeighbors,  Node, Arena);

    int x = Node->X;
    int y = Node->Y;
    int z = Node->Z;
    int jx, jy, jz;
    float Distance, NextGivenCost;
    for (neighbor * Neighbor = Neighbors->Sentinel->Next;
         Neighbor != Neighbors->Sentinel;
         Neighbor = Neighbor->Next) {

        jump_point JumpPoint;
        JumpPoint = JumpType.jump(Grid,  End,  Neighbor->X, Neighbor->Y,Neighbor->Z, x, y, z);

        if (JumpPoint.X != -1) {
            jx = JumpPoint.X;
            jy = JumpPoint.Y;
            jz = JumpPoint.Z;
            grid_node * JumpNode = GetNodeAt(Grid, jx,jy,jz);
            if (JumpNode->closed) {
                continue;
            }

            Distance = ManHattan(ABS(x-jx), ABS(y-jy), ABS(z-jz));
            NextGivenCost = Node->g + Distance;

            if (!JumpNode->opened || NextGivenCost < JumpNode->g) {
                JumpNode->g = NextGivenCost;

                if (!JumpNode->h) {
                    JumpNode->h = Octile(ABS(jx-End->X), ABS(jy-End->Y), ABS(jz-End->Z));
                }
                JumpNode->f = JumpNode->g + JumpNode->h;
                JumpNode->parent = Node;

                if (!JumpNode->opened) {
                    HEAP_PUSH(OpenList, JumpNode);
                    // TODO: dirtylist
                    JumpNode->opened = true;
                } else {
                    HEAP_UPDATE_ITEM(OpenList, grid_node* , JumpNode);
                }
            }
        }
    }
    FREE_DLIST(Neighbors);
}


path_list * FindPathJPS(grid_node * Start, grid_node * End, grid * Grid, memory_arena * Arena, diagonal_movement Diagonal) {
    grid_node_heap * OpenList = PUSH_STRUCT(Arena, grid_node_heap);
    NEW_HEAP(OpenList, 250, grid_node*, Arena);

    JPS JumpType = {};
    if (Diagonal == OnlyWhenNoObstacles) {
        JumpType.jump = jump_diagonally_if_no_obstacles;
        JumpType.find_neighbors = find_neighbors_diagonally_if_no_obstacles;
    } else if (Diagonal == Never) {
        JumpType.jump = jump_never_diagonally;
        JumpType.find_neighbors = find_neighbors_never_diagonally;
    } else if (Diagonal == Always) {
        JumpType.jump = jump_always_diagonally;
        JumpType.find_neighbors = find_neighbors_always_diagonally;
    } else if (Diagonal == IfAtMostOneObstacle) {
        JumpType.jump = jump_if_at_most_one_obstacle;
        JumpType.find_neighbors = find_neighbors_if_at_most_one_obstacle;
    }
    // todo : let the heuristic depend on diagonal movement type.

    Start->g = 0.0f;
    Start->f = 0.0f;
    Start->h = 0.0f;

    HEAP_PUSH(OpenList, Start);
    // TODO: dirtylist

    Start->opened = true;

    grid_node * Node;
    neighbor_list * Neighbors = PUSH_STRUCT(Arena, neighbor_list);
    NEW_DLIST_FREE(Neighbors, Arena, neighbor);
    grid_node_list *GridNodeNeighbors =  PUSH_STRUCT(Arena, grid_node_list);
    NEW_SLIST(GridNodeNeighbors, Arena, grid_node);

    while (OpenList->count > 0) {
        Node = HeapPop(OpenList);
        Node->closed = true;

        if (NODE_EQUALS(Node, End)) {
            printf("found path jps style\n");
            return BacktrackPath(Node, Arena);

        }
        identifySuccessors(Node, Neighbors, GridNodeNeighbors, OpenList, Grid, End, Arena, JumpType);
    }
    return NULL;
}

// things needed for jps plus

#ifdef JPS_PLUS

static inline int isJumpNode(grid *Grid, int x, int y, int z, int dx, int dy) {
    return (GridWalkableAt(Grid, x-dx, y-dy, z) &&
            ((GridWalkableAt(Grid, x+dy, y+dx, z) &&
              !GridWalkableAt(Grid, x-dx+dy, y-dy+dx, z)) ||
             ((GridWalkableAt(Grid, x-dy, y-dx, z) &&
               !GridWalkableAt(Grid, x-dx-dy, y-dy-dx,z)))));
}


#define isWall(x,y,z) !GridWalkableAt(g, x, y, z)

#define isEmpty(x,y,z) GridWalkableAt(g, x, y, z)

void PreProcessGrid(grid *g){
    int depth = g->depth;
    int width = g->width;
    int height = g->height;
    for (int z =0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int index = FLATTEN_3D_INDEX(x,y,z, width, height);
                grid_node  *node = &g->nodes[index];
                if (!node->walkable) {
                    continue;
                }
                // TODO: add portals here
                int canGoUp = GridCanGoUpFrom(g, x, y, z);
                int canGoDown = GridCanGoDownFrom(g, x, y, z);

                if (canGoUp || canGoDown) {
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

                if (isJumpNode(g, x, y, z, 0, 1)) {
                    node->isJumpNode = 1;
                } else if (isJumpNode(g, x, y, z, 0, -1)) {
                    node->isJumpNode = 1;
                } else if (isJumpNode(g, x, y, z, 1, 0)) {
                    node->isJumpNode = 1;
                } else if (isJumpNode(g, x, y, z, -1, 0)) {
                    node->isJumpNode = 1;
                }


            }
        }
    }


    // FIXME, or well UNDERSTAND ME BETTER
    // in the cardinal directions I changed all if (node->isJumpNode && countMovingWest > 0)
    // to the much simpler if (node->isJumpNode)
    // in the cpp project on github, they mask it with a bitmask
    // https://github.com/SteveRabin/JPSPlusWithGoalBounding/blob/master/JPSPlusGoalBounding/PrecomputeMap.cpp
    // In the book this algorithm first occured (Game AI Pro 2 page 138)
    // they do it like I've done here now (without the && countBla > 0)

    for (int z =0; z < depth; z++) {
        // first we do east and west
        for (int y = 0; y < height; y++) {
            {
                // distance  west

                int countMovingWest = -1;
                int jumpPointLastSeen = 0;
                for (int x = 0; x < width; x++) {
                    int index = FLATTEN_3D_INDEX(x,y,z, width, height);
                    grid_node  *node = &g->nodes[index];

                    if (isWall(x,y,z)) {
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
                for (int x = width-1; x >= 0; x--) {
                    int index = FLATTEN_3D_INDEX(x,y,z, width, height);
                    grid_node  *node = &g->nodes[index];
                    if (isWall(x,y,z)) {
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
                    int index = FLATTEN_3D_INDEX(x,y,z, width, height);
                    grid_node  *node = &g->nodes[index];

                    if (isWall(x,y,z)) {
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
                for (int y = height-1; y>=0; y--) {
                    int index = FLATTEN_3D_INDEX(x,y,z, width, height);
                    grid_node  *node = &g->nodes[index];
                    if (isWall(x,y,z)) {
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

                if (isEmpty(x,y,z)) {
                    int index = FLATTEN_3D_INDEX(x,y,z, width, height);
                    grid_node  *node = &g->nodes[index];
                    // North West
                    if (y == 0 || x == 0 || isWall(x, y-1,z) || isWall(x-1, y,z ) || isWall(x-1, y-1, z))
                    {
                        node->distance[nw] = 0;
                    }
                    else if (isEmpty(x-1,y,z) && isEmpty(x,y-1,z) &&
                             ((GetNodeAt(g,x-1, y-1, z)->distance[north]  > 0) ||
                              (GetNodeAt(g,x-1, y-1, z)->distance[west] > 0)))
                    {
                        node->distance[nw] = 1;
                    }
                    else
                    { // start incrementing
                        int jumpDistance = GetNodeAt(g, x-1, y-1, z)->distance[nw];
                        if (jumpDistance > 0) {
                            node->distance[nw] = 1 + jumpDistance;
                        } else { // jumpDistance <= 0
                            node->distance[nw] = -1 + jumpDistance;
                        }
                    }

                    // North East
                    if (y == 0 || x == width-1 || (isWall(x, y-1, z) || isWall(x+1, y, z) || isWall(x+1, y-1, z)))
                    {
                        node->distance[ne] = 0;
                    }
                    else if (isEmpty(x, y-1, z) && isEmpty(x+1, y, z) &&
                             (( GetNodeAt(g,x+1, y-1, z)->distance[north] > 0) ||
                              GetNodeAt(g, x+1, y-1, z)->distance[east] > 0))
                    {
                        node->distance[ne] = 1;
                    }
                    else
                    {
                        int jumpDistance = GetNodeAt(g, x+1, y-1, z)->distance[ne];
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
        for (int y = height-1; y >= 0; y--) {
            for (int x = 0; x < width; x++) {
                if (isEmpty(x,y,z)) {
                    // South West
                    int index = FLATTEN_3D_INDEX(x,y,z, width, height);
                    grid_node  *node = &g->nodes[index];
                    if (y == height-1 || x == 0 || isWall(x, y+1, z) || isWall(x-1, y, z) ||  isWall(x-1, y+1, z))
                    {
                        node->distance[sw] = 0;
                    } else if (isEmpty(x, y+1, z) && isEmpty(x-1, y, z) &&
                               (GetNodeAt(g, x-1, y+1, z)->distance[south] > 0 ||
                                GetNodeAt(g, x-1, y+1, z)->distance[west] > 0))
                    {
                        node->distance[sw] = 1;
                    }
                    else
                    {
                        int jumpDistance = GetNodeAt(g, x-1, y+1, z)->distance[sw];
                        if (jumpDistance > 0) {
                            node->distance[sw] = 1 + jumpDistance;
                        } else {
                            node->distance[sw] = -1 + jumpDistance;
                        }
                    }

                    // South East
                    if (y == height -1 || x == width -1 || isWall(x, y+1, z) || isWall(x+1, y, z) || isWall(x+1, y+1, z)) {
                        node->distance[se] = 0;
                    } else if (isEmpty(x, y+1, z) && isEmpty(x+1, y, z) &&
                               (GetNodeAt(g,x+1, y+1, z)->distance[south] > 0 ||
                                GetNodeAt(g,x+1, y+1, z)->distance[east] > 0))
                    {
                        node->distance[se] = 1;
                    }
                    else {
                        int jumpDistance = GetNodeAt(g, x+1, y+1, z)->distance[se];
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


#define SET_ALLOWED(travelling) allowed = travelling; allowedSize = sizeof(travelling)/sizeof(allowed[0]);

#define IS_CARDINAL(direction) (direction == north || direction == south || direction == west || direction == east || direction == up || direction == down)


grid_node * getNodeGivenDirectionDiagonalHack(grid_node * Current, jump_direction direction, grid * Grid, int dx, int dy) {
    int x = Current->X;
    int y = Current->Y;
    int z = Current->Z;
    grid_node *Result;
    switch(direction) {
    case ne:
        Result = GetNodeAt(Grid,x+dx, y-dy, z);
        break;
    case se:
        Result = GetNodeAt(Grid,x+dx, y+dy, z);
        break;
    case nw:
        Result = GetNodeAt(Grid,x-dx, y-dy, z);
        break;
    case sw:
        Result = GetNodeAt(Grid,x-dx, y+dy, z);
        break;
    case north: case east: case west: case south: case up: case down:
        break;
    }

    return Result;
}

grid_node * getNodeGivenDirectionAndDistance(grid_node * Current, jump_direction direction, grid * Grid, int distance ){
    //    printf("distance: %d\n", distance);
    int x = Current->X;
    int y = Current->Y;
    int z = Current->Z;
    grid_node *Result;

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
        Result = GetNodeAt(Grid,x, y, z+distance);
        break;
    case down:
        Result = GetNodeAt(Grid, x, y, z-distance);
        break;

    }

    return Result;
}

grid_node * getNodeGivenDirection(grid_node * Current, jump_direction direction, grid * Grid) {
    int distance = Current->distance[direction];
    return getNodeGivenDirectionAndDistance(Current, direction, Grid, distance);
}



jump_direction getDirection(grid_node *Current, grid_node *Parent) {
    int dx = Current->X - Parent->X;
    int dy = Current->Y - Parent->Y;
    int dz = Current->Z - Parent->Z;
    jump_direction Result;
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

static inline int diffXY(grid_node *Current, grid_node *Parent) {
    int dx = Current->X - Parent->X;
    int dy = Current->Y - Parent->Y;
    //if (dy) assert(dx==0);
    //if (dx) assert(dy==0);
    return ABS(dx+dy);
}

static inline int otherIsInGeneralDirectionOfDiagonal(jump_direction diagonal, jump_direction other) {
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



path_list * FindPathPlus(grid_node * startNode, grid_node * endNode, grid * Grid, memory_arena * Arena) {
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

    char* names[10] = {
        "north",
        "east",
        "south",
        "west",
        "ne",
        "se",
        "sw",
        "nw",
        "up",
        "down"
    };

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
            //printf("parent, delta %d %d %d\n", dx, dy, dz);

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
        //printf("Node %d,%d,%d \n",Node->X, Node->Y, Node->Z);
        for (int i = 0; i < allowedSize; i++) {
            int direction = allowed[i];
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
                    printf("node z %d, succ z: %d\n",Node->Z, Successor->Z);
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
}

#endif
