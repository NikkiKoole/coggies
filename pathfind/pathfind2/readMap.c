
void readMapString(const char* string, map_file *Result) {
    char* buffer = 0;
    int i=0;
    while (string[i] != '\0') {
        i++;
    }
    int length = i;
    int width=0, height=0, depth=0;
    int line_length = 0;
    int block_height = 0;
    int count = 0;

    int startX=0, startY=0, startZ=0;
    int endX=0, endY=0, endZ=0;
    int startSet = 0;
    int endSet = 0;


    for (int i = 0; i < length; i++) {
        if (string[i] == '\n') {
            if (line_length > 0) {
                width = line_length;
                line_length = 0;
                block_height++;
                height = block_height;
            } else {
                block_height = 0;
                depth++;
            }
        } else {
            if (string[i] == 'S') {
                if (startSet) {
                    printf("Error multiple start positions\n");
                }
                startSet = 1;
                startX = line_length;
                startY = block_height;
                startZ = depth;
            }
            if (string[i] == 'E') {
                if (endSet) {
                    printf("Error multiple end positions\n");
                }
                endSet = 1;
                endX = line_length;
                endY = block_height;
                endZ = depth;
            }
            line_length++;
            count++;
        }
    }
    depth = depth+1;
    //printf("%d, %d, %d\n",width,height,depth);
    // printf("START: %d, %d, %d\n",startX,startY,startZ);
    //printf("END: %d, %d, %d\n",endX,endY,endZ);
    if (!startSet || !endSet) {
        printf("error missing start or endpoint\n");
    }
    Result->startX = startX;
    Result->startY = startY;
    Result->startZ = startZ;
    Result->endX = endX;
    Result->endY = endY;
    Result->endZ = endZ;

    Result->width = width;
    Result->height = height;
    Result->depth = depth;
    // TODO: use push_size instead of malloc;
    //Result->chars = malloc(width*height*(depth));
    Result->data = malloc(width*height*(depth));
    int writeIndex  =0;
    for (int readIndex = 0; readIndex < length; readIndex++) {
        if (!(string[readIndex] == '\n')) {
            const char s = string[readIndex];
            //Result->chars[writeIndex] = string[readIndex];
            if (s == '#' || s == '0') { /* Wall & Air */
                Result->data[writeIndex] = 9; // impassable
            } else if (s == '.') {
                Result->data[writeIndex] = 1; // walkable
            } else if (s == 'U') {
                Result->data[writeIndex] = 2; // stairs up
            } else if (s == 'D') {
                Result->data[writeIndex] = 3; // stairs down
            } else if (s == 'Z') {
                Result->data[writeIndex] = 4; // stairs up &down
            } else if (s == 'S' || s == 'E') {  // lets assume the start and end are walkable
                Result->data[writeIndex] = 1; // walkable
            } else {
                printf("reading unknow charcter in file %c ?\n", s);
            }

            writeIndex++;
        }
    }
    free(buffer);
}


void readMapFile(const char* path, map_file *Result) {
    // # is wall
    // . is walkable ground
    // 0 is unwalkable (air)
    // S is startPoint
    // E is endpoint
    // U is stairs up
    // D is stairs down
    // Z is stairs up & down

    char * buffer = 0;
    long length = 0;
    FILE *f = fopen(path, "rb");

    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(length);
        if (buffer) {
            fread(buffer, 1, length, f);
        }
        fclose(f);
    } else {
        printf("Error opening : %s\n", path);
    }

    readMapString(buffer, Result);
    free(buffer);
}
