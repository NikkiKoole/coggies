#include "jsmn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

const char *readFile(const char *path) {
    char *buffer = 0;
    long length = 0;
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("Couldn't open file: %s\n", path);
    }
    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(length + 1);
        if (buffer) {
            fread(buffer, 1, length, f);
        }
        fclose(f);
    }
    if (buffer) {
        buffer[length] = 0;
        return buffer;
    }
    return "";
}

struct sprite_frame {
    char name[64];
    int frameX;
    int frameY;
    int frameW;
    int frameH;
    int spriteSourceX;
    int spriteSourceY;
    int spriteSourceW;
    int spriteSourceH;
    int sourceW;
    int sourceH;
};

int main() {
    jsmn_parser parser;
    jsmntok_t t[1000];

    jsmn_init(&parser);
    const char* js = readFile("./test2.json");
    int i, r;
    r = jsmn_parse(&parser, js, strlen(js), t, sizeof(t)/sizeof(t[0]));
    if (r < 0) {
        printf("failed to parse JSON %d\n", r);
    }

    int framesCount = 0;
    for (i = 0; i < r; i++) {
        if (jsoneq(js, &t[i], "frame") == 0) {
            framesCount++;
        }
    }

    // now I should prepare an array of unfilled struct sprite_frame
    // and start filling the individual structs with the data.

    for (i = 0; i < r; i++) {
        if (jsoneq(js, &t[i], "frames") == 0) {
            printf("frames to be found\n");
            continue;
        }
        if (jsoneq(js, &t[i], "meta") == 0) {
            printf("No more frames to be found\n");
            continue;
        }
        if (jsoneq(js, &t[i], "frame") == 0) {
            printf("frame data\n");
            i+=9;
            continue;
        }
        if (jsoneq(js, &t[i], "spriteSourceSize") == 0) {
            printf("spritesource size  data\n");
            i+=9;
            continue;
        }
        if (jsoneq(js, &t[i], "sourceSize") == 0) {
            printf("source size  data\n");
            i+=5;
            continue;
        }
        if (t[i].type ==  JSMN_STRING) {
            printf("index: %d content: %.*s\n",i, t[i].end-t[i].start, js + t[i].start);
        }

        //i+=1;
        //i++;

    }
    printf("framecount: %d\n", framesCount);
}
