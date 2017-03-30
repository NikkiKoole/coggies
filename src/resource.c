#include <string.h>
#include "multi_platform.h"

#include "types.h"
#include "resource.h"
#include "memory.h"
#include "level.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wold-style-definition"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"
#pragma GCC diagnostic pop


internal b32 exists(const char *fname) {
    FILE *f;
    if ((f = fopen(fname, "r"))) {
        fclose(f);
        return true;
    }
    return false;
}

internal void make_font(BM_Font *font, const char *path) {
    UNUSED(font);
    //    printf("%s\n", path);
    if (exists(path)) {
        //ok the bmf file will be in binary, thats a lot easier actually for me.
        //http://www.angelcode.com/products/bmfont/doc/file_format.html
        FILE *f = fopen(path, "rb");
        u8 header[3];
        if (f == NULL) {
            printf("Couldnt open file %s \n", path);
        }

        fread(&header, sizeof(u8), 3, f);
        if (header[0] == 'B' && header[1] == 'M' && header[2] == 'F') {
            // now lets check the format version (should be '3')
            u8 format;
            fread(&format, sizeof(u8), 1, f);
            if (format != 3) {
                printf("wrong format, I only know how to parse version 3 of BMFont");
            }
            // the BMFont has 4 blocks of data, the first 3 I am not very interrested in, I just trash all the data inside.
            // the fourth one contains all the chars.

            u8 blocktype_id;
            u32 block_size;
            u8 trash;

            u16 lineHeight;

            fread(&blocktype_id, sizeof(u8), 1, f);
            fread(&block_size, sizeof(u32), 1, f);
            for (u32 i = 0; i < block_size; i++) {
                fread(&trash, sizeof(u8), 1, f);
            }

            fread(&blocktype_id, sizeof(u8), 1, f);
            fread(&block_size, sizeof(u32), 1, f);
            fread(&lineHeight, sizeof(u16), 1, f);
            for (u32 i = 2; i < block_size; i++) { //starts from 2 because the first 2 bytes was the lineHeight
                fread(&trash, sizeof(u8), 1, f);
            }

            fread(&blocktype_id, sizeof(u8), 1, f);
            fread(&block_size, sizeof(u32), 1, f);
            for (u32 i = 0; i < block_size; i++) {
                fread(&trash, sizeof(u8), 1, f);
            }

            fread(&blocktype_id, sizeof(u8), 1, f);
            fread(&block_size, sizeof(u32), 1, f);

            u32 num_chars = block_size / 20;
            for (u32 i = 0; i < num_chars; i++) {
                u32 id;
                u16 x, y, width, height;
                s16 xoffset, yoffset, xadvance;
                u8 page, channel;
                fread(&id, sizeof(u32), 1, f);
                fread(&x, sizeof(u16), 1, f);
                fread(&y, sizeof(u16), 1, f);
                fread(&width, sizeof(u16), 1, f);
                fread(&height, sizeof(u16), 1, f);
                fread(&xoffset, sizeof(s16), 1, f);
                fread(&yoffset, sizeof(s16), 1, f);
                fread(&xadvance, sizeof(s16), 1, f);
                fread(&page, sizeof(u8), 1, f);
                fread(&channel, sizeof(u8), 1, f);
                ASSERT(page == 0 && "Only fonts that fit on 1 texture page are allowed.");
                font->chars[id].id = id;
                font->chars[id].x = x;
                font->chars[id].y = y;
                font->chars[id].width = width;
                font->chars[id].height = height;
                font->chars[id].xoffset = xoffset;
                font->chars[id].yoffset = yoffset;
                font->chars[id].xadvance = xadvance;
                //printf("x:%d, y:%d, xoffset:%d, yoffset:%d, width:%d, height:%d\n", x,y,xoffset,yoffset,width,height);
            }
            font->line_height = lineHeight;
            //printf("%d chars found in fnt file %s.\n", num_chars, path);

        } else {
            printf("%s is not a valid binary BMFont file.\n", path);
        }
        fclose(f);
    } else {
        printf("couldnt find %s\n", path);
    }
}


internal char *resource(const char *path, char *buffer) {
    strcpy(buffer, getResourcePath());
    strcat(buffer, path);
    return buffer;
}


void resource_font(BM_Font *font, const char *path) {
    // lets try and parse a fnt file.
    char buffer[256];
    make_font(font, resource(path, buffer));
}


internal b32 load_TGA_file(const char *path, TGA_File *image) {
    FILE *f;
    u8 u8bad;
    u16 u16bad;
    u32 imageSize;
    u32 colorMode;
    u8 colorSwap;

    f = fopen(path, "rb");
    if (f == NULL) {
        printf("Couldnt open file %s \n", path);
        return false;
    }

    // dispose the first two bytes
    fread(&u8bad, sizeof(u8), 1, f);
    fread(&u8bad, sizeof(u8), 1, f);

    // read the image format
    fread(&image->format, sizeof(u8), 1, f);

    if (image->format != 2 && image->format != 3) {
        printf("TGA File is of wrong format!\n");
        //return false;
    }

    // dispose of 13 bytes
    fread(&u16bad, sizeof(u16), 1, f);
    fread(&u16bad, sizeof(u16), 1, f);
    fread(&u8bad, sizeof(u8), 1, f);
    fread(&u16bad, sizeof(u16), 1, f);
    fread(&u16bad, sizeof(u16), 1, f);

    // read the dimensions
    fread(&image->width, sizeof(u16), 1, f);
    fread(&image->height, sizeof(u16), 1, f);

    // read the bit depth
    fread(&image->bpp, sizeof(u8), 1, f);

    // dispose of 1 byte
    fread(&u8bad, sizeof(u8), 1, f);

    // color mode
    colorMode = image->bpp / 8;
    imageSize = image->width * image->height * colorMode;

    // allocate memory
    image->pixels = (u8 *)malloc(sizeof(u8) * imageSize);

    //read image
    fread(image->pixels, sizeof(u8), imageSize, f);

    //change BGR to RGB
    for (u32 i = 0; i < imageSize; i += colorMode) {
        colorSwap = image->pixels[i];
        image->pixels[i] = image->pixels[i + 2];
        image->pixels[i + 2] = colorSwap;
    }

    fclose(f);
    return true;
}

internal b32 is_power_of_2(u32 x) {
    return x && !(x & (x - 1));
}

internal void make_texture(Texture *t, const char *path, int type) {
    // second texture
    GLuint *tex = &t->id;
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
    // Set our texture parameters
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if (type == 0) { // TGA
        TGA_File image;
        load_TGA_file(path, &image);
        GLenum colorType = image.bpp == 24 ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, colorType, image.width, image.height, 0, colorType, GL_UNSIGNED_BYTE, image.pixels);
        t->width = image.width;
        t->height = image.height;
        ASSERT(t->width == t->height);
        ASSERT(is_power_of_2(t->width));
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        //glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess up our texture.
        //CHECK();
        free(image.pixels); // this pixels were malloced inside load_TGA_file

    } else if (type == 1) { //PNG
         int w;
         int h;
         int comp;
         unsigned char* image = stbi_load(path, &w, &h, &comp, STBI_rgb_alpha);
         GLenum colorType = GL_RGBA;
         glTexImage2D(GL_TEXTURE_2D, 0, colorType, w, h, 0, colorType, GL_UNSIGNED_BYTE, image);
         t->width = w;
         t->height = h;
         ASSERT(t->width == t->height);
         ASSERT(is_power_of_2(t->width));
         glBindTexture(GL_TEXTURE_2D, 0);
         stbi_image_free(image);
    }

}

void resource_png(Texture *t, const char *path) {
    UNUSED(t);UNUSED(path);
    char buffer[256];
    make_texture(t, resource(path, buffer), 1);
    //if (image == NULL) {
    //    printf("failed loading image: %s\n", path);
    //}
    //printf("%d, %d, %d\n", w, h, comp);
}

void resource_tga(Texture *t, const char *path) {
    char buffer[256];
    make_texture(t, resource(path, buffer), 0);
}

internal const char *read_file(const char *path, char* buffer ) {
    u32 length = 0;
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("Couldn't open file: %s\n", path);
    }
    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = (char*)malloc(length + 1); // this buffer is freed outside this function...
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



internal GLuint load_shader(const GLchar *path, GLenum type) {
    char *buffer = 0;
    const GLchar *code = read_file(path, buffer);
    free(buffer);
    GLint success;
    GLuint shader = glCreateShader(type);
    GLchar infoLog[512];

    if (shader == 0) {
        printf("Couldnt create shader of type: %d\n", type);
    }

    glShaderSource(shader, 1, &code, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("Shader (%s) compilation failed: %s\n", path, infoLog);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}
#define CHECK()                                                         \
    {                                                                   \
        int error = glGetError();                                       \
        if (error != 0) {                                               \
            printf("%d, function %s, file: %s, line:%d. \n", error, __FUNCTION__, __FILE__, __LINE__); \
            exit(0);                                                    \
        }                                                               \
    }



internal GLuint make_shader_program(const GLchar *vertexPath, const GLchar *fragmentPath) {
    GLuint vertex, fragment;
    GLint success;
    GLchar infoLog[512];
    vertex = load_shader(vertexPath, GL_VERTEX_SHADER);
    fragment = load_shader(fragmentPath, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);

    /* glBindAttribLocation(program, 0, "a_Position"); */
    /* glBindAttribLocation(program, 1, "a_TexCoord"); */
    /* glBindAttribLocation(program, 2, "a_Palette"); */

    CHECK();

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("Linking vertex(%s) & fragment(%s) failed: %s\n", vertexPath, fragmentPath, infoLog);
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}



void resource_shader(GLuint *shader, const char *vertPath, const char *fragPath) {
    char buffer[256];
    char buffer2[256];
    *shader = make_shader_program(resource(vertPath, buffer), resource(fragPath, buffer2));
}


internal void make_sprite_atlas(const char *path) {
    //printf("%s\n", path);
    if (exists(path)) {
        FILE *f = fopen(path, "rb");
        u8 header[3];
        if (f == NULL) {
            printf("Couldnt open file %s \n", path);
        }
        fread(&header, sizeof(u8), 3, f);
        if (header[0] == 'S' && header[1] == 'H' && header[2] == 'O') {
            u8 format;
            fread(&format, sizeof(u8), 1, f);
            if (format != 1) {
                printf("wrong format, I only know how to parse version 1 of binary shoebox");
            }
            char image_name[16];
            fread(&image_name, sizeof(u8) * 16, 1, f);
            u16 image_width;
            u16 image_height;
            fread(&image_width, sizeof(u16), 1, f);
            fread(&image_height, sizeof(u16), 1, f);
            u32 block_size;
            fread(&block_size, sizeof(u32), 1, f);

            for (u32 i = 0; i < block_size / 36; i++) {
                char name[16];
                u16 frameX, frameY, frameW, frameH;
                u16 sourceX, sourceY, sourceW, sourceH;
                u16 ssW, ssH;
                fread(&name, sizeof(u8) * 16, 1, f);
                fread(&frameX, sizeof(u16), 1, f);
                fread(&frameY, sizeof(u16), 1, f);
                fread(&frameW, sizeof(u16), 1, f);
                fread(&frameH, sizeof(u16), 1, f);
                fread(&sourceX, sizeof(u16), 1, f);
                fread(&sourceY, sizeof(u16), 1, f);
                fread(&sourceW, sizeof(u16), 1, f);
                fread(&sourceH, sizeof(u16), 1, f);
                fread(&ssW, sizeof(u16), 1, f);
                fread(&ssH, sizeof(u16), 1, f);
                //printf("%.*s\n", 16, name);
                //printf("ID:%d frame:{x:%d, y:%d, w:%d, h:%d} source:{x:%d, y:%d, w:%d, h:%d} ?:{w:%d, h:%d}\n", i, frameX, frameY, frameW, frameH, sourceX, sourceY, sourceW, sourceH, ssW, ssH);
            }
        }
    }
}

/* internal int starts_with(const char *pre, const char *str) */
/* { */
/*     size_t lenpre = strlen(pre), */
/*            lenstr = strlen(str); */
/*     return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0; */
/* } */

void resource_sprite_atlas(const char *path) {
    char buffer[256];
    make_sprite_atlas(resource(path, buffer));
}

#define BUF_SIZE 1024



void resource_level(PermanentState *permanent, LevelData *level, const char * path) {
    char buffer[256];
    make_level(permanent, level, resource(path, buffer));
}


void resource_ogg(Mix_Music **music, const char *path) {
    char buffer[256];
    *music = Mix_LoadMUS(resource(path, buffer));
    //SDL_MIX_ASSERT(*music);
}
void resource_wav(Mix_Chunk **chunk, const char *path) {
    char buffer[256];
    *chunk = Mix_LoadWAV(resource(path, buffer));
    //SDL_MIX_ASSERT(*chunk);
}
