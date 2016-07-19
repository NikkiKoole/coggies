// This little commandline app will create an empty map text file
// usage write_map empty,10,10,10,1
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



static void build_line_in_parts(char left, char middle, int middle_length, char right, char * buffer){
    char result[middle_length+2];
    int index = 0;
    result[0] = left;
    for (index=0; index<middle_length; ++index){
        result[index+1]= middle;
    }
    index++;
    result[index] = right;
    result[index+1]='\n';
    result[index+2]=0;

    strcpy(buffer, result);
}


static int write_map(int width, int depth, int height, char * name){
    FILE *f = fopen(name, "w");
    if (!f) {
        printf("Couldnt open file: %s\n",name);
    }
    fprintf(f, "MAP\n"); //file header
    fprintf(f, "1\n"); // version
    fprintf(f, "%d, %d, %d\n",width, depth, height); // dimensions

    char *line;

    int i,j = 0;
    for (i = 0; i < height; i++) {
        build_line_in_parts('+','-',width,'+',line);
        fprintf(f, line);

        for (j = 0; j < depth; ++j) {
            build_line_in_parts('|',' ',width,'|',line);
            fprintf(f, line);
        }
    }
    // final line
    build_line_in_parts('+','-',width,'+',line);
    fprintf(f, line);

    fclose(f);
}


int main(int argc, char* argv[]) {

    if (argc != 5) {
        printf("Incorrect amount of arguments.\n\n\n");
        goto info;
    } else {
        int x=0, z_level=0, y=0;
        char* name;

        if (atoi(argv[1])) {
            x = atoi(argv[1]);
        }
        if (atoi(argv[2])) {
            y = atoi(argv[2]);
        }
        if (atoi(argv[3])) {
            z_level = atoi(argv[3]);
        }
        name = argv[4];

        printf("%d, %d, %d, %s\n",x,y,z_level,name);
        if (x && z_level && y) {
            write_map(x, y, z_level,name);
            return;
        } else {
            printf("dimensions are wrong.\n\n\n");
            goto usage;
        }
    }

    
 info:
    printf("Little empty map creation tool.\n");
    printf("x & y form a plane, z_level is the amount of these planes.\n");
    printf("\n");
 usage:
    printf("Usage:\n");
    printf(BOLDWHITE "\twrite_map x y z_level name\n");
}
