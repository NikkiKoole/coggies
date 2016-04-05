#include "renderer.h"
#include "multi_platform.h"
#include "types.h"
#include "memory.h"
#include "random.h"

RenderState _rstate;
RenderState *RState = &_rstate;

GameState _gstate;
GameState *GState = &_gstate;

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


void make_texture(GLuint *tex, const char *path) {
    // second texture
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
    glBindAttribLocation(program, 0, "a_Position");
    glBindAttribLocation(program, 1, "a_TexCoord");
    glBindAttribLocation(program, 2, "a_Palette");

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


void prepare_renderer(void) {
    ASSERT(RState->walls.count * VALUES_PER_ELEM < 2048 * 24);

    glViewport(0, 0, RState->view.width, RState->view.height);
    for (u32 i = 0; i < RState->walls.count * VALUES_PER_ELEM; i += VALUES_PER_ELEM) {
        int prepare_index = i / VALUES_PER_ELEM;
        Wall data = GState->walls[prepare_index];
        float scale = 1;
        float wallX = 0.0f;
        float x = -1.0f + ((data.x * 24) / (float)RState->view.width) * 2;  // -1.0 -> 1.0
        float y = -1.0f + ((data.y * 96) / (float)RState->view.height) * 2; //  -1.0 -> 1.0
        float wallDepth = rand_float() - 0.5;
        float wallY = 12.0f;
        float wallHeight = 108.0f;
        float paletteIndex = data.x / 100.0f;
        Rect2 uvs = get_uvs(256, wallX, wallY, 24, wallHeight);
        Rect2 verts = get_verts(RState->view.width, RState->view.height, x, y, 24.0f, wallHeight, scale, scale, 0.5, 1);

        // bottomright
        RState->walls.vertices[i + 0] = verts.br.x;
        RState->walls.vertices[i + 1] = verts.br.y;
        RState->walls.vertices[i + 2] = wallDepth;
        RState->walls.vertices[i + 3] = uvs.br.x;
        RState->walls.vertices[i + 4] = uvs.br.y;
        RState->walls.vertices[i + 5] = paletteIndex;
        //topright
        RState->walls.vertices[i + 6] = verts.br.x;
        RState->walls.vertices[i + 7] = verts.tl.y;
        RState->walls.vertices[i + 8] = wallDepth;
        RState->walls.vertices[i + 9] = uvs.br.x;
        RState->walls.vertices[i + 10] = uvs.tl.y;
        RState->walls.vertices[i + 11] = paletteIndex;
        // top left
        RState->walls.vertices[i + 12] = verts.tl.x;
        RState->walls.vertices[i + 13] = verts.tl.y;
        RState->walls.vertices[i + 14] = wallDepth;
        RState->walls.vertices[i + 15] = uvs.tl.x;
        RState->walls.vertices[i + 16] = uvs.tl.y;
        RState->walls.vertices[i + 17] = paletteIndex;
        // bottomleft
        RState->walls.vertices[i + 18] = verts.tl.x;
        RState->walls.vertices[i + 19] = verts.br.y;
        RState->walls.vertices[i + 20] = wallDepth;
        RState->walls.vertices[i + 21] = uvs.tl.x;
        RState->walls.vertices[i + 22] = uvs.br.y;
        RState->walls.vertices[i + 23] = paletteIndex;
    }

    ASSERT(RState->walls.count * 6 < 2048 * 6);
    for (u32 i = 0; i < RState->walls.count * 6; i += 6) {
        int j = (i / 6) * 4;
        RState->walls.indices[i + 0] = j + 0;
        RState->walls.indices[i + 1] = j + 1;
        RState->walls.indices[i + 2] = j + 2;
        RState->walls.indices[i + 3] = j + 0;
        RState->walls.indices[i + 4] = j + 2;
        RState->walls.indices[i + 5] = j + 3;
    }

#ifdef GLES
    makeBufferRPI(RState->walls.vertices, RState->walls.indices, RState->walls.count, &RState->walls.VBO, &RState->walls.EBO, GL_STATIC_DRAW);
#endif
#ifdef GL3
    makeBuffer(RState->walls.vertices, RState->walls.indices, RState->walls.count, &RState->walls.VAO, &RState->walls.VBO, &RState->walls.EBO, GL_STATIC_DRAW);
#endif


    // actors
    for (u32 i = 0; i <= RState->actors.count * VALUES_PER_ELEM; i++) {
        RState->actors.vertices[i] = 0;
    }

    for (u32 i = 0; i < RState->actors.count * 6; i += 6) {
        int j = (i / 6) * 4;
        RState->actors.indices[i + 0] = j + 0;
        RState->actors.indices[i + 1] = j + 1;
        RState->actors.indices[i + 2] = j + 2;
        RState->actors.indices[i + 3] = j + 0;
        RState->actors.indices[i + 4] = j + 2;
        RState->actors.indices[i + 5] = j + 3;
    }
#ifdef GLES
    makeBufferRPI(RState->actors.vertices, RState->actors.indices, RState->actors.count, &RState->actors.VBO, &RState->actors.EBO, GL_DYNAMIC_DRAW);
#endif
#ifdef GL3
    makeBuffer(RState->actors.vertices, RState->actors.indices, RState->actors.count, &RState->actors.VAO, &RState->actors.VBO, &RState->actors.EBO, GL_DYNAMIC_DRAW);
#endif
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

    glUseProgram(RState->assets.shader1);
    CHECK();

    // Bind Textures using texture units
    glActiveTexture(GL_TEXTURE0);
    CHECK();

    glBindTexture(GL_TEXTURE_2D, RState->assets.tex1);
    CHECK();


    glUniform1i(glGetUniformLocation(RState->assets.shader1, "ourTexture1"), 0);
    CHECK();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, RState->assets.tex2);
    glUniform1i(glGetUniformLocation(RState->assets.shader1, "ourTexture2"), 1);

    CHECK();



    int count = RState->actors.count;
// actors
#ifdef GL3
    glBindVertexArray(RState->actors.VAO);
#endif
    for (int i = 0; i < count * VALUES_PER_ELEM; i += VALUES_PER_ELEM) {
        int prepare_index = i / VALUES_PER_ELEM;
        Actor data = GState->actors[prepare_index];
        r32 scale = 1; //(float)randInt(3);
        r32 guyFrame = 48.0 + data.frame * 24.0f;
        float guyDepth = data.z; //0.0f;//-0.8 + randFloat()*0.8f; // walls are at -0.5
        float x = -1.0f + (((float)data.x / (float)RState->view.width) * 2.0f);
        float y = -1.0f + (((float)data.y / (float)RState->view.height) * 2.0f);
        float paletteIndex = data.palette_index; //rand_float();

        Rect2 uvs = get_uvs(256.0f, guyFrame, 24.0f, 24.0f, 96.0f);
        Rect2 verts = get_verts(RState->view.width, RState->view.height, x, y, 24.0f, 96.0f, scale, scale, 0.5, 0.5);

        // bottomright
        RState->actors.vertices[i + 0] = verts.br.x;
        RState->actors.vertices[i + 1] = verts.br.y;
        RState->actors.vertices[i + 2] = guyDepth;
        RState->actors.vertices[i + 3] = uvs.br.x;
        RState->actors.vertices[i + 4] = uvs.br.y;
        RState->actors.vertices[i + 5] = paletteIndex;
        //topright
        RState->actors.vertices[i + 6] = verts.br.x;
        RState->actors.vertices[i + 7] = verts.tl.y;
        RState->actors.vertices[i + 8] = guyDepth;
        RState->actors.vertices[i + 9] = uvs.br.x;
        RState->actors.vertices[i + 10] = uvs.tl.y;
        RState->actors.vertices[i + 11] = paletteIndex;
        // top left
        RState->actors.vertices[i + 12] = verts.tl.x;
        RState->actors.vertices[i + 13] = verts.tl.y;
        RState->actors.vertices[i + 14] = guyDepth;
        RState->actors.vertices[i + 15] = uvs.tl.x;
        RState->actors.vertices[i + 16] = uvs.tl.y;
        RState->actors.vertices[i + 17] = paletteIndex;
        // bottomleft
        RState->actors.vertices[i + 18] = verts.tl.x;
        RState->actors.vertices[i + 19] = verts.br.y;
        RState->actors.vertices[i + 20] = guyDepth;
        RState->actors.vertices[i + 21] = uvs.tl.x;
        RState->actors.vertices[i + 22] = uvs.br.y;
        RState->actors.vertices[i + 23] = paletteIndex;
    }
#ifdef GLES
    bindBuffer(&RState->actors.VBO, &RState->actors.EBO, &RState->shader1);
    check();
    glBufferSubData(GL_ARRAY_BUFFER, 0, RState->actors.count * VALUES_PER_ELEM * 4, RState->actors.vertices);
    glDrawElements(GL_TRIANGLES, RState->actors.count * 6, GL_UNSIGNED_SHORT, 0);
    glDisableVertexAttribArray(0);
#endif
#ifdef GL3
    glBindBuffer(GL_ARRAY_BUFFER, RState->actors.VBO);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(RState->actors.vertices), RState->actors.vertices, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, RState->actors.count * VALUES_PER_ELEM * 4, RState->actors.vertices);
    glDrawElements(GL_TRIANGLES, RState->actors.count * 6, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
#endif
    CHECK();

//Draw walls
#ifdef GLES
    bindBuffer(&RState->walls.VBO, &RState->walls.EBO, &RState->shader1);
    glDrawElements(GL_TRIANGLES, RState->walls.count * 6, GL_UNSIGNED_SHORT, 0);
    glDisableVertexAttribArray(0);
#endif

#ifdef GL3
    glBindVertexArray(RState->walls.VAO);
    glDrawElements(GL_TRIANGLES, RState->walls.count * 6, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
#endif


    CHECK();
    SDL_GL_SwapWindow(window);
}
