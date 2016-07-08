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
        int width=0, height=0, depth=0;
        char* name;

        if (atoi(argv[1])) {
            width = atoi(argv[1]);
        }
        if (atoi(argv[2])) {
            depth = atoi(argv[2]);
        }
        if (atoi(argv[3])) {
            height = atoi(argv[3]);
        }
        name = argv[4];

        printf("%d, %d, %d, %s\n",width,depth,height,name);
        if (width && height && depth) {
            write_map(width, depth, height,name);
            return;
        } else {
            printf("dimensions are wrong.\n\n\n");
            goto usage;
        }
    }

    
 info:
    printf("Little empty map creation tool.\n");
    printf("Width & depth form a plane, height is the amount of these planes.\n");
    printf("\n");
 usage:
    printf("Usage:\n");
    printf(BOLDWHITE "\twrite_map width depth height name\n");
}
