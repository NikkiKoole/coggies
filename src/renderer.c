#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "renderer.h"
#include "multi_platform.h"
#include "types.h"
#include "memory.h"
#include "random.h"

RenderState _rstate;
RenderState *renderer = &_rstate;

GameState _gstate;
GameState *game = &_gstate;

PerfDict _p_dict;
PerfDict *perf_dict = &_p_dict;


ShaderLayout debug_text_layout;
ShaderLayout actors_layout;
ShaderLayout walls_layout;

typedef struct {
    float x;
    float y;
} V2;

typedef struct {
    V2 tl;
    V2 br;
} Rect2;




#define CHECK()                                                                                        \
    {                                                                   \
        int error = glGetError();                                       \
        if (error != 0) {                                               \
            printf("%d, function %s, file: %s, line:%d. \n", error, __FUNCTION__, __FILE__, __LINE__); \
            exit(0);                                                    \
        }                                                               \
    }


void setup_shader_layouts(void) {
    debug_text_layout.elements[0] = (ShaderLayoutElement) {2, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "xy"} ;
    debug_text_layout.elements[1] = (ShaderLayoutElement) {2, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "uv"} ;
    debug_text_layout.values_per_quad = (2 + 2) * 4;
    debug_text_layout.element_count = 2;

    actors_layout.elements[0] = (ShaderLayoutElement) {3, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "xyz"} ;
    actors_layout.elements[1] = (ShaderLayoutElement) {2, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "uv"} ;
    actors_layout.elements[2] = (ShaderLayoutElement) {1, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "palette"} ;
    actors_layout.values_per_quad = (3 + 2 + 1) * 4;
    actors_layout.element_count = 3;

    walls_layout.elements[0] = (ShaderLayoutElement) {3, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "xyz"} ;
    walls_layout.elements[1] = (ShaderLayoutElement) {2, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "uv"} ;
    walls_layout.values_per_quad = (3 + 2) * 4;
    walls_layout.element_count = 2;
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
internal void makeBufferRPI(VERTEX_FLOAT_TYPE vertices[], GLushort indices[], int size, GLuint *VBO, GLuint *EBO, GLenum usage) {
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
    GLuint a_Position = glGetAttribLocation(*program, "xyz");
    glVertexAttribPointer(a_Position, 3, GL_FLOAT_TYPE, GL_FALSE, sizeof(VERTEX_FLOAT_TYPE) * 6, (GLvoid *)0);
    glEnableVertexAttribArray(a_Position);
    GLuint a_TexCoord = glGetAttribLocation(*program, "uv");
    glVertexAttribPointer(a_TexCoord, 2, GL_FLOAT_TYPE, GL_FALSE, sizeof(VERTEX_FLOAT_TYPE) * 6, (GLvoid *)(3 * sizeof(VERTEX_FLOAT_TYPE)));
    glEnableVertexAttribArray(a_TexCoord);
    GLuint a_Palette = glGetAttribLocation(*program, "palette");
    glVertexAttribPointer(a_Palette, 1, GL_FLOAT_TYPE, GL_FALSE, sizeof(VERTEX_FLOAT_TYPE) * 6, (GLvoid *)(5 * sizeof(VERTEX_FLOAT_TYPE)));
    glEnableVertexAttribArray(a_Palette);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
}
#endif

#ifdef GL3

internal void makeBuffer(VERTEX_FLOAT_TYPE vertices[], GLushort indices[], int size, GLuint *VAO, GLuint *VBO, GLuint *EBO, GLenum usage, ShaderLayout *layout) {
    UNUSED(layout);
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glGenBuffers(1, EBO);

    glBindVertexArray(*VAO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);


    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * size * layout->values_per_quad, vertices, usage);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * size * 6, indices, GL_STATIC_DRAW);


    int stride = 0;
    for (int i = 0; i < layout->element_count; i++) {
        //printf("%d, %d,  %s\n",i, stride, layout->elements[i].attr_name);
        ShaderLayoutElement *elem = &layout->elements[i];
        glVertexAttribPointer(i, elem->amount, elem->type, GL_FALSE, (layout->values_per_quad/4) * elem->type_size, (GLvoid *) (uintptr_t)(stride * elem->type_size));
        stride += elem->amount;
        glEnableVertexAttribArray(i);
        CHECK();
        //printf("stuff!\n");
        //printf("%s\n",layout->elements[i].attr_name);
    }
    // Position attribute
    /* glVertexAttribPointer(0, 3, GL_FLOAT_TYPE, GL_FALSE, 6 * sizeof(VERTEX_FLOAT_TYPE), (GLvoid *)0); */
    /* glEnableVertexAttribArray(0); */
    /* // TexCoord attribute */
    /* glVertexAttribPointer(1, 2, GL_FLOAT_TYPE, GL_FALSE, 6 * sizeof(VERTEX_FLOAT_TYPE), (GLvoid *)(3 * sizeof(VERTEX_FLOAT_TYPE))); */
    /* glEnableVertexAttribArray(1); */
    /* // PAlette */
    /* glVertexAttribPointer(2, 1, GL_FLOAT_TYPE, GL_FALSE, 6 * sizeof(VERTEX_FLOAT_TYPE), (GLvoid *)(5 * sizeof(VERTEX_FLOAT_TYPE))); */
    /* glEnableVertexAttribArray(2); */




    glBindVertexArray(0); // Unbind VAO
    CHECK();
}
#endif




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


/* internal int cmpfunc(const void *a, const void *b) { */
/*     // TODO  I dont understand it anymore ;) */

/*     const Wall *a2 = (const Wall *)a; */
/*     const Wall *b2 = (const Wall *)b; */
/*     return ((a2->y) - (b2->y)); */
/* } */

void prepare_renderer(void) {
    //printf("PREPARE CALLED!\n");
    //ASSERT(renderer->walls.count * VALUES_PER_ELEM < 2048 * 24);
    glViewport(0, 0, renderer->view.width, renderer->view.height);

    //int real_world_height = game->dims.z_level * game->block_size.z_level;
    int real_world_depth = game->dims.y * (game->block_size.y);
    int screenWidth = renderer->view.width;
    int screenHeight = renderer->view.height;

    int offset_x_blocks = game->x_view_offset;
    int offset_y_blocks = game->y_view_offset;
    int texture_size = renderer->assets.sprite.width;



    for (int wall_batch_index = 0; wall_batch_index < 8; wall_batch_index++) {
        DrawBuffer *batch = &renderer->walls[wall_batch_index];
        u32 count = batch->count; //game->actor_count;

        for (u32 i = 0; i < count * 20; i += 20) {
            int prepare_index = i / 20;
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
            float wallY = 0.0f;
            float wallHeight = 108.0f;


            // TODO THIS IS SOME OPTIMIZATION, TEST IT ON THE RPI
            // Eventually I want to have some meta lookup for texture atlasses, then everything will be like this.
            /* if (data.frame > 6 && data.frame < 10) { // wallpart is a floor, much smaller then a wall */
            /*     wallY = 106.0f; */
            /*     wallHeight = 14.0f; */
            /* } */


            // float paletteIndex = 1.0f / 16 * 1; //rand_float(); //(data.y / 350.0f);
            Rect2 uvs = get_uvs(texture_size, wallX, wallY, 24, wallHeight);
            Rect2 verts = get_verts(renderer->view.width, renderer->view.height, x, y, 24.0f, wallHeight, scale, scale, 0.5, 1.0f);
            //printf("%d\n",i);
            // bottomright
            batch->vertices[i + 0] = verts.br.x;
            batch->vertices[i + 1] = verts.br.y;
            batch->vertices[i + 2] = wallDepth;
            batch->vertices[i + 3] = uvs.br.x;
            batch->vertices[i + 4] = uvs.br.y;
            //batch->vertices[i + 5] = paletteIndex;
            //topright
            batch->vertices[i + 5] = verts.br.x;
            batch->vertices[i + 6] = verts.tl.y;
            batch->vertices[i + 7] = wallDepth;
            batch->vertices[i + 8] = uvs.br.x;
            batch->vertices[i + 9] = uvs.tl.y;
            //batch->vertices[i + 11] = paletteIndex;
            // top left
            batch->vertices[i + 10] = verts.tl.x;
            batch->vertices[i + 11] = verts.tl.y;
            batch->vertices[i + 12] = wallDepth;
            batch->vertices[i + 13] = uvs.tl.x;
            batch->vertices[i + 14] = uvs.tl.y;
            // batch->vertices[i + 17] = paletteIndex;
            // bottomleft
            batch->vertices[i + 15] = verts.tl.x;
            batch->vertices[i + 16] = verts.br.y;
            batch->vertices[i + 17] = wallDepth;
            batch->vertices[i + 18] = uvs.tl.x;
            batch->vertices[i + 19] = uvs.br.y;
            //batch->vertices[i + 23] = paletteIndex;
            //printf("uv xy: %f,%f  %f,%f  %f,%f  %f,%f\n",batch->vertices[i + 3],batch->vertices[i + 4], batch->vertices[i + 8], batch->vertices[i + 9], batch->vertices[i + 13],batch->vertices[i + 14], batch->vertices[i + 18],batch->vertices[i + 19]);
        }




        //ASSERT(batch->count * 6 < 2048 * 6);
        //printf("batch-count: %d\n",batch->count * 6);
        for (u32 i = 0; i < batch->count * 6; i += 6) {
            int j = (i / 6) * 4;
            //printf("%d\n",j);
            batch->indices[i + 0] = j + 0;
            batch->indices[i + 1] = j + 1;
            batch->indices[i + 2] = j + 2;
            batch->indices[i + 3] = j + 0;
            batch->indices[i + 4] = j + 2;
            batch->indices[i + 5] = j + 3;
        }

#ifdef GLES
        makeBufferRPI(batch->vertices, batch->indices, batch->count, &batch->VBO, &batch->EBO, GL_STATIC_DRAW, &walls_layout);
#endif
#ifdef GL3
        makeBuffer(batch->vertices, batch->indices, batch->count, &batch->VAO, &batch->VBO, &batch->EBO, GL_STATIC_DRAW, &walls_layout);
#endif
    }
    CHECK();
    for (int actor_batch_index = 0; actor_batch_index < 32; actor_batch_index++) {
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
        makeBufferRPI(batch->vertices, batch->indices, 2048, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &actors_layout);
#endif
#ifdef GL3
        makeBuffer(batch->vertices, batch->indices, 2048, &batch->VAO, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &actors_layout);
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
            makeBufferRPI(batch->vertices, batch->indices, 2048, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &actors_layout);
#endif
#ifdef GL3
            makeBuffer(batch->vertices, batch->indices, 2048, &batch->VAO, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &actors_layout);
#endif
        }
    }
    //printf("PREPARE END!\n");
}


void render_walls(void);
void render_walls(void) {
    glUseProgram(renderer->assets.xyz_uv);

    // Bind Textures using texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->assets.sprite.id);
    glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv, "sprite_atlas"), 0);

    //glActiveTexture(GL_TEXTURE1);
    //glBindTexture(GL_TEXTURE_2D, renderer->assets.palette.id);
    //glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv_palette, "palette16x16"), 1);



    for (int wall_batch_index = 0; wall_batch_index < renderer->used_wall_batches; wall_batch_index++) {
        DrawBuffer *batch = &renderer->walls[wall_batch_index];
        UNUSED(batch);
//int count = batch->count; //game->wall_count;
//Draw walls
#ifdef GLES
        bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_uv_palette);
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
}

void render_actors(void);
void render_actors(void) {
    float screenWidth = renderer->view.width;
    float screenHeight = renderer->view.height;
    float actor_texture_size = renderer->assets.sprite.width;
    //float real_world_height = game->dims.z_level * game->block_size.z_level;
    float real_world_depth = game->dims.y * game->block_size.y;


    //u64 running_total_nested_loop = 0;


    for (int actor_batch_index = 0; actor_batch_index < renderer->used_actor_batches; actor_batch_index++) {
        DrawBuffer *batch = &renderer->actors[actor_batch_index];
        int count = batch->count; //game->actor_count;

// actors
#ifdef GL3
        glBindVertexArray(batch->VAO);
#endif

        // TODO: try to optimize this loop., humpf getting rid of the functions didnt do a whole lot..
        // https://software.intel.com/en-us/articles/creating-a-particle-system-with-streaming-simd-extensions
        // I might have to bite the bullet, make my actors an soa instead of an aos and
        // simdify/neonify the f*ck out of them..

        // also for the mapbuffer stuff to actually work, I will need to try to chaneg the amount of actors as littel as possible
        // changing the AMOUNT will need to a new bufferData, so perhaps, just a isDead is a better approach.
        // then I could shedule some moment in tme where I clean up

        // anyway, it will be quite the rewrite, better test this in a separate exzample first

        for (int i = 0; i < count * VALUES_PER_ELEM; i += VALUES_PER_ELEM) {
            BEGIN_PERFORMANCE_COUNTER(actor_draw);
            //            u64 begin_loop = SDL_GetPerformanceCounter();
            int prepare_index = i / VALUES_PER_ELEM;
            prepare_index += (actor_batch_index * 2048);
            Actor data = game->actors[prepare_index];


            //printf("in render is actor float?  %f \n", data.y);
            const float scale = 1.0f;
            const float guyFrameX = data.frame * 24.0f;


            // this offset is to get actors drawn on top of walls/floors that are of the same depth
            const float offset_toget_actor_ontop_of_floor = 24.0f / real_world_depth;
            const float guyDepth = -1.0f * (data.y / real_world_depth) - offset_toget_actor_ontop_of_floor;

            const float tempX = round(data.x + game->x_view_offset);
            const float tempY = round(((data.z) - (data.y) / 2.0f) + game->y_view_offset);
            //tempX += game->x_view_offset;
            //tempY += game->y_view_offset;

            const float x = (tempX / screenWidth) * 2.0f - 1.0f;
            const float y = (tempY / screenHeight) * 2.0f - 1.0f;

            const float paletteIndex = data.palette_index; //rand_float();


            const float guyFrameY = 9.0f * 12.0f;
            const float guyFrameHeight = 108.0f;
            const float guyFrameWidth = 24.0f;

            const float UV_TL_X = guyFrameX / actor_texture_size;
            const float UV_TL_Y = (guyFrameY + guyFrameHeight) / actor_texture_size;
            const float UV_BR_X = (guyFrameX + guyFrameWidth) / actor_texture_size;
            const float UV_BR_Y = guyFrameY / actor_texture_size;

            //Rect2 uvs = get_uvs(actor_texture_size, guyFrameX, 9*12.0f , 24.0f, 108.0f);
            //Rect2 verts = get_verts(renderer->view.width, renderer->view.height, x, y, 24.0f, 108.0f, scale, scale, 0.5, 1.0f);

            const float pivotX = 0.5f;
            const float pivotY = 1.0f; //

            const float VERT_TL_X = x - ((pivotX * 2) * (guyFrameWidth / screenWidth) * scale);
            const float VERT_TL_Y = y - ((2 - pivotY * 2) * (guyFrameHeight / screenHeight) * scale);
            const float VERT_BR_X = x + ((2 - pivotX * 2) * (guyFrameWidth / screenWidth) * scale);
            const float VERT_BR_Y = y + ((pivotY * 2) * (guyFrameHeight / screenHeight) * scale);


            // bottomright
            batch->vertices[i + 0] = VERT_BR_X; //verts.br.x;
            batch->vertices[i + 1] = VERT_BR_Y; //verts.br.y;
            batch->vertices[i + 2] = guyDepth;
            batch->vertices[i + 3] = UV_BR_X; // uvs.br.x;
            batch->vertices[i + 4] = UV_BR_Y; ////uvs.br.y;
            batch->vertices[i + 5] = paletteIndex;
            //topright
            batch->vertices[i + 6] = VERT_BR_X; //verts.br.x;
            batch->vertices[i + 7] = VERT_TL_Y; //verts.tl.y;
            batch->vertices[i + 8] = guyDepth;
            batch->vertices[i + 9] = UV_BR_X;  //uvs.br.x;
            batch->vertices[i + 10] = UV_TL_Y; //uvs.tl.y;
            batch->vertices[i + 11] = paletteIndex;
            // top left
            batch->vertices[i + 12] = VERT_TL_X; //verts.tl.x;
            batch->vertices[i + 13] = VERT_TL_Y; //verts.tl.y;
            batch->vertices[i + 14] = guyDepth;
            batch->vertices[i + 15] = UV_TL_X; //uvs.tl.x;
            batch->vertices[i + 16] = UV_TL_Y; //uvs.tl.y;
            batch->vertices[i + 17] = paletteIndex;
            // bottomleft
            batch->vertices[i + 18] = VERT_TL_X; //verts.tl.x;
            batch->vertices[i + 19] = VERT_BR_Y; //verts.br.y;
            batch->vertices[i + 20] = guyDepth;
            batch->vertices[i + 21] = UV_TL_X; //uvs.tl.x;
            batch->vertices[i + 22] = UV_BR_Y; //uvs.br.y;
            batch->vertices[i + 23] = paletteIndex;
            END_PERFORMANCE_COUNTER(actor_draw);

            //u64 end_loop = SDL_GetPerformanceCounter();
            //running_total_nested_loop += (end_loop - begin_loop);
        }

#ifdef GLES
        bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_uv_palette);
        CHECK();
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * VALUES_PER_ELEM * sizeof(VERTEX_FLOAT_TYPE), batch->vertices);
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        glDisableVertexAttribArray(0);
#endif
#ifdef GL3
        glBindBuffer(GL_ARRAY_BUFFER, batch->VBO);
        CHECK();
        //glBufferData(GL_ARRAY_BUFFER, sizeof(batch->vertices), batch->vertices, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * VALUES_PER_ELEM * sizeof(VERTEX_FLOAT_TYPE), batch->vertices);
        CHECK();
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        CHECK();
        glBindVertexArray(0);
#endif
        CHECK();
    }
}

void render_text(void);
void render_text(void) {
    // Draw FONTS
    {
        // Bind Textures using texture units
        glActiveTexture(GL_TEXTURE0);
        CHECK();

        glBindTexture(GL_TEXTURE_2D, renderer->assets.menlo.id);
        CHECK();

        glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv_palette, "sprite_atlas"), 0);
        CHECK();

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, renderer->assets.palette.id);
        glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv_palette, "palette16x16"), 1);

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
                float paletteIndex = 0.3f; //rand_float();

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
            bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_uv_palette);
            CHECK();

            glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * VALUES_PER_ELEM * sizeof(VERTEX_FLOAT_TYPE), batch->vertices);
            glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
            glDisableVertexAttribArray(0);
#endif
#ifdef GL3
            glBindBuffer(GL_ARRAY_BUFFER, batch->VBO);
            //glBufferData(GL_ARRAY_BUFFER, sizeof(batch->vertices), batch->vertices, GL_DYNAMIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * VALUES_PER_ELEM * sizeof(VERTEX_FLOAT_TYPE), batch->vertices);
            glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
            glBindVertexArray(0);
#endif
            CHECK();
        }
    }
}

void render(SDL_Window *window) {
    BEGIN_PERFORMANCE_COUNTER(render_func);
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClearDepthf(1.0f);
    glEnable(GL_BLEND);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    //glDepthFunc(GL_GREATER);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // this part needs to be repeated in all render loops (To use specific shader programs)
    glUseProgram(renderer->assets.xyz_uv_palette);

    // Bind Textures using texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->assets.sprite.id);
    glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv_palette, "sprite_atlas"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->assets.palette.id);
    glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv_palette, "palette16x16"), 1);
    // end this part needs to be repeated

    render_actors();
    render_walls();

 // this part needs to be repeated in all render loops (To use specific shader programs)
    glUseProgram(renderer->assets.xyz_uv_palette);

    // Bind Textures using texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->assets.sprite.id);
    glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv_palette, "sprite_atlas"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->assets.palette.id);
    glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv_palette, "palette16x16"), 1);
    // end this part needs to be repeated
    render_text();


    CHECK();
    END_PERFORMANCE_COUNTER(render_func);
    BEGIN_PERFORMANCE_COUNTER(swap_window);
    SDL_GL_SwapWindow(window);
    END_PERFORMANCE_COUNTER(swap_window);
}
