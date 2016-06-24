#include "renderer.h"
#include "multi_platform.h"
#include "types.h"
#include "memory.h"
#include "random.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
  TODO
  I think a bit more descriptive rendering methods would be better in the long run:

  initArchitecture()
  initActors()

  drawArchitecture()
  drawActors()

  and eventually
  drawObjects()
  drawTexts()
  etc

  updating would be done with:
  updateArchitecture()
  updateActors()

  and etc
*/



RenderState _rstate;
RenderState *renderer = &_rstate;

GameState _gstate;
GameState *game = &_gstate;

typedef struct {
    float x;
    float y;
} V2;

typedef struct {
    V2 tl;
    V2 br;
} Rect2;

typedef struct {
    u8 format;
    u16 width;
    u16 height;
    u8 bpp;
    u8 *pixels;
} TGA_File;



#define CHECK()                                                                                        \
    {                                                                                                  \
        int error = glGetError();                                                                      \
        if (error != 0) {                                                                              \
            printf("%d, function %s, file: %s, line:%d. \n", error, __FUNCTION__, __FILE__, __LINE__); \
            exit(0);                                                                                   \
        }                                                                                              \
    }


internal Rect2 get_uvs(float size, int x, int y, int width, int height) {
    Rect2 result;
    result.tl.x = x / size;
    result.tl.y = (y + height) / size;
    result.br.x = (x + width) / size;
    result.br.y = y / size;
    return result;
}

internal Rect2 get_verts(float viewportWidth,
                        float viewportHeight,
                        float x,
                        float y,
                        float width,
                        float height,
                        float scaleX,
                        float scaleY,
                        float pivotX,
                        float pivotY) {
    Rect2 result;
    result.tl.x = x - ((pivotX * 2) * (width / viewportWidth) * scaleX);
    result.tl.y = y - ((2 - pivotY * 2) * (height / viewportHeight) * scaleY);
    result.br.x = x + ((2 - pivotX * 2) * (width / viewportWidth) * scaleX);
    result.br.y = y + ((pivotY * 2) * (height / viewportHeight) * scaleY);
    return result;
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

#ifdef GLES
internal void
makeBufferRPI(GLfloat vertices[], GLushort indices[], int size, GLuint *VBO, GLuint *EBO, GLenum usage) {
    glGenBuffers(1, VBO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * size * VALUES_PER_ELEM, vertices, usage);

    glGenBuffers(1, EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * size * 6, indices, GL_STATIC_DRAW);
}


internal void
bindBuffer(GLuint *VBO, GLuint *EBO, GLuint *program) {
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    //setup and enable attrib pointer
    GLuint a_Position = glGetAttribLocation(*program, "a_Position");
    glVertexAttribPointer(a_Position, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, (GLvoid *)0);
    glEnableVertexAttribArray(a_Position);
    GLuint a_TexCoord = glGetAttribLocation(*program, "a_TexCoord");
    glVertexAttribPointer(a_TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(a_TexCoord);
    GLuint a_Palette = glGetAttribLocation(*program, "a_Palette");
    glVertexAttribPointer(a_Palette, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, (GLvoid *)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(a_Palette);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
}
#endif

#ifdef GL3
internal void makeBuffer(GLfloat vertices[], GLushort indices[], int size, GLuint *VAO, GLuint *VBO, GLuint *EBO, GLenum usage) {
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glGenBuffers(1, EBO);

    glBindVertexArray(*VAO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * size * VALUES_PER_ELEM, vertices, usage);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * size * 6, indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);
    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);



    glBindVertexArray(0); // Unbind VAO
    CHECK();
}
#endif



internal b32 exists(const char *fname) {
    FILE *f;
    if ((f = fopen(fname, "r"))) {
        fclose(f);
        return true;
    }
    return false;
}

internal const char *read_file(const char *path) {
    char *buffer = 0;
    u32 length = 0;
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

internal GLuint load_shader(const GLchar *path, GLenum type) {
    const GLchar *code = read_file(path);
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

void make_sprite_atlas(const char *path) {
    printf("%s\n", path);
    if (exists(path)){
        FILE *f = fopen(path, "rb");
        u8 header[3];
        if  (f == NULL) {
            printf("Couldnt open file %s \n", path);
        }
        fread(&header, sizeof(u8), 3, f);
        if (header[0]=='S' && header[1]=='H' && header[2]=='O') {
            u8 format;
            fread(&format, sizeof(u8), 1, f);
            if (format != 1) {
                printf("wrong format, I only know how to parse version 1 of binary shoebox");
            }
            char image_name[16];
            fread(&image_name, sizeof(u8)*16, 1, f);
            //printf("%.*s\n", 16, image_name);
            u16 image_width;
            u16 image_height;
            fread(&image_width, sizeof(u16), 1, f);
            fread(&image_height, sizeof(u16), 1, f);
            u32 block_size;
            fread(&block_size, sizeof(u32), 1, f);
            //printf("image width: %d, image height: %d \n", image_width, image_height);
            //printf("amount of frames: %d \n", block_size/36);
            for (u32 i = 0; i < block_size/36; i++) {
                char name[16];
                u16 frameX, frameY, frameW, frameH;
                u16 sourceX, sourceY, sourceW, sourceH;
                u16 ssW, ssH;
                fread(&name, sizeof(u8)*16, 1, f);
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



void make_font(BM_Font *font, const char *path) {
    UNUSED(font);
    printf("%s\n", path);
    if (exists(path)) {
        //ok the bmf file will be in binary, thats a lot easier actually for me.
        //http://www.angelcode.com/products/bmfont/doc/file_format.html
        FILE *f = fopen(path, "rb");
        u8 header[3];
        if (f == NULL) {
            printf("Couldnt open file %s \n", path);
        }

        fread(&header, sizeof(u8), 3, f);
        if (header[0]=='B' && header[1]=='M' && header[2]=='F') {
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

            u32 num_chars = block_size/20;
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
            printf("%d chars found in fnt file %s.\n",num_chars, path);

        } else {

            printf("%s is not a valid binary BMFont file.\n", path);
        }
        fclose(f);
    } else {
        printf("couldnt find %s\n",path);
    }
}


internal b32 is_power_of_2(u32 x) {
   return x && !(x & (x - 1));
 }

void make_texture(Texture *t, const char *path) {
    // second texture
    GLuint *tex = &t->id;
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
    // Set our texture parameters
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    TGA_File image;
    load_TGA_file(path, &image);
    GLenum colorType = image.bpp == 24 ? GL_RGB : GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, colorType, image.width, image.height, 0, colorType, GL_UNSIGNED_BYTE, image.pixels);
    t->width = image.width;
    t->height = image.height;
    ASSERT(t->width == t->height);
    ASSERT(is_power_of_2(t->width));
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess up our texture.
    CHECK();
}




GLuint make_shader_program(const GLchar *vertexPath, const GLchar *fragmentPath) {
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


internal const char *gl_error_string(GLenum error) {
    switch (error) {
        case GL_INVALID_OPERATION:
            return "INVALID_OPERATION";
            break;
        case GL_INVALID_ENUM:
            return "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            return "INVALID_VALUE";
            break;
        case GL_OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GL_NO_ERROR:
            return "NO_ERROR";
            break;
    }
    return "UNKNOWN_ERROR";
}

void initialize_GL(void) {
    GLenum error = GL_NO_ERROR;
    const char *str = "Error initializing OpenGL";
#ifdef GL3
    glewExperimental = GL_TRUE;
    ASSERT(glewInit() == GLEW_OK);
    glGetError(); // pop the error glewInit generates (http://stackoverflow.com/questions/10857335/opengl-glgeterror-returns-invalid-enum-after-call-to-glewinit)
#endif
    error = glGetError();
    glClearColor(0.5f, 0.5f, 1.0f, 1.0f);

    if (error != GL_NO_ERROR) {
        printf("%s (ClearColor) (%s)\n", str, gl_error_string(error));
        exit(0);
    }
}


internal int cmpfunc(const void * a, const void * b)
{
    // TODO this one is correct I think, but I dont understand it anymore ;)
    const Wall *a2 = (const Wall *) a;
    const Wall *b2 = (const Wall *) b;
    return ( ( b2->y) - ( a2->y));
}

void prepare_renderer(void) {

    ASSERT(renderer->walls.count * VALUES_PER_ELEM < 2048 * 24);

    glViewport(0, 0, renderer->view.width, renderer->view.height);

    qsort(game->walls, renderer->walls.count, sizeof(game->walls[0]), cmpfunc);

    int real_world_height = game->world_height * game->block_height;
    int real_world_depth = game->world_depth * (game->block_depth);
    int screenWidth = renderer->view.width;
    int screenHeight = renderer->view.height;

    int offset_x_blocks = game->x_view_offset;
    int offset_y_blocks = game->y_view_offset;
    int texture_size = renderer->assets.sprite.width;


    for (u32 i = 0; i < renderer->walls.count * VALUES_PER_ELEM; i += VALUES_PER_ELEM) {

        int prepare_index = i / VALUES_PER_ELEM;
        Wall data = game->walls[prepare_index];
        float scale = 1;
        float wallX = data.frame * 24;

        float tempX = data.x;// * block_width;
        float tempY = real_world_height - (data.y) + (data.z);
        tempX  += offset_x_blocks;
        tempY += offset_y_blocks-(96);   // ok you need to do -96 because thast the height, and 12 less because the pivot is 12 px of the bottom

        float x = (tempX /screenWidth)*2 - 1.0;
        float y = (tempY/screenHeight)*2 - 1.0;

        //float wallDepth = 0.25f ;
        float wallDepth = ((float)data.z/(float)real_world_depth);
        //printf("%f %d %d\n",wallDepth, data.z, real_world_depth);
        float wallY = 12.0f;
        float wallHeight = 108.0f;
        float paletteIndex = (data.y / 350.0f);
        Rect2 uvs = get_uvs(texture_size, wallX, wallY, 24, wallHeight);
        //96.0f/108.0f
        Rect2 verts = get_verts(renderer->view.width, renderer->view.height, x, y, 24.0f, wallHeight, scale, scale, 0.5, 1.0f);

        // bottomright
        renderer->walls.vertices[i + 0] = verts.br.x;
        renderer->walls.vertices[i + 1] = verts.br.y;
        renderer->walls.vertices[i + 2] = wallDepth;
        renderer->walls.vertices[i + 3] = uvs.br.x;
        renderer->walls.vertices[i + 4] = uvs.br.y;
        renderer->walls.vertices[i + 5] = paletteIndex;
        //topright
        renderer->walls.vertices[i + 6] = verts.br.x;
        renderer->walls.vertices[i + 7] = verts.tl.y;
        renderer->walls.vertices[i + 8] = wallDepth;
        renderer->walls.vertices[i + 9] = uvs.br.x;
        renderer->walls.vertices[i + 10] = uvs.tl.y;
        renderer->walls.vertices[i + 11] = paletteIndex;
        // top left
        renderer->walls.vertices[i + 12] = verts.tl.x;
        renderer->walls.vertices[i + 13] = verts.tl.y;
        renderer->walls.vertices[i + 14] = wallDepth;
        renderer->walls.vertices[i + 15] = uvs.tl.x;
        renderer->walls.vertices[i + 16] = uvs.tl.y;
        renderer->walls.vertices[i + 17] = paletteIndex;
        // bottomleft
        renderer->walls.vertices[i + 18] = verts.tl.x;
        renderer->walls.vertices[i + 19] = verts.br.y;
        renderer->walls.vertices[i + 20] = wallDepth;
        renderer->walls.vertices[i + 21] = uvs.tl.x;
        renderer->walls.vertices[i + 22] = uvs.br.y;
        renderer->walls.vertices[i + 23] = paletteIndex;
    }




    ASSERT(renderer->walls.count * 6 < 2048 * 6);
    for (u32 i = 0; i < renderer->walls.count * 6; i += 6) {
        int j = (i / 6) * 4;
        renderer->walls.indices[i + 0] = j + 0;
        renderer->walls.indices[i + 1] = j + 1;
        renderer->walls.indices[i + 2] = j + 2;
        renderer->walls.indices[i + 3] = j + 0;
        renderer->walls.indices[i + 4] = j + 2;
        renderer->walls.indices[i + 5] = j + 3;
    }

#ifdef GLES
    makeBufferRPI(renderer->walls.vertices, renderer->walls.indices, renderer->walls.count, &renderer->walls.VBO, &renderer->walls.EBO, GL_STATIC_DRAW);
#endif
#ifdef GL3
    makeBuffer(renderer->walls.vertices, renderer->walls.indices, renderer->walls.count, &renderer->walls.VAO, &renderer->walls.VBO, &renderer->walls.EBO, GL_STATIC_DRAW);
#endif


    // TODO
    // mayeb in the prepare step the buffers will always all be initialized
    // that way I think its easier to during runtime, create and delete actors

    for (int actor_batch_index = 0; actor_batch_index < 8; actor_batch_index++) {
        DrawBuffer *batch = &renderer->actors[actor_batch_index];
        // actors
        for (u32 i = 0; i <= 2048 * VALUES_PER_ELEM; i++) {
            //batch->vertices[i] = 0;
        }

        for (u32 i = 0; i < 2048 * 6; i += 6) {
            int j = (i / 6) * 4;
            batch->indices[i + 0] = j + 0;
            batch->indices[i + 1] = j + 1;
            batch->indices[i + 2] = j + 2;
            batch->indices[i + 3] = j + 0;
            batch->indices[i + 4] = j + 2;
            batch->indices[i + 5] = j + 3;
        }
#ifdef GLES
        makeBufferRPI(batch->vertices, batch->indices, 2048, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW);
#endif
#ifdef GL3
        makeBuffer(batch->vertices, batch->indices, 2048, &batch->VAO, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW);
#endif

    }




    // prepage buffers for FONT drawing
     // TODO
    // mayeb in the prepare step the buffers will always all be initialized
    // that way I think its easier to during runtime, create and delete actors

    {
        for (int glyph_batch_index = 0; glyph_batch_index < 1; glyph_batch_index++) {
            DrawBuffer *batch = &renderer->glyphs[glyph_batch_index];
            // actors
            for (u32 i = 0; i <= 2048 * VALUES_PER_ELEM; i++) {
                //batch->vertices[i] = 0;
            }

            for (u32 i = 0; i < 2048 * 6; i += 6) {
                int j = (i / 6) * 4;
                batch->indices[i + 0] = j + 0;
                batch->indices[i + 1] = j + 1;
                batch->indices[i + 2] = j + 2;
                batch->indices[i + 3] = j + 0;
                batch->indices[i + 4] = j + 2;
                batch->indices[i + 5] = j + 3;
            }
#ifdef GLES
            makeBufferRPI(batch->vertices, batch->indices, 2048, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW);
#endif
#ifdef GL3
            makeBuffer(batch->vertices, batch->indices, 2048, &batch->VAO, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW);
#endif
        }
    }
}



void render(SDL_Window *window) {
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClearDepthf(1.0f);
    glEnable(GL_BLEND);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    CHECK();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    // TODO
    // I should separate out the architecture drawing and the actor drawing
    // That will probably cost setting the textures and shader twice but its much cleaner
    // I also get rid of the palletized architecture drawing, cause I dont need that
    // I could also add some new shader logic to the architecture (to add shadow (or well dark light tiles))


    glUseProgram(renderer->assets.shader1);
    CHECK();

    // Bind Textures using texture units
    glActiveTexture(GL_TEXTURE0);
    CHECK();

    glBindTexture(GL_TEXTURE_2D, renderer->assets.sprite.id);
    CHECK();


    glUniform1i(glGetUniformLocation(renderer->assets.shader1, "ourTexture1"), 0);
    CHECK();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->assets.palette.id);
    glUniform1i(glGetUniformLocation(renderer->assets.shader1, "ourTexture2"), 1);

    CHECK();

    for (int actor_batch_index = 0; actor_batch_index < renderer->used_actor_batches; actor_batch_index++) {
        DrawBuffer *batch = &renderer->actors[actor_batch_index];
        int count = batch->count;//game->actor_count;

        // actors
#ifdef GL3
        glBindVertexArray(batch->VAO);
#endif
        u32 screenWidth = renderer->view.width;
        u32 screenHeight = renderer->view.height;


        int texture_size = renderer->assets.sprite.width;
        int real_world_height = game->world_height * game->block_height;
        int real_world_depth = game->world_depth * game->block_depth;

        for (int i = 0; i < count * VALUES_PER_ELEM; i += VALUES_PER_ELEM) {
            int prepare_index = i / VALUES_PER_ELEM;
            prepare_index += (actor_batch_index * 2048);
            Actor data = game->actors[prepare_index];
            r32 scale = 1;
            r32 guyFrame = (4*24)+ data.frame * 24.0f;

            // TODO this -0.2f is to ghet actors drawn on top of walls/floors that are of the same depth, understand this number better
            //printf ("%f \n", (float)12 / (float)real_world_depth);

            float offset_toget_actor_ontop_of_floor = (float)12 / (float)real_world_depth;
            float guyDepth = ((float)data.z/(float)real_world_depth) - offset_toget_actor_ontop_of_floor;
            float tempX = data.x;
            float tempY = real_world_height - (data.y) + (data.z);
            tempX += game->x_view_offset;
            tempY += game->y_view_offset;

            // TODO this value (as with this value with the walls) has todo with the pivotY point in get_verts
            // curreently the result is correct but the code between walls and actors is too different.
            tempY -= (96);

            float x = ((float)tempX /screenWidth)*2 - 1.0;
            float y = ((float)tempY /screenHeight)*2 - 1.0;

            float paletteIndex = data.palette_index; //rand_float();

            Rect2 uvs = get_uvs(texture_size, guyFrame, 24.0f, 24.0f, 96.0f);
            //84.0f/96.0f
            Rect2 verts = get_verts(renderer->view.width, renderer->view.height, x, y, 24.0f, 96.0f, scale, scale, 0.5, 1.0f);


            // bottomright
            batch->vertices[i + 0] = verts.br.x;
            batch->vertices[i + 1] = verts.br.y;
            batch->vertices[i + 2] = guyDepth;
            batch->vertices[i + 3] = uvs.br.x;
            batch->vertices[i + 4] = uvs.br.y;
            batch->vertices[i + 5] = paletteIndex;
            //topright
            batch->vertices[i + 6] = verts.br.x;
            batch->vertices[i + 7] = verts.tl.y;
            batch->vertices[i + 8] = guyDepth;
            batch->vertices[i + 9] = uvs.br.x;
            batch->vertices[i + 10] = uvs.tl.y;
            batch->vertices[i + 11] = paletteIndex;
            // top left
            batch->vertices[i + 12] = verts.tl.x;
            batch->vertices[i + 13] = verts.tl.y;
            batch->vertices[i + 14] = guyDepth;
            batch->vertices[i + 15] = uvs.tl.x;
            batch->vertices[i + 16] = uvs.tl.y;
            batch->vertices[i + 17] = paletteIndex;
            // bottomleft
            batch->vertices[i + 18] = verts.tl.x;
            batch->vertices[i + 19] = verts.br.y;
            batch->vertices[i + 20] = guyDepth;
            batch->vertices[i + 21] = uvs.tl.x;
            batch->vertices[i + 22] = uvs.br.y;
            batch->vertices[i + 23] = paletteIndex;
        }

#ifdef GLES
        bindBuffer(&batch->VBO, &batch->EBO, &renderer->shader1);
        check();
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * VALUES_PER_ELEM * 4, batch->vertices);
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        glDisableVertexAttribArray(0);
#endif
#ifdef GL3
        glBindBuffer(GL_ARRAY_BUFFER, batch->VBO);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(batch->vertices), batch->vertices, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * VALUES_PER_ELEM * 4, batch->vertices);
        glDrawElements(GL_TRIANGLES, batch->count * 6 , GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);
#endif
        CHECK();


    }




//Draw walls
#ifdef GLES
    bindBuffer(&renderer->walls.VBO, &renderer->walls.EBO, &renderer->shader1);
    glDrawElements(GL_TRIANGLES, renderer->walls.count * 6, GL_UNSIGNED_SHORT, 0);
    glDisableVertexAttribArray(0);
#endif

#ifdef GL3
    glBindVertexArray(renderer->walls.VAO);
    glDrawElements(GL_TRIANGLES, renderer->walls.count * 6, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
#endif


    // Draw FONTS
    {
            // Bind Textures using texture units
        glActiveTexture(GL_TEXTURE0);
        CHECK();

        glBindTexture(GL_TEXTURE_2D, renderer->assets.menlo.id);
        CHECK();


        glUniform1i(glGetUniformLocation(renderer->assets.shader1, "ourTexture1"), 0);
        CHECK();

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, renderer->assets.palette.id);
        glUniform1i(glGetUniformLocation(renderer->assets.shader1, "ourTexture2"), 1);

        CHECK();

        int texture_size = renderer->assets.menlo.width;

        for (int glyph_batch_index = 0; glyph_batch_index < renderer->used_glyph_batches; glyph_batch_index++) {

            DrawBuffer *batch = &renderer->glyphs[glyph_batch_index];
            int count = batch->count;
#ifdef GL3
            glBindVertexArray(batch->VAO);
#endif

            for (int i = 0; i < count * VALUES_PER_ELEM; i += VALUES_PER_ELEM) {
                int prepare_index = i / VALUES_PER_ELEM;
                prepare_index += (glyph_batch_index * 2048);
                Glyph data = game->glyphs[prepare_index];
                r32 scale = 1;
                float guyDepth = -1.0f;
                float x = -1.0f + (((float) (data.x) / (float)renderer->view.width) * 2.0f);
                float y = -1.0f + (((float) (renderer->view.height-data.y) / (float)renderer->view.height) * 2.0f);
                float paletteIndex = 0.4f;//rand_float();

                Rect2 uvs = get_uvs(texture_size,  (float)data.sx,  (float)data.sy,  (float)data.w,  (float)data.h);
                Rect2 verts = get_verts(renderer->view.width, renderer->view.height, x, y,  (float)data.w,  (float)data.h, scale, scale, 0.0, 0.0);
                /* // bottomright */
                batch->vertices[i + 0] = verts.br.x;
                batch->vertices[i + 1] = verts.br.y;
                batch->vertices[i + 2] = guyDepth;
                batch->vertices[i + 3] = uvs.br.x;
                batch->vertices[i + 4] = uvs.br.y;
                batch->vertices[i + 5] = paletteIndex;
                //topright
                batch->vertices[i + 6] = verts.br.x;
                batch->vertices[i + 7] = verts.tl.y;
                batch->vertices[i + 8] = guyDepth;
                batch->vertices[i + 9] = uvs.br.x;
                batch->vertices[i + 10] = uvs.tl.y;
                batch->vertices[i + 11] = paletteIndex;
                // top left
                batch->vertices[i + 12] = verts.tl.x;
                batch->vertices[i + 13] = verts.tl.y;
                batch->vertices[i + 14] = guyDepth;
                batch->vertices[i + 15] = uvs.tl.x;
                batch->vertices[i + 16] = uvs.tl.y;
                batch->vertices[i + 17] = paletteIndex;
                // bottomleft
                batch->vertices[i + 18] = verts.tl.x;
                batch->vertices[i + 19] = verts.br.y;
                batch->vertices[i + 20] = guyDepth;
                batch->vertices[i + 21] = uvs.tl.x;
                batch->vertices[i + 22] = uvs.br.y;
                batch->vertices[i + 23] = paletteIndex;

            }
#ifdef GLES
            bindBuffer(&batch->VBO, &batch->EBO, &renderer->shader1);
            check();
            glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * VALUES_PER_ELEM * 4, batch->vertices);
            glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
            glDisableVertexAttribArray(0);
#endif
#ifdef GL3
            glBindBuffer(GL_ARRAY_BUFFER, batch->VBO);
            //glBufferData(GL_ARRAY_BUFFER, sizeof(batch->vertices), batch->vertices, GL_DYNAMIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * VALUES_PER_ELEM * 4, batch->vertices);
            glDrawElements(GL_TRIANGLES, batch->count * 6 , GL_UNSIGNED_SHORT, 0);
            glBindVertexArray(0);
#endif
        CHECK();

        }


    }



    CHECK();
    SDL_GL_SwapWindow(window);
}
