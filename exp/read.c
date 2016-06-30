#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int starts_with(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}


int main() {

    #define BUF_SIZE 1024
    char str[BUF_SIZE];
    const char *path = "test.txt";
    FILE *f = fopen(path, "rb");

    int width = 0; //x
    int height = 0; //y
    int depth = 0; //z
    int line_counter = 0;
    int height_counter = -1;
    if (!f) {
        printf("Couldn't open file: %s\n", path);
    }

    // first  line should contain MAP string
    if (fgets(str, BUF_SIZE, f) != NULL) {
        printf("map header found ? %d\n", starts_with("MAP", str));
    }
    line_counter++;

    // second line should contain 3 values, with with commas inbetween, width/depth,height
    if (fgets(str, BUF_SIZE, f) != NULL) {
        char * pch;
        pch = strtok(str,",");
        width = atoi(pch);
        pch = strtok(NULL, ",");
        depth = atoi(pch);
        pch = strtok(NULL, ",");
        height = atoi(pch);
        printf("width(x) %d, depth(z) %d, height(y) %d\n", width, depth, height);
    }
    line_counter++;

    while (fgets(str, BUF_SIZE, f) != NULL) {
        // ensure data is valid, measure widths heights depths etc.
        // see if the corner + are in the expected locations
        if ((line_counter-2) % depth == 0 ) {
            if (str[0] == '+' && str[width+2] == '+') {
                height_counter++;
                if (height_counter < height) {
                    printf("check! height = %d", height_counter);
                }
            } else {
                printf("problems this file could be corrupted!\n");
            }
        } else {
            printf("X %3d,%d ",((line_counter-2) % depth)-1, height_counter ); // these lines must be read and put in map.
        }


        printf("%4d %s",line_counter, str);




        line_counter++;
    }
}
