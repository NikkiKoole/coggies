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



#ifdef GLES
internal void makeBufferRPI(GLfloat vertices[], GLushort indices[], int size, GLuint *VBO, GLuint *EBO, GLenum usage) {
    glGenBuffers(1, VBO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * size * VALUES_PER_ELEM, vertices, usage);

    glGenBuffers(1, EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * size * 6, indices, GL_STATIC_DRAW);
}


internal void bindBuffer(GLuint *VBO, GLuint *EBO, GLuint *program) {
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


/* internal b32 exists(const char *fname) { */
/*     FILE *f; */
/*     if ((f = fopen(fname, "r"))) { */
/*         fclose(f); */
/*         return true; */
/*     } */
/*     return false; */
/* } */



















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


internal int cmpfunc(const void *a, const void *b) {
    // TODO  I dont understand it anymore ;)

    const Wall *a2 = (const Wall *)a;
    const Wall *b2 = (const Wall *)b;
    return ((a2->y) - (b2->y));
}

void prepare_renderer(void) {
    //ASSERT(renderer->walls.count * VALUES_PER_ELEM < 2048 * 24);
    glViewport(0, 0, renderer->view.width, renderer->view.height);

    int real_world_height = game->dims.z_level * game->block_size.z_level;
    int real_world_depth = game->dims.y * (game->block_size.y);
    int screenWidth = renderer->view.width;
    int screenHeight = renderer->view.height;

    int offset_x_blocks = game->x_view_offset;
    int offset_y_blocks = game->y_view_offset;
    int texture_size = renderer->assets.sprite.width;



    for (int wall_batch_index = 0; wall_batch_index < 8; wall_batch_index++) {
        DrawBuffer *batch = &renderer->walls[wall_batch_index];
        u32 count = batch->count; //game->actor_count;

        for (u32 i = 0; i < count * VALUES_PER_ELEM; i += VALUES_PER_ELEM) {
            int prepare_index = i / VALUES_PER_ELEM;
            prepare_index += (wall_batch_index * 2048);
            Wall data = game->walls[prepare_index];
            float scale = 1;
            float wallX = data.frame * 24;

            float tempX = data.x;
            float tempY = (data.z) - (data.y) / 2;
            tempX += offset_x_blocks;
            tempY += offset_y_blocks;

            float x = (tempX / screenWidth) * 2 - 1.0;
            float y = (tempY / screenHeight) * 2 - 1.0;

            float wallDepth = -1 * ((float)data.y / (float)real_world_depth);
            float wallY = 12.0f;
            float wallHeight = 108.0f;


            // TODO THIS IS SOME OPTIMIZATION, TEST IT ON THE RPI
            // Eventually I want to have some meta lookup for texture atlasses, then everything will be like this.
            /* if (data.frame > 6 && data.frame < 10) { // wallpart is a floor, much smaller then a wall */
            /*     wallY = 106.0f; */
            /*     wallHeight = 14.0f; */
            /* } */


            float paletteIndex = 1.0f/16 *1;//rand_float(); //(data.y / 350.0f);
            Rect2 uvs = get_uvs(texture_size, wallX, wallY, 24, wallHeight);
            Rect2 verts = get_verts(renderer->view.width, renderer->view.height, x, y, 24.0f, wallHeight, scale, scale, 0.5, 1.0f);

            // bottomright
            batch->vertices[i + 0] = verts.br.x;
            batch->vertices[i + 1] = verts.br.y;
            batch->vertices[i + 2] = wallDepth;
            batch->vertices[i + 3] = uvs.br.x;
            batch->vertices[i + 4] = uvs.br.y;
            batch->vertices[i + 5] = paletteIndex;
            //topright
            batch->vertices[i + 6] = verts.br.x;
            batch->vertices[i + 7] = verts.tl.y;
            batch->vertices[i + 8] = wallDepth;
            batch->vertices[i + 9] = uvs.br.x;
            batch->vertices[i + 10] = uvs.tl.y;
            batch->vertices[i + 11] = paletteIndex;
            // top left
            batch->vertices[i + 12] = verts.tl.x;
            batch->vertices[i + 13] = verts.tl.y;
            batch->vertices[i + 14] = wallDepth;
            batch->vertices[i + 15] = uvs.tl.x;
            batch->vertices[i + 16] = uvs.tl.y;
            batch->vertices[i + 17] = paletteIndex;
            // bottomleft
            batch->vertices[i + 18] = verts.tl.x;
            batch->vertices[i + 19] = verts.br.y;
            batch->vertices[i + 20] = wallDepth;
            batch->vertices[i + 21] = uvs.tl.x;
            batch->vertices[i + 22] = uvs.br.y;
            batch->vertices[i + 23] = paletteIndex;
        }




        //ASSERT(batch->count * 6 < 2048 * 6);
        for (u32 i = 0; i < batch->count * 6; i += 6) {
            int j = (i / 6) * 4;
            batch->indices[i + 0] = j + 0;
            batch->indices[i + 1] = j + 1;
            batch->indices[i + 2] = j + 2;
            batch->indices[i + 3] = j + 0;
            batch->indices[i + 4] = j + 2;
            batch->indices[i + 5] = j + 3;
        }

#ifdef GLES
        makeBufferRPI(batch->vertices, batch->indices, batch->count, &batch->VBO, &batch->EBO, GL_STATIC_DRAW);
#endif
#ifdef GL3
        makeBuffer(batch->vertices, batch->indices, batch->count, &batch->VAO, &batch->VBO, &batch->EBO, GL_STATIC_DRAW);
#endif
    }

    for (int actor_batch_index = 0; actor_batch_index < 8; actor_batch_index++) {
        DrawBuffer *batch = &renderer->actors[actor_batch_index];
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




    // prepare buffers for FONT drawing
 
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
    //glDepthFunc(GL_GREATER);


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
        int count = batch->count; //game->actor_count;

// actors
#ifdef GL3
        glBindVertexArray(batch->VAO);
#endif
        u32 screenWidth = renderer->view.width;
        u32 screenHeight = renderer->view.height;


        int texture_size = renderer->assets.sprite.width;
        int real_world_height = game->dims.z_level * game->block_size.z_level;
        int real_world_depth = game->dims.y * game->block_size.y;

        for (int i = 0; i < count * VALUES_PER_ELEM; i += VALUES_PER_ELEM) {
            int prepare_index = i / VALUES_PER_ELEM;
            prepare_index += (actor_batch_index * 2048);
            Actor data = game->actors[prepare_index];
            r32 scale = 1;
            r32 guyFrameX = data.frame * 24.0f;

            // this offset is to get actors drawn on top of walls/floors that are of the same depth
            float offset_toget_actor_ontop_of_floor = (float)24.0f / real_world_depth;
            float guyDepth = -1 * ((float)data.y / (float)real_world_depth) - offset_toget_actor_ontop_of_floor;

            float tempX = data.x;
            float tempY = (data.z) - (data.y) / 2;
            tempX += game->x_view_offset;
            tempY += game->y_view_offset;

            float x = ((float)tempX / (float)screenWidth) * 2.0f - 1.0f;
            double y = ((float)tempY / (float)screenHeight) * 2.0f - 1.0f;

            float paletteIndex = data.palette_index; //rand_float();

            Rect2 uvs = get_uvs(texture_size, guyFrameX, 11*12.0f , 24.0f, 96.0f);
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
        bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.shader1);
        CHECK();
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * VALUES_PER_ELEM * 4, batch->vertices);
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        glDisableVertexAttribArray(0);
#endif
#ifdef GL3
        glBindBuffer(GL_ARRAY_BUFFER, batch->VBO);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(batch->vertices), batch->vertices, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * VALUES_PER_ELEM * 4, batch->vertices);
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);
#endif
        CHECK();
    }


    for (int wall_batch_index = 0; wall_batch_index < renderer->used_wall_batches; wall_batch_index++) {
        DrawBuffer *batch = &renderer->walls[wall_batch_index];
        int count = batch->count; //game->wall_count;
                                  //Draw walls
#ifdef GLES
        bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.shader1);
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        glDisableVertexAttribArray(0);
#endif

#ifdef GL3
        glBindVertexArray(batch->VAO);
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);
#endif

        CHECK();
    }

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
                float x = -1.0f + (((float)(data.x) / (float)renderer->view.width) * 2.0f);
                float y = -1.0f + (((float)(renderer->view.height - data.y) / (float)renderer->view.height) * 2.0f);
                float paletteIndex = 0.4f; //rand_float();

                Rect2 uvs = get_uvs(texture_size, (float)data.sx, (float)data.sy, (float)data.w, (float)data.h);
                Rect2 verts = get_verts(renderer->view.width, renderer->view.height, x, y, (float)data.w, (float)data.h, scale, scale, 0.0, 0.0);
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
            bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.shader1);
            CHECK();
            glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * VALUES_PER_ELEM * 4, batch->vertices);
            glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
            glDisableVertexAttribArray(0);
#endif
#ifdef GL3
            glBindBuffer(GL_ARRAY_BUFFER, batch->VBO);
            //glBufferData(GL_ARRAY_BUFFER, sizeof(batch->vertices), batch->vertices, GL_DYNAMIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * VALUES_PER_ELEM * 4, batch->vertices);
            glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
            glBindVertexArray(0);
#endif
            CHECK();
        }
    }



    CHECK();
    SDL_GL_SwapWindow(window);
}
