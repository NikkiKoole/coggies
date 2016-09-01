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



void setup_shader_layouts(RenderState *renderer) {
    renderer->debug_text_layout.elements[0] = (ShaderLayoutElement) {2, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "xy"} ;
    renderer->debug_text_layout.elements[1] = (ShaderLayoutElement) {2, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "uv"} ;
    renderer->debug_text_layout.values_per_quad = (2 + 2) * 4;
    renderer->debug_text_layout.element_count = 2;

    renderer->actors_layout.elements[0] = (ShaderLayoutElement) {3, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "xyz"} ;
    renderer->actors_layout.elements[1] = (ShaderLayoutElement) {2, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "uv"} ;
    renderer->actors_layout.elements[2] = (ShaderLayoutElement) {1, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "palette"} ;
    renderer->actors_layout.values_per_quad = (3 + 2 + 1) * 4;
    renderer->actors_layout.element_count = 3;

    renderer->walls_layout.elements[0] = (ShaderLayoutElement) {3, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "xyz"} ;
    renderer->walls_layout.elements[1] = (ShaderLayoutElement) {2, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "uv"} ;
    renderer->walls_layout.values_per_quad = (3 + 2) * 4;
    renderer->walls_layout.element_count = 2;
}



internal inline Rect2 get_uvs(float size, int x, int y, int width, int height) {
    Rect2 result;
    result.tl.x = x / size;
    result.tl.y = (y + height) / size;
    result.br.x = (x + width) / size;
    result.br.y = y / size;
    return result;
}

internal inline Rect2 get_verts(float viewportWidth,
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
internal void makeBufferRPI(VERTEX_FLOAT_TYPE vertices[], GLushort indices[], int size, GLuint *VBO, GLuint *EBO, GLenum usage, ShaderLayout *layout) {
    UNUSED(layout);
    glGenBuffers(1, VBO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * size * layout->values_per_quad, vertices, usage);

    glGenBuffers(1, EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * size * 6, indices, GL_STATIC_DRAW);


}


internal void bindBuffer(GLuint *VBO, GLuint *EBO, GLuint *program,  ShaderLayout *layout) {
    UNUSED(layout);

    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    int stride = 0;
    for (int i = 0; i < layout->element_count; i++) {
        ShaderLayoutElement *elem = &layout->elements[i];
	GLuint loc =  glGetAttribLocation(*program, elem->attr_name);

        glVertexAttribPointer(loc, elem->amount, elem->type, GL_FALSE, (layout->values_per_quad/4) * elem->type_size, (GLvoid *) (uintptr_t)(stride * elem->type_size));
	//printf("%d, %d, %d, %d, %d, %d\n",( GLuint)i, elem->amount, elem->type, GL_FALSE, (layout->values_per_quad/4) * elem->type_size, (stride * elem->type_size));
	stride += elem->amount;
        glEnableVertexAttribArray(loc);
        CHECK();
    }
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

    }


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

void prepare_renderer(PermanentState *permanent, RenderState *renderer) {
    //ASSERT(renderer->walls.count * VALUES_PER_ELEM < 2048 * 24);
    glViewport(0, 0, renderer->view.width, renderer->view.height);

    //int real_world_height = permanent->dims.z_level * permanent->block_size.z_level;
    int real_world_depth = permanent->dims.y * (permanent->block_size.y);
    int screenWidth = renderer->view.width;
    int screenHeight = renderer->view.height;

    int offset_x_blocks = permanent->x_view_offset;
    int offset_y_blocks = permanent->y_view_offset;
    int texture_size = renderer->assets.sprite.width;

    int number_to_do = renderer->walls_layout.values_per_quad;
    ASSERT(number_to_do > 0);

    for (int wall_batch_index = 0; wall_batch_index < 8; wall_batch_index++) {
        DrawBuffer *batch = &renderer->walls[wall_batch_index];
        u32 count = batch->count; //permanent->actor_count;

	for (u32 i = 0; i < count * number_to_do; i += number_to_do) {
            int prepare_index = i / number_to_do;
            prepare_index += (wall_batch_index * 2048);
            Wall data = permanent->walls[prepare_index];
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
        makeBufferRPI(batch->vertices, batch->indices, batch->count, &batch->VBO, &batch->EBO, GL_STATIC_DRAW, &walls_layout);
#endif
#ifdef GL3
        makeBuffer(batch->vertices, batch->indices, batch->count, &batch->VAO, &batch->VBO, &batch->EBO, GL_STATIC_DRAW, &renderer->walls_layout);
#endif
    }

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
        makeBuffer(batch->vertices, batch->indices, 2048, &batch->VAO, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->actors_layout);
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
            makeBufferRPI(batch->vertices, batch->indices, 2048, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &debug_text_layout);
#endif
#ifdef GL3
            makeBuffer(batch->vertices, batch->indices, 2048, &batch->VAO, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->debug_text_layout);
#endif
        }
    }
}



void update_and_draw_actor_vertices(PermanentState *permanent, RenderState *renderer, DebugState *debug);
void update_and_draw_actor_vertices(PermanentState *permanent, RenderState *renderer, DebugState *debug){

    float screenWidth = renderer->view.width;
    float screenHeight = renderer->view.height;
    float actor_texture_size = renderer->assets.sprite.width;
    //float real_world_height = permanent->dims.z_level * permanent->block_size.z_level;
    float real_world_depth = permanent->dims.y * permanent->block_size.y;

    int number_to_do = renderer->actors_layout.values_per_quad;
    ASSERT(number_to_do > 0);


    for (int actor_batch_index = 0; actor_batch_index < renderer->used_actor_batches; actor_batch_index++) {
        DrawBuffer *batch = &renderer->actors[actor_batch_index];
        int count = batch->count; //permanent->actor_count;

        /* glBindVertexArray(batch->VAO); */
        /* glBindBuffer(GL_ARRAY_BUFFER, batch->VBO); */
        /* GLvoid * ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY); */


        for (int i = 0; i < count * number_to_do; i += number_to_do) {
            BEGIN_PERFORMANCE_COUNTER(render_actors_batches);
            //            u64 begin_loop = SDL_GetPerformanceCounter();
            int prepare_index = i / number_to_do;
            prepare_index += (actor_batch_index * 2048);
            Actor data = permanent->actors[prepare_index];


            //printf("in render is actor float?  %f \n", data.y);
            const float scale = 1.0f;
            const float guyFrameX = data.frame * 24.0f;


            // this offset is to get actors drawn on top of walls/floors that are of the same depth
            const float offset_toget_actor_ontop_of_floor = 24.0f / real_world_depth;
            const float guyDepth = -1.0f * (data.y / real_world_depth) - offset_toget_actor_ontop_of_floor;

            const float tempX = round(data.x + permanent->x_view_offset);
            const float tempY = round(((data.z) - (data.y) / 2.0f) + permanent->y_view_offset);
            //tempX += permanent->x_view_offset;
            //tempY += permanent->y_view_offset;

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

            /* const int FL = sizeof(VERTEX_FLOAT_TYPE); */

            /* *(VERTEX_FLOAT_TYPE*)ptr = VERT_BR_X;     ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = VERT_BR_Y;     ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = guyDepth;      ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = UV_BR_X;       ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = UV_BR_Y;       ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = paletteIndex;  ptr+=FL; */

            /* *(VERTEX_FLOAT_TYPE*)ptr = VERT_BR_X;     ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = VERT_TL_Y;     ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = guyDepth;      ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = UV_BR_X;       ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = UV_TL_Y;       ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = paletteIndex;  ptr+=FL; */

            /* *(VERTEX_FLOAT_TYPE*)ptr = VERT_TL_X;     ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = VERT_TL_Y;     ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = guyDepth;      ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = UV_TL_X;       ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = UV_TL_Y;       ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = paletteIndex;  ptr+=FL; */

            /* *(VERTEX_FLOAT_TYPE*)ptr = VERT_TL_X;     ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = VERT_BR_Y;     ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = guyDepth;      ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = UV_TL_X;       ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = UV_BR_Y;       ptr+=FL; */
            /* *(VERTEX_FLOAT_TYPE*)ptr = paletteIndex;  ptr+=FL; */






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
            END_PERFORMANCE_COUNTER(render_actors_batches);


            //glUnmapBuffer(GL_ARRAY_BUFFER);
        }
        //glUnmapBuffer(GL_ARRAY_BUFFER);

        CHECK();
        BEGIN_PERFORMANCE_COUNTER(render_actors_buffers);


#ifdef GLES
        bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_uv_palette, &actors_layout);
        CHECK();
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * VALUES_PER_ELEM * sizeof(VERTEX_FLOAT_TYPE), batch->vertices);
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        glDisableVertexAttribArray(0);
#endif






#ifdef GL3
        /*  BEGIN_PERFORMANCE_COUNTER(render_actors_mapbuffer); */
        /* //glBindVertexArray(batch->VAO); */
        /* //glBindBuffer(GL_ARRAY_BUFFER, batch->VBO); */
        /* //GLvoid * ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY); */
        /* //memcpy(ptr, batch->vertices, (batch->count * VALUES_PER_ELEM * sizeof(VERTEX_FLOAT_TYPE))); */
        /* CHECK(); */
        /* glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0); */
        /* CHECK(); */
        /* glUnmapBuffer(GL_ARRAY_BUFFER); */
        /* glFlush(); */
        /* CHECK(); */
        /* END_PERFORMANCE_COUNTER(render_actors_mapbuffer); */




        BEGIN_PERFORMANCE_COUNTER(render_actors_buffersubber);
        glBindVertexArray(batch->VAO);
        glBindBuffer(GL_ARRAY_BUFFER, batch->VBO);
        CHECK();
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * VALUES_PER_ELEM * sizeof(VERTEX_FLOAT_TYPE), batch->vertices);
        CHECK();
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        CHECK();
        glBindVertexArray(0);
        //glFlush(); // not needed but gives a better idea of the costs
        END_PERFORMANCE_COUNTER(render_actors_buffersubber);

#endif
        END_PERFORMANCE_COUNTER(render_actors_buffers);
    }

}




void render_actors(PermanentState *permanent,  RenderState *renderer, DebugState *debug);
void render_actors(PermanentState *permanent, RenderState *renderer, DebugState *debug) {

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

    update_and_draw_actor_vertices(permanent, renderer, debug);



}



void render_walls(PermanentState *permanent,  RenderState *renderer);
void render_walls(PermanentState *permanent, RenderState *renderer) {
    UNUSED(permanent);
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
        bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_uv, &walls_layout );
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        glDisableVertexAttribArray(0);
#endif

#ifdef GL3
        ASSERT(batch->VAO);
        glBindVertexArray(batch->VAO);
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);
#endif
        CHECK();
    }
}

void render_text(PermanentState *permanent, RenderState *renderer);
void render_text(PermanentState *permanent, RenderState *renderer) {
    // Draw FONTS
    {
	glUseProgram(renderer->assets.xy_uv);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderer->assets.menlo.id);
        glUniform1i(glGetUniformLocation(renderer->assets.xy_uv, "sprite_atlas"), 0);
        CHECK();

        int texture_size = renderer->assets.menlo.width;
        int number_to_do = renderer->debug_text_layout.values_per_quad;
        ASSERT(number_to_do > 0);

        for (int glyph_batch_index = 0; glyph_batch_index < renderer->used_glyph_batches; glyph_batch_index++) {
            DrawBuffer *batch = &renderer->glyphs[glyph_batch_index];
            int count = batch->count;
#ifdef GL3
            glBindVertexArray(batch->VAO);
#endif

            for (int i = 0; i < count * number_to_do; i += number_to_do) {
                int prepare_index = i / number_to_do;
                prepare_index += (glyph_batch_index * 2048);
                Glyph data = permanent->glyphs[prepare_index];
                r32 scale = 1;
                float x = -1.0f + (((float)(data.x) / (float)renderer->view.width) * 2.0f);
                float y = -1.0f + (((float)(renderer->view.height - data.y) / (float)renderer->view.height) * 2.0f);
                Rect2 uvs = get_uvs(texture_size, (float)data.sx, (float)data.sy, (float)data.w, (float)data.h);
                Rect2 verts = get_verts(renderer->view.width, renderer->view.height, x, y, (float)data.w, (float)data.h, scale, scale, 0.0, 0.0);

                /* // bottomright */
                batch->vertices[i + 0] = verts.br.x;
                batch->vertices[i + 1] = verts.br.y;
                batch->vertices[i + 2] = uvs.br.x;
                batch->vertices[i + 3] = uvs.br.y;

                //topright
                batch->vertices[i + 4] = verts.br.x;
                batch->vertices[i + 5] = verts.tl.y;
                batch->vertices[i + 6] = uvs.br.x;
                batch->vertices[i + 7] = uvs.tl.y;

                // top left
                batch->vertices[i + 8] = verts.tl.x;
                batch->vertices[i + 9] = verts.tl.y;
                batch->vertices[i + 10] = uvs.tl.x;
                batch->vertices[i + 11] = uvs.tl.y;

                // bottomleft
                batch->vertices[i + 12] = verts.tl.x;
                batch->vertices[i + 13] = verts.br.y;
                batch->vertices[i + 14] = uvs.tl.x;
                batch->vertices[i + 15] = uvs.br.y;
            }
#ifdef GLES
            bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.xy_uv, &debug_text_layout);
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



void render(PermanentState *permanent, RenderState *renderer, DebugState *debug) {
    if (renderer->needs_prepare == 1) {
        prepare_renderer(permanent, renderer);
        renderer->needs_prepare = 0;
    }


    BEGIN_PERFORMANCE_COUNTER(render_func);

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClearDepthf(1.0f);
    glEnable(GL_BLEND);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    CHECK();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    render_actors(permanent, renderer, debug);
    render_walls(permanent, renderer);
    glDisable(GL_DEPTH_TEST);
    render_text(permanent, renderer);

    CHECK();
    END_PERFORMANCE_COUNTER(render_func);

    BEGIN_PERFORMANCE_COUNTER(swap_window);
    SDL_GL_SwapWindow(renderer->window);
    END_PERFORMANCE_COUNTER(swap_window);
}
