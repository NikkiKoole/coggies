#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int starts_with(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}


typedef struct {
    int width;
    int depth;
    int height;
} World_Size;

World_Size validate_and_get_dimensions(char * path){
    World_Size result;
#define BUF_SIZE 1024
    char str[BUF_SIZE];

    FILE *f = fopen(path, "rb");
    
    if (!f) {
        printf("Couldn't open file: %s\n", path);
    }

    // first  line should contain MAP string
    if (fgets(str, BUF_SIZE, f) != NULL) {
        printf("map header found ? %d\n", starts_with("MAP", str));
    }

    //second line should contain some version info
    if (fgets(str, BUF_SIZE, f) != NULL) {
        if (atoi(str) != 1) {
            printf("I only grasp version 1.0 at the moment\n");
        } else {
            printf("version %d\n", atoi(str));
        }
    }
    
    // third line should contain 3 values, with with commas inbetween, width/depth,height
    if (fgets(str, BUF_SIZE, f) != NULL) {
        char * pch;
        pch = strtok(str,",");
        result.width = atoi(pch);
        pch = strtok(NULL, ",");
        result.depth = atoi(pch);
        pch = strtok(NULL, ",");
        result.height = atoi(pch);
        printf("width(x) %d, depth(z) %d, height(y) %d\n", result.width, result.depth, result.height);
    }
    fclose(f);
    return result;
}



typedef struct {
    int floor;
    int wall;
    
}WorldBlock;

typedef struct {
    WorldBlock blocks[1024];
}WorldMap;











int main() {
    #define LINES_BEFORE_DATA 3
    #define BUF_SIZE 1024
    char str[BUF_SIZE];
    char *path = "test2.txt";

    
    World_Size dims = validate_and_get_dimensions(path);
    FILE *f = fopen(path, "rb");
    int line_counter = LINES_BEFORE_DATA; // because validate has already read some lines
    int height_counter = -1;

    for (int i = 0; i < 255; ++i){
        printf("%d) %c\n",i, i);
    }
    printf("\u2500\u2501\u2502");

    
    // dont need to interpret, vaidate and dims already did that.
    for (int i = 0; i<LINES_BEFORE_DATA;i++){
        if (fgets(str, BUF_SIZE, f) != NULL) {
        }
    }


    while (fgets(str, BUF_SIZE, f) != NULL) {
        // ensure data is valid, measure widths heights depths etc.
        // see if the corner '+' signs are in the expected locations
        if ((line_counter-LINES_BEFORE_DATA) % (dims.depth+1) == 0 ) {
            if (str[0] == '+' && str[dims.width+1] == '+') {
                height_counter++;
                if (height_counter < dims.height) {
                    //printf("check! height = %d", height_counter);
                }
            } else {
                printf("missing + signs at the corners.\n");
            }
        } else {
            if (str[0] == '|' && str[dims.width+1] == '|') {
                printf("X %3d,%d, %s ",((line_counter-LINES_BEFORE_DATA) % (dims.depth+1))-1, height_counter ,str);
                // this is where its at


                
            } else {
                printf("missing | signs at begin and end. %s.\n",str);

            }
        }
        line_counter++;
    }
    printf("\n");
    
}
