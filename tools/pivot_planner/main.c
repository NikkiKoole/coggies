#include "jsmn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

typedef struct {
    int x, y, w, h;
} LocationAndDimension;

typedef struct {
    int w, h;
} Dimension;

typedef struct {
    char * name;
    LocationAndDimension frame;
    LocationAndDimension spriteSourceSize;
    Dimension sourceSize;
} Frame;

typedef struct {
    char * name;
    int frameX, frameY, frameW, frameH;
    int sssX, sssY, sssW, sssH;
    int ssW, ssH;
} SimpleFrame;


typedef struct {
    char *name;
    SimpleFrame frames[1024];
    int frame_count;
} TextureAtlasSingles;


typedef struct {
    char *name;
    Frame frames[64];
    int frame_count;
} TextureAtlasItem;

typedef struct {
    char *name;
    int child_count;
    int is_solo;
    TextureAtlasItem children[16]; // used to put the layer children.
    TextureAtlasItem own_data; // used if this thing is a solo object or to put the group data itself
} TextureAtlasGroupOrSolo;

typedef struct {
    int total;
    TextureAtlasGroupOrSolo data[1024];
} TextureAtlasLayerValues;

typedef struct {
    int x, y;
} Point;


typedef struct {
    // it has at its base frames + pivot and/or anchorpoints.
    LocationAndDimension frame;
    LocationAndDimension spriteSourceSize;
    Dimension sourceSize;
    Point pivot;
    Point anchor_1;//todo make this an array maybe

} CombinedFrame;

typedef struct {
    char *layer_name;
    char *group_name;
    char *file_name;

    int frame_count;
    CombinedFrame frames[64];
} CombinedItem;

typedef struct {
    int count;
    CombinedItem items[1024];
} CombinedSets;



TextureAtlasLayerValues all_data;
//TextureAtlasLayerValues shoe_data;
TextureAtlasLayerValues pixel_layer;
CombinedSets combined;
TextureAtlasSingles singles;

jsmntok_t all_tokens[1024 * 256];
//jsmntok_t shoe_tokens[1024 * 256];
jsmntok_t pixel_tokens[1024 * 256];



int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
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


void analyze_layers(jsmntok_t *t, int i, const char* js, TextureAtlasLayerValues *r, int expectJustChildren) {
    int count = t[i].size;
    int j = 0;
    int current = 0;
    char substr[128];

    i = i+1;

    for (j = 0; j < count; j++) {
        if (t[i].size == 4 ) { //child of group

            strncpy(substr, js + t[i+2].start, t[i+2].end-t[i+2].start);
            substr[t[i+2].end-t[i+2].start] = '\0';
            r->data[current].children[r->data[current].child_count].name = strdup(substr);
            r->data[current].child_count++;

            if (expectJustChildren) {
                strncpy(substr, js + t[i+4].start, t[i+4].end-t[i+4].start);
                substr[t[i+4].end-t[i+4].start] = '\0';
                r->data[current].name = strdup(substr);
                current++;
            }

        } else if (t[i].size == 1) { //group identifier
            strncpy(substr, js + t[i+2].start, t[i+2].end-t[i+2].start);
            substr[t[i+2].end-t[i+2].start] = '\0';

            r->data[current].name = strdup(substr);

            current++;
        } else if (t[i].size == 3) {
            strncpy(substr, js + t[i+2].start, t[i+2].end-t[i+2].start);
            substr[t[i+2].end-t[i+2].start] = '\0';

            r->data[current].name = strdup(substr);
            r->data[current].child_count=0;
            current++;
        } else {
            printf("%d) derp: size(%d) %.*s\n", i, t[i].size, t[i].end-t[i].start, js + t[i].start);
        }

        i += 1+(t[i].size)*2;
    }
    r->total = current;
}


int get_first_index_of_char(const char* str, char find, int start_index) {
    int i;
    for (i = start_index; i < strlen(str); i++) {
        if (str[i] == find) {
            return i;
        }
    }
    return -1;
}

int get_last_index_of_char(const char* str, char find, int start_index) {
    int i;
    for (i = start_index; i > 0; i--) {
        if (str[i] == find) {
            return i;
        }

    }
    return -1;
}


int frame_has_cell_index(const char* str) {
    int space_index = get_last_index_of_char(str, ' ', strlen(str));
    int dot_index = get_last_index_of_char(str, '.', strlen(str));
    int ret;

    if (! isalnum(str[dot_index-1])) {
        ret = 0;
    } else {
        ret = strtol(str+ (space_index), NULL, 10);

    }
    return ret;
}

char* frame_has_file(const char* str) {
    char result[128];
    int index = get_first_index_of_char(str, '(', 0);

    strncpy(result, str, index);
    result[index] = '\0';

    return strdup(result);
}

char* frame_has_layer(const char* str) {
    char result[128];

    int openbrace = get_first_index_of_char(str, '(', 0);
    int closebrace = get_first_index_of_char(str, ')', openbrace);

    strncpy(result, str+openbrace+1, closebrace-openbrace-1);
    result[closebrace-openbrace-1] = '\0';
    return strdup(result);
}


typedef struct {
    int parent;
    int child;
    int points_to_group;
    int is_solo;
} ParentChildIndex;

ParentChildIndex get_best_parent_child(int parent, int child, char* name, TextureAtlasLayerValues *r) {
    ParentChildIndex result = {parent, child, 0, 0};
    int test_parent = parent;
    int test_child = child;
    int score_to_beat = r->data[test_parent].child_count > 0 ?
        strcmp(r->data[parent].children[child].name, name) :
        strcmp(r->data[parent].name, name);

    for (test_parent = parent; test_parent < r->total; test_parent++) {

        int child_start = test_parent == parent ? child :  0;

        if (r->data[test_parent].child_count == 0) {
            if (strcmp(r->data[test_parent].name, name) == 0) {
                result.parent = test_parent;
                result.child = test_child;
                result.is_solo = 1;
                goto leave;
            }
        } else {
            for (test_child = child_start; test_child < r->data[test_parent].child_count; test_child++) {
                if (strcmp(r->data[test_parent].name, name) == 0) {
                    result.points_to_group = 1;
                    result.parent = test_parent;
                    result.child = test_child;
                    goto leave;

                } else if (strcmp(r->data[test_parent].children[test_child].name, name) == 0 ) {
                    result.parent = test_parent;
                    result.child = test_child;
                    goto leave;

                }
            }
        }
    }

 leave:
    return result;

}

int get_value(jsmntok_t *t, const char* str, int offset) {
    char clone[1024];
    int ret;
    strncpy(clone, str + t[offset].start, t[offset].end-t[offset].start);
    clone[t[offset].end-t[offset].start] = '\0';

    ret = strtol(clone, NULL, 10);
    return (int) ret;
}

char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}


void analyze_frames(jsmntok_t *t, int i, const char* str, TextureAtlasLayerValues *r, int advanceComplex) {
    int count = t[i].size;
    int j =0;
    int size_per_elem = 35;
    int parent_index = 0;
    int child_index = 0;
    char substr[128];
    char* last_frame_file = "";
    char* last_frame_layer = "";
    int last_frame_index =0;
    int items_seen = 0;

    i = i+1;
    for (j = 0; j < count; j++) {

        if (t[i].size == 7) {
            strncpy(substr, str + t[i+2].start, t[i+2].end-t[i+2].start);
            substr[t[i+2].end-t[i+2].start] = '\0';

            int file_has_changed = ( strcmp(frame_has_file(substr), last_frame_file) != 0);
            int layer_has_changed = ( strcmp(frame_has_layer(substr), last_frame_layer) != 0);
            ParentChildIndex best;

            if (advanceComplex == 0) {
                best = get_best_parent_child(parent_index, child_index, frame_has_layer(substr), r);
            } else {
                if (items_seen == 0) {
                    best = get_best_parent_child(parent_index, child_index, frame_has_layer(substr), r);
                } else {
                    if (file_has_changed || layer_has_changed || last_frame_index > frame_has_cell_index(substr)) {
                        if (child_index < r->data[parent_index].child_count-1) {
                            best = get_best_parent_child(parent_index, child_index+1, frame_has_layer(substr), r);
                        } else {
                            if (parent_index+1 < r->total) {
                                best = get_best_parent_child(parent_index+1, 0, frame_has_layer(substr), r);
                            } else {
                                best = get_best_parent_child(parent_index, child_index, frame_has_layer(substr), r);
                            }
                        }
                    } else {
                        best = get_best_parent_child(parent_index, child_index, frame_has_layer(substr), r);
                    }
                }
            }

            child_index = best.child;
            parent_index = best.parent;

            Frame *frame;

            if (!(best.points_to_group || best.is_solo)) {
                r->data[parent_index].children[child_index].frame_count++;
                frame = &(r->data[parent_index].children[child_index].frames[frame_has_cell_index(substr)]) ;
            } else {
                r->data[parent_index].own_data.frame_count++;
                frame = &(r->data[parent_index].own_data.frames[frame_has_cell_index(substr)]);
            }

            r->data[parent_index].is_solo = best.is_solo;


            frame->name = strdup(frame_has_file(substr));
            frame->frame.x = get_value(t, str, i+6);
            frame->frame.y = get_value(t, str, i+8);
            frame->frame.w = get_value(t, str, i+10);
            frame->frame.h = get_value(t, str, i+12);

            frame->spriteSourceSize.x = get_value(t, str, i+20);
            frame->spriteSourceSize.y = get_value(t, str, i+22);
            frame->spriteSourceSize.w = get_value(t, str, i+24);
            frame->spriteSourceSize.h = get_value(t, str, i+26);
            //printf("spritesource : %d, %d, %d, %d\n", frame->spriteSourceSize.x, frame->spriteSourceSize.y, frame->spriteSourceSize.w, frame->spriteSourceSize.h);
            frame->sourceSize.w = get_value(t, str, i+30);
            frame->sourceSize.h = get_value(t, str, i+32);
            //printf("spritesource : %d, %d \n", frame->sourceSize.w, frame->sourceSize.h);


            last_frame_file = frame_has_file(substr);
            last_frame_layer = frame_has_layer(substr);
            last_frame_index = frame_has_cell_index(substr);
        }
        items_seen++;
        i += size_per_elem;

    }
    assert(r->total == parent_index+1);
}


int has_layer_with_name(TextureAtlasGroupOrSolo thing, const char* str) {
    int j;
    for(j =0; j< thing.child_count; j++) {
        if (strcmp( thing.children[j].name, str) == 0) {
            return 1;
        }
    }
    return 0;

}



TextureAtlasItem* get_pixel_layer(TextureAtlasLayerValues *pixel_layer,  char * file_name, char * group_name, char * layer_name) {
    int i = 0;
    for (i = 0; i < pixel_layer->total;i++) {
        //printf("%s, %s\n", pixel_layer->data[i].name, group_name);
        if (strcmp(pixel_layer->data[i].name, group_name) == 0) {
               if (strcmp(pixel_layer->data[i].children[0].frames[0].name, file_name) == 0) {
                if (strcmp(pixel_layer->data[i].children[0].name, layer_name) == 0 ){
                    return &(pixel_layer->data[i].children[0]);
                }
            }
        }
    }
    return NULL;
}

TextureAtlasItem* get_pixel_layer_simpler(TextureAtlasLayerValues *pixel_layer,  char * file_name, char * group_name) {
    int i = 0;
    for (i = 0; i < pixel_layer->total;i++) {
        if ( strcmp(pixel_layer->data[i].name, group_name)==0 && strcmp(pixel_layer->data[i].own_data.frames[0].name, file_name) == 0) {
            return &(pixel_layer->data[i].own_data);
        }

    }
    return NULL;
}

TextureAtlasItem* get_layer_with_name(TextureAtlasGroupOrSolo *thing, const char* str) {
    int j;
    for(j =0; j< thing->child_count; j++) {
        printf("child name: %s,\n",thing->children[j].name);
        if (strcmp( thing->children[j].name, str) == 0) {
            return &(thing->children[j]);
        }
    }
    return NULL;

}

void combine_blocks(TextureAtlasLayerValues *pixel_layer, TextureAtlasLayerValues *all_data, CombinedSets *combined){
    int i, j, k;
    int index = 0;
    char *prefix = "BL";
    FILE *fptr;
    fptr = fopen("blocks.txt", "w");
    fprintf(fptr, "//this blocks file is autogenerated by tools/pivot_planner, don't edit this file, instead use pivotplanner to generate it.\n");
    fprintf(fptr, "enum BlockParts {\n");

    int counter = 0;

    int item_count = 0;
    for (i = 0; i< all_data->total;i++){
        assert(all_data->data != NULL);
        assert(all_data->data[i].own_data.frames[0].name != NULL);
        assert(all_data->data[i].name != NULL);

        CombinedItem * item = &(combined->items[i/2]);
        Frame * frame = NULL;
        item->file_name  = strdup(all_data->data[i].own_data.frames[0].name);
        item->frame_count = all_data->data[i].own_data.frame_count;



        if ( strcmp(all_data->data[i].name,"pixels")==0 ) {
            TextureAtlasItem* pixel = get_pixel_layer_simpler(pixel_layer,
                                                              all_data->data[i].own_data.frames[0].name,
                                                              all_data->data[i].name);
            assert(pixel);

            for (j =0; j< pixel->frame_count; j++) {
                frame= &pixel->frames[j];
                item->frames[j].frame.x            = frame->frame.x;
                item->frames[j].frame.y            = frame->frame.y;
                item->frames[j].frame.w            = frame->frame.w;
                item->frames[j].frame.h            = frame->frame.h;
                item->frames[j].spriteSourceSize.x = frame->spriteSourceSize.x;
                item->frames[j].spriteSourceSize.y = frame->spriteSourceSize.y;
                item->frames[j].spriteSourceSize.w = frame->spriteSourceSize.w;
                item->frames[j].spriteSourceSize.h = frame->spriteSourceSize.h;
                item->frames[j].sourceSize.w       = frame->sourceSize.w;
                item->frames[j].sourceSize.h       = frame->sourceSize.h;
            }
        }
        if (strcmp(all_data->data[i].name,"pivot")==0) {
            for (j = 0; j < all_data->data[i].own_data.frame_count; j++) {
                frame = &all_data->data[i].own_data.frames[j];
                item->frames[j].pivot.x            = frame->spriteSourceSize.x;
                item->frames[j].pivot.y            = frame->spriteSourceSize.y;
            }


        }
    }
    counter = 0;
    for ( i = 0; i<all_data->total/2; i++) {
        CombinedItem * item = &(combined->items[i]);
        for (j = 0; j < item->frame_count; j++) {
            fprintf(fptr, "    %s_%s_%03d = %d,\n",
                    prefix,
                    trimwhitespace(item->file_name),
                    j,
                    counter);
            counter++;
        }
    }
    fprintf(fptr, "    %s_TOTAL = %d\n", prefix, counter);
    fprintf(fptr, "};\n");
    fprintf(fptr, "internal void fill_generated_values(SimpleFrame* generated_frames ) {\n");

    for ( i = 0; i<all_data->total/2; i++) {
        CombinedItem * item = &(combined->items[i]);
        for (j = 0; j < item->frame_count; j++) {
            //printf("%s)  xy) (%d, %d) \n", trimwhitespace(item->file_name), item->frames[j].frame.x, item->frames[j].frame.y);
            fprintf(fptr, "    generated_frames[%s_%s_%03d] = (SimpleFrame){%d, %d, %d, %d,%d, %d, %d, %d, %d, %d, %d, %d};",
                    prefix,
                    trimwhitespace(item->file_name),
                    j,
                    item->frames[j].frame.x,
                    item->frames[j].frame.y,
                    item->frames[j].frame.w,
                    item->frames[j].frame.h,
                    item->frames[j].spriteSourceSize.x,
                    item->frames[j].spriteSourceSize.y,
                    item->frames[j].spriteSourceSize.w,
                    item->frames[j].spriteSourceSize.h,
                    item->frames[j].sourceSize.w,
                    item->frames[j].sourceSize.h,
                    item->frames[j].pivot.x,
                    item->frames[j].pivot.y);
            fprintf(fptr, "\n");
        }

    }

    // now we have the items


    fprintf(fptr, "};\n");
    fclose(fptr);
    printf("written %d frames.\n", counter);
}



void combine_bodies(TextureAtlasLayerValues *pixel_layer, TextureAtlasLayerValues *all_data, CombinedSets *combined){
    int i, j, k;
    int index = 0;
    char *prefix = "BP";
    FILE *fptr;
    fptr = fopen("bodies.txt", "w");
    fprintf(fptr, "//this bodies file is autogenerated by tools/pivot_planner, don't edit this file, instead use pivotplanner to generate it.\n");
    fprintf(fptr, "enum BodyParts {\n");

    int counter = 0;
    for (i = 0; i< all_data->total;i++){
        assert(all_data->data != NULL);
        assert(all_data->data[i].own_data.frames[0].name != NULL);
        assert(all_data->data[i].name != NULL);

        TextureAtlasItem* pivot = get_layer_with_name(&all_data->data[i], "pivot");
        TextureAtlasItem* anchor_1 = get_layer_with_name(&all_data->data[i], "anchor_1");
        TextureAtlasItem* pixel = get_pixel_layer(pixel_layer,
                                                  all_data->data[i].own_data.frames[0].name,
                                                  all_data->data[i].name,
                                                  all_data->data[i].children[0].name);

        assert(pivot);

        if (pivot && anchor_1 && pixel) {
            CombinedItem * item = &(combined->items[combined->count]);

            assert(pivot->frame_count == anchor_1->frame_count);
            assert(anchor_1->frame_count == pixel->frame_count);

            item->file_name  = strdup(all_data->data[i].own_data.frames[0].name);
            item->group_name = strdup(all_data->data[i].name);
            item->layer_name = strdup(all_data->data[i].children[0].name);

            for (j = 0; j < pivot->frame_count; j++) {
                fprintf(fptr, "    %s_%s_%s_%03d = %d,\n",
                        prefix,
                        trimwhitespace(item->file_name),
                        trimwhitespace(item->group_name),
                        j,
                        counter);
                counter++;
            }
        }
    }
    fprintf(fptr, "    %s_TOTAL = %d\n", prefix, counter);
    fprintf(fptr, "};\n");
    fprintf(fptr, "internal void fill_generated_complex_values(FrameWithPivotAnchor* generated_frames ) {\n");

    for (i = 0; i< all_data->total;i++) {
        if (all_data->data[i].child_count > 0) {

            assert(all_data->data != NULL);
            assert(all_data->data[i].own_data.frames[0].name != NULL);
            assert(all_data->data[i].name != NULL);

            TextureAtlasItem* pivot = get_layer_with_name(&all_data->data[i], "pivot");
            TextureAtlasItem* anchor_1 = get_layer_with_name(&all_data->data[i], "anchor_1");
            TextureAtlasItem* pixel = get_pixel_layer(pixel_layer,
                                                      all_data->data[i].own_data.frames[0].name,
                                                      all_data->data[i].name,
                                                      all_data->data[i].children[0].name);

            if (pivot && anchor_1 && pixel) {

                CombinedItem * item = &(combined->items[combined->count]);
                assert(pivot->frame_count == anchor_1->frame_count);
                assert(anchor_1->frame_count == pixel->frame_count);

                item->file_name  = strdup(all_data->data[i].own_data.frames[0].name);
                item->group_name = strdup(all_data->data[i].name);
                item->layer_name = strdup(all_data->data[i].children[0].name);

                for (j = 0; j < pivot->frame_count; j++) {
                    item->frames[j].frame.x            = pixel->frames[j].frame.x;
                    item->frames[j].frame.y            = pixel->frames[j].frame.y;
                    item->frames[j].frame.w            = pixel->frames[j].frame.w;
                    item->frames[j].frame.h            = pixel->frames[j].frame.h;
                    item->frames[j].spriteSourceSize.x = pixel->frames[j].spriteSourceSize.x;
                    item->frames[j].spriteSourceSize.y = pixel->frames[j].spriteSourceSize.y;
                    item->frames[j].spriteSourceSize.w = pixel->frames[j].spriteSourceSize.w;
                    item->frames[j].spriteSourceSize.h = pixel->frames[j].spriteSourceSize.h;
                    item->frames[j].sourceSize.w       = pixel->frames[j].sourceSize.w;
                    item->frames[j].sourceSize.h       = pixel->frames[j].sourceSize.h;
                    item->frames[j].pivot.x            = pivot->frames[j].spriteSourceSize.x;
                    item->frames[j].pivot.y            = pivot->frames[j].spriteSourceSize.y;
                    item->frames[j].anchor_1.x         = anchor_1->frames[j].spriteSourceSize.x;
                    item->frames[j].anchor_1.y         = anchor_1->frames[j].spriteSourceSize.y;

                    fprintf(fptr, "    generated_frames[%s_%s_%s_%03d] = (FrameWithPivotAnchor){%d, %d, %d, %d,%d, %d, %d, %d, %d, %d, %d, %d, %d, %d};",
                            prefix,
                            trimwhitespace(item->file_name),
                            trimwhitespace(item->group_name),
                            j,
                            item->frames[j].frame.x,
                            item->frames[j].frame.y,
                            item->frames[j].frame.w,
                            item->frames[j].frame.h,
                            item->frames[j].spriteSourceSize.x,
                            item->frames[j].spriteSourceSize.y,
                            item->frames[j].spriteSourceSize.w,
                            item->frames[j].spriteSourceSize.h,
                            item->frames[j].sourceSize.w,
                            item->frames[j].sourceSize.h,
                            item->frames[j].pivot.x,
                            item->frames[j].pivot.y,
                            item->frames[j].anchor_1.x,
                            item->frames[j].anchor_1.y);
                    fprintf(fptr, "\n");
                }

                combined->count++;
            } else {
                //printf("skipping layered object that doesnt have pixel, anchor_1 & pivot data\n");
            }
        } else if (all_data->data[i].is_solo) {
            printf("SOLO %s %s \n", all_data->data[i].own_data.frames[0].name, all_data->data[i].name);
        } else {
            assert(0);
        }
    }

    fprintf(fptr, "};\n");
    fclose(fptr);
    printf("written %d frames.\n", counter);
}





typedef enum {
    False,
    Blocks,
    Bodies
} Usage;


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("This tool makes useful binary representation out of specific json representations of TextureAtlasses.\n");
        printf("It can either use ASEPrite generated json, that crafted in such a way you can use pivots and anchors.\n");
        printf("Alternatively you can use shoebox generated json for single framed blocks\n");
        printf("The all.json and pixel.json are generated with Aseprite CLI (use the create.sh in this folder for that).\n\n");
        printf("Usage 1: pivotplanner gen/all.json gen/pixel.json\n");
        printf("Usage 2: pivotplanner shoe.json\n");
        return 1;
    }
    printf("aoisdoaish doaish dohiasod \n");
    printf("%s\n", argv[1]);
    printf("str compare 'blocks' : %d\n",strcmp(argv[1],"blocks")==0);
    printf("str compare 'bodies'  : %d\n",strcmp(argv[1],"bodies")==0);

    Usage use = False;
    if (strcmp(argv[1],"blocks")==0) {
        use = Blocks;
        // expect a pixel and pivot
    } else if (strcmp(argv[1],"bodies")==0) {
        use = Bodies;
        // expect pixel, pivot and anchor
    } else {
        printf("CRITICAL, i need 'blocks' or 'bodies' as my first argument\n");
        return 1;
    }


    if (argc == 4) {
        char * all_path = "";
        char * pixel_path = "";
        pixel_path=argv[2];
        all_path=argv[3];

        const char* all_str = readFile(all_path);
        const char* pix_str = readFile(pixel_path);

        jsmn_parser parser;

        jsmn_init(&parser);
        int all = jsmn_parse(&parser, all_str, strlen(all_str), all_tokens, sizeof(all_tokens)/sizeof(all_tokens[0]));

        jsmn_init(&parser);
        int pixels = jsmn_parse(&parser, pix_str, strlen(pix_str), pixel_tokens, sizeof(pixel_tokens)/sizeof(pixel_tokens[0]));

        if (all < 0) {
            printf("failed to parse JSON, maybe enlarge the ALL buffer above? %d\n", all);
        }
        if (pixels < 0) {
            printf("failed to parse JSON, maybe enlarge the PIXEL buffer above? %d\n", pixels);
        }

        int i = 0;
        // first create layer object

        for (i = 0; i < all; i++) {
            if (all_tokens[i].type == JSMN_ARRAY) {
                if (jsoneq(all_str, &all_tokens[i-1], "layers") == 0) {
                    analyze_layers(all_tokens, i, all_str, &all_data, 0);
                }
            }
        }
        for (i = 0; i < all; i++) {
            if (all_tokens[i].type == JSMN_ARRAY) {
                if (jsoneq(all_str, &all_tokens[i-1], "frames") == 0) {
                    analyze_frames(all_tokens, i, all_str, &all_data, 0);
                }
            }
        }

        // now do the pixel layer stuff
        for (i = 0; i < pixels; i++) {
            if (pixel_tokens[i].type == JSMN_ARRAY) {
                if (jsoneq(pix_str, &pixel_tokens[i-1], "layers") == 0) {
                    analyze_layers(pixel_tokens, i, pix_str, &pixel_layer, 1);
                }
            }
        }
        for (i = 0; i < pixels; i++) {
            if (pixel_tokens[i].type == JSMN_ARRAY) {
                if (jsoneq(pix_str, &pixel_tokens[i-1], "frames") == 0) {
                    analyze_frames(pixel_tokens, i, pix_str, &pixel_layer, 1);
                }
            }
        }

        if (use == Bodies) {
            printf("I want to do the bodies man\n");
            combine_bodies(&pixel_layer, &all_data, &combined);
        } else if (use == Blocks) {
            printf("I want to do the blocks man\n");
            combine_blocks(&pixel_layer, &all_data, &combined);

        }
        //combine_all_and_pixel(&pixel_layer, &all_data, &combined, "BP");
    }













    //while (strcmp(argv[1],"block") != 0) {

    //}


    return 0;




}
