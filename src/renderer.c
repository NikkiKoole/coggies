#include "renderer.h"
#include "multi_platform.h"
#include "types.h"
#include "memory.h"
#include "random.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>




typedef struct {
    float x;
    float y;
} V2;

typedef struct {
    V2 tl;
    V2 br;
} Rect2;



#define CHECK()                                                         \
    {                                                                   \
        int error = glGetError();                                       \
        if (error != 0) {                                               \
            printf("%d, function %s, file: %s, line:%d. \n", error, __FUNCTION__, __FILE__, __LINE__); \
            exit(0);                                                    \
        }                                                               \
    }



void setup_shader_layouts(RenderState *renderer) {
    renderer->debug_text_layout.elements[0] = (ShaderLayoutElement) {2, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "xy"} ;
    renderer->debug_text_layout.elements[1] = (ShaderLayoutElement) {2, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "uv"} ;
    renderer->debug_text_layout.values_per_vertex = (2 + 2);
    renderer->debug_text_layout.values_per_thing = 4 * renderer->debug_text_layout.values_per_vertex;
    renderer->debug_text_layout.element_count = 2;

    renderer->actors_layout.elements[0] = (ShaderLayoutElement) {3, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "xyz"} ;
    renderer->actors_layout.elements[1] = (ShaderLayoutElement) {2, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "uv"} ;
    renderer->actors_layout.elements[2] = (ShaderLayoutElement) {1, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "palette"} ;
    renderer->actors_layout.values_per_vertex = (3 + 2 + 1);
    renderer->actors_layout.values_per_thing = 4 *  renderer->actors_layout.values_per_vertex;;
    renderer->actors_layout.element_count = 3;

    renderer->walls_layout.elements[0] = (ShaderLayoutElement) {3, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "xyz"} ;
    renderer->walls_layout.elements[1] = (ShaderLayoutElement) {2, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "uv"} ;
    renderer->walls_layout.values_per_vertex = (3 + 2);
    renderer->walls_layout.values_per_thing = 4 * renderer->walls_layout.values_per_vertex;
    renderer->walls_layout.element_count = 2;

    renderer->colored_lines_layout.elements[0] = (ShaderLayoutElement) {3, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "xyz"} ;
    renderer->colored_lines_layout.elements[1] = (ShaderLayoutElement) {3, GL_FLOAT_TYPE, sizeof(VERTEX_FLOAT_TYPE), "rgb"} ;
    renderer->colored_lines_layout.values_per_vertex = (3 + 3);
    renderer->colored_lines_layout.values_per_thing = 2 * renderer->colored_lines_layout.values_per_vertex;
    renderer->colored_lines_layout.element_count = 2;
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

internal inline Rect2 get_verts_mvp(float x,
                                    float y,
                                    float width,
                                    float height,
                                    float scaleX,
                                    float scaleY,
                                    float pivotX,
                                    float pivotY) {
    Rect2 result;
    result.tl.x = x - ((pivotX) * (width) * scaleX);
    result.tl.y = y - ((1.0f - pivotY ) * (height ) * scaleY);
    result.br.x = x + ((1.0f - pivotX ) * (width ) * scaleX);
    result.br.y = y + ((pivotY) * (height ) * scaleY);
    return result;
}


#ifdef GLES
internal void makeBufferRPI(VERTEX_FLOAT_TYPE vertices[], GLushort indices[], int size, GLuint *VBO, GLuint *EBO, GLenum usage, ShaderLayout *layout) {
    UNUSED(layout);
    glGenBuffers(1, VBO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * size * layout->values_per_thing, vertices, usage);

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
        GLint loc =  glGetAttribLocation(*program, elem->attr_name);
        ASSERT(loc >=0);
        glVertexAttribPointer(loc, elem->amount, elem->type, GL_FALSE, (layout->values_per_vertex) * elem->type_size, (GLvoid *) (uintptr_t)(stride * elem->type_size));
        stride += elem->amount;
        glEnableVertexAttribArray(loc);
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

    //glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, usage);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * size * layout->values_per_thing, vertices, usage);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * size * 6, indices, GL_STATIC_DRAW);


    int stride = 0;
    for (int i = 0; i < layout->element_count; i++) {
        //printf("%d, %d,  %s\n",i, stride, layout->elements[i].attr_name);
        ShaderLayoutElement *elem = &layout->elements[i];
        glVertexAttribPointer(i, elem->amount, elem->type, GL_FALSE, (layout->values_per_vertex) * elem->type_size, (GLvoid *) (uintptr_t)(stride * elem->type_size));
        stride += elem->amount;
        glEnableVertexAttribArray(i);

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




void prepare_renderer(PermanentState *permanent, RenderState *renderer) {
    //ASSERT(renderer->walls.count * VALUES_PER_ELEM < 2048 * 24);
    glViewport(0, 0, renderer->view.width, renderer->view.height);

    int texture_size = renderer->assets.sprite.width;


    for (int dynamic_block_batch_index = 0; dynamic_block_batch_index < DYNAMIC_BLOCK_BATCH_COUNT; dynamic_block_batch_index++) {
        DrawBuffer *batch = &renderer->dynamic_blocks[dynamic_block_batch_index];

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
        makeBufferRPI(batch->vertices, batch->indices, 2048, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->walls_layout);
#endif
#ifdef GL3
        makeBuffer(batch->vertices, batch->indices, 2048, &batch->VAO, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->walls_layout);
#endif
    }
    {
    for (int wall_batch_index = 0; wall_batch_index < TRANSPARENT_BLOCK_BATCH_COUNT; wall_batch_index++) {
        DrawBuffer *batch = &renderer->transparent_blocks[wall_batch_index];
        u32 count = batch->count; //permanent->actor_count;
        for (u32 i = 0;
             i < count * renderer->walls_layout.values_per_thing;
             i += renderer->walls_layout.values_per_thing)
            {
                int prepare_index = i / renderer->walls_layout.values_per_thing;
                prepare_index += (wall_batch_index * 2048);
                StaticBlock data = permanent->transparent_blocks[prepare_index];
                float scale = 1.0f;
                float wallX = data.frame.x_pos;// * 24;
                float wallY = data.frame.y_pos;// * 108;
                float wallDepth = data.y;
                float pivotY = 1.0f;
                float wallHeight = data.frame.height;
                float tempX = data.x;
                float tempY = (data.z) - (data.y) / 2;

                if (data.is_floor) {
                    // TODO this offset is still bugging me
                    wallDepth = data.y - 20;
                    //pivotY = (108.0f) /(108.0f - 12.0f);
                    //tempY -= 12;
                }


                Rect2 uvs = get_uvs(texture_size, wallX, wallY, data.frame.width, wallHeight);
                //Rect2 verts = get_verts(renderer->view.width, renderer->view.height, x, y, 24.0f, wallHeight, scale, scale, 0.5, 1.0f);
                Rect2 verts = get_verts_mvp(tempX, tempY, data.frame.width*1.0f, wallHeight, scale, scale, 0.5, pivotY);
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
                printf("TRANSPARENT uv xy: %f,%f  %f,%f  %f,%f  %f,%f\n",batch->vertices[i + 3],batch->vertices[i + 4], batch->vertices[i + 8], batch->vertices[i + 9], batch->vertices[i + 13],batch->vertices[i + 14], batch->vertices[i + 18],batch->vertices[i + 19]);
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
        makeBufferRPI(batch->vertices, batch->indices, batch->count, &batch->VBO, &batch->EBO, GL_STATIC_DRAW, &renderer->walls_layout);
#endif
#ifdef GL3
        makeBuffer(batch->vertices, batch->indices, batch->count, &batch->VAO, &batch->VBO, &batch->EBO, GL_STATIC_DRAW, &renderer->walls_layout);
#endif
    }

    }



    for (int wall_batch_index = 0; wall_batch_index < STATIC_BLOCK_BATCH_COUNT; wall_batch_index++) {
        DrawBuffer *batch = &renderer->static_blocks[wall_batch_index];
        u32 count = batch->count; //permanent->actor_count;

        for (u32 i = 0;
             i < count * renderer->walls_layout.values_per_thing;
             i += renderer->walls_layout.values_per_thing)
            {
                int prepare_index = i / renderer->walls_layout.values_per_thing;
                prepare_index += (wall_batch_index * 2048);
                StaticBlock data = permanent->static_blocks[prepare_index];
                float scale = 1.0f;
                float wallX = data.frame.x_pos;// * 24;
                float wallY = data.frame.y_pos;// * 108;
                float wallDepth = data.y;
                float pivotY = 1.0f;
                float wallHeight = data.frame.height;
                float tempX = data.x;
                float tempY = (data.z) - (data.y) / 2;

                if (data.is_floor) {
                    // TODO this offset is still bugging me
                    wallDepth = data.y - 20;
                    //pivotY = (108.0f) /(108.0f - 12.0f);
                    //tempY -= 12;
                }


                Rect2 uvs = get_uvs(texture_size, wallX, wallY, data.frame.width, wallHeight);
                //Rect2 verts = get_verts(renderer->view.width, renderer->view.height, x, y, 24.0f, wallHeight, scale, scale, 0.5, 1.0f);
                Rect2 verts = get_verts_mvp(tempX, tempY, data.frame.width*1.0f, wallHeight, scale, scale, 0.5, pivotY);
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
                //printf("STATIC uv xy: %f,%f  %f,%f  %f,%f  %f,%f\n",batch->vertices[i + 3],batch->vertices[i + 4], batch->vertices[i + 8], batch->vertices[i + 9], batch->vertices[i + 13],batch->vertices[i + 14], batch->vertices[i + 18],batch->vertices[i + 19]);
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
        makeBufferRPI(batch->vertices, batch->indices, batch->count, &batch->VBO, &batch->EBO, GL_STATIC_DRAW, &renderer->walls_layout);
#endif
#ifdef GL3
        makeBuffer(batch->vertices, batch->indices, batch->count, &batch->VAO, &batch->VBO, &batch->EBO, GL_STATIC_DRAW, &renderer->walls_layout);
#endif
    }



    for (int actor_batch_index = 0; actor_batch_index < ACTOR_BATCH_COUNT; actor_batch_index++) {
        DrawBuffer *batch = &renderer->actors[actor_batch_index];

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
        makeBufferRPI(batch->vertices, batch->indices, 2048, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->actors_layout);
#endif
#ifdef GL3
        makeBuffer(batch->vertices, batch->indices, 2048, &batch->VAO, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->actors_layout);
#endif
    }
    //printf("fonts \n");

    // prepare buffers for FONT drawing
    {
        for (int glyph_batch_index = 0; glyph_batch_index < GLYPH_BATCH_COUNT; glyph_batch_index++) {
            DrawBuffer *batch = &renderer->glyphs[glyph_batch_index];

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
            makeBufferRPI(batch->vertices, batch->indices, 2048, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->debug_text_layout);
#endif
#ifdef GL3
            makeBuffer(batch->vertices, batch->indices, 2048, &batch->VAO, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->debug_text_layout);
#endif
        }
    }
    //printf("lines \n");


    // prepare buffers for LINE drawing
    {
        for (int line_batch_index = 0; line_batch_index < LINE_BATCH_COUNT; line_batch_index++) {
            DrawBuffer *batch = &renderer->colored_lines[line_batch_index];
            for (u32 i = 0; i < 2048 * 6; i += 6) {
                int j = (i / 6) * 4;
                batch->indices[i + 0] = j + 0;
                batch->indices[i + 1] = j + 1;
                batch->indices[i + 2] = j + 2;
                batch->indices[i + 3] = j + 3;
                batch->indices[i + 4] = j + 4;
                batch->indices[i + 5] = j + 5;
            }
#ifdef GLES
            makeBufferRPI(batch->vertices, batch->indices, 2048, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->colored_lines_layout);
#endif
#ifdef GL3
            makeBuffer(batch->vertices, batch->indices, 2048, &batch->VAO, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->colored_lines_layout);
#endif
        }
    }
}



void update_and_draw_actor_vertices(PermanentState *permanent, RenderState *renderer, DebugState *debug);
void update_and_draw_actor_vertices(PermanentState *permanent, RenderState *renderer, DebugState *debug){

    float actor_texture_size = renderer->assets.sprite.width;
    int number_to_do = renderer->actors_layout.values_per_thing;

    ASSERT(number_to_do > 0);

    for (int actor_batch_index = 0; actor_batch_index < renderer->used_actor_batches; actor_batch_index++) {
        DrawBuffer *batch = &renderer->actors[actor_batch_index];
        int count = batch->count;


        for (int i = 0; i < count * number_to_do; i += number_to_do) {
            BEGIN_PERFORMANCE_COUNTER(render_actors_batches);
            int prepare_index = i / number_to_do;
            prepare_index += (actor_batch_index * 2048);
            Actor data = permanent->actors[prepare_index];

            const float scale = 1.0f;
            const float guyFrameX = data._frame * 24.0f;

            GLKVector3 location = data._location;
            const float guyDepth = location.y;


            const float x2 = round(location.x);
            const float y2 = round((location.z) - (location.y) / 2.0f);
            const float paletteIndex = data._palette_index; //rand_float();

            const float guyFrameY = 9.0f * 12.0f;
            const float guyFrameHeight = 108.0f ;
            const float guyFrameWidth = 24.0f;

            const float UV_TL_X = guyFrameX / actor_texture_size;
            const float UV_TL_Y = (guyFrameY + guyFrameHeight) / actor_texture_size;
            const float UV_BR_X = (guyFrameX + guyFrameWidth) / actor_texture_size;
            const float UV_BR_Y = guyFrameY / actor_texture_size;

            Rect2 verts = get_verts_mvp(x2, y2, 24.0f, 108.0f, scale, scale, 0.5, 1.0f);


            // bottomright
            batch->vertices[i + 0] = verts.br.x;
            batch->vertices[i + 1] = verts.br.y;
            batch->vertices[i + 2] = guyDepth;
            batch->vertices[i + 3] = UV_BR_X;
            batch->vertices[i + 4] = UV_BR_Y;
            batch->vertices[i + 5] = paletteIndex;
            //topright
            batch->vertices[i + 6] = verts.br.x;
            batch->vertices[i + 7] = verts.tl.y;
            batch->vertices[i + 8] = guyDepth;
            batch->vertices[i + 9] = UV_BR_X;
            batch->vertices[i + 10] = UV_TL_Y;
            batch->vertices[i + 11] = paletteIndex;
            // top left
            batch->vertices[i + 12] = verts.tl.x;
            batch->vertices[i + 13] = verts.tl.y;
            batch->vertices[i + 14] = guyDepth;
            batch->vertices[i + 15] = UV_TL_X;
            batch->vertices[i + 16] = UV_TL_Y;
            batch->vertices[i + 17] = paletteIndex;
            // bottomleft
            batch->vertices[i + 18] = verts.tl.x;
            batch->vertices[i + 19] = verts.br.y;
            batch->vertices[i + 20] = guyDepth;
            batch->vertices[i + 21] = UV_TL_X;
            batch->vertices[i + 22] = UV_BR_Y;
            batch->vertices[i + 23] = paletteIndex;
            END_PERFORMANCE_COUNTER(render_actors_batches);
        }

        BEGIN_PERFORMANCE_COUNTER(render_actors_buffers);


#ifdef GLES
        bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_uv_palette, &renderer->actors_layout);
        CHECK();
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * number_to_do * sizeof(VERTEX_FLOAT_TYPE), batch->vertices);
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        glDisableVertexAttribArray(0);
#endif

#ifdef GL3
        BEGIN_PERFORMANCE_COUNTER(render_actors_buffersubber);
        glBindVertexArray(batch->VAO);
        glBindBuffer(GL_ARRAY_BUFFER, batch->VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * number_to_do * sizeof(VERTEX_FLOAT_TYPE), batch->vertices);
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);
        END_PERFORMANCE_COUNTER(render_actors_buffersubber);
#endif
        END_PERFORMANCE_COUNTER(render_actors_buffers);
    }
    //printf("largestZ = %f, smallestZ = %f\n",smallestZ, largestZ);
}




void render_actors(PermanentState *permanent,  RenderState *renderer, DebugState *debug);
void render_actors(PermanentState *permanent, RenderState *renderer, DebugState *debug) {

    // this part needs to be repeated in all render loops (To use specific shader programs)
    glUseProgram(renderer->assets.xyz_uv_palette);

     GLint MatrixID = glGetUniformLocation(renderer->assets.xyz_uv_palette, "MVP");
     ASSERT(MatrixID >= 0);
     glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &renderer->mvp.m[0]);

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



void render_dynamic_blocks(PermanentState *permanent,  RenderState *renderer);
void render_dynamic_blocks(PermanentState *permanent, RenderState *renderer) {
    UNUSED(permanent);

    glUseProgram(renderer->assets.xyz_uv);

    GLint MatrixID = glGetUniformLocation(renderer->assets.xyz_uv, "MVP");
    ASSERT(MatrixID >= 0);
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &renderer->mvp.m[0]);

    // Bind Textures using texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->assets.sprite.id);
    glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv, "sprite_atlas"), 0);

    //glActiveTexture(GL_TEXTURE1);
    //glBindTexture(GL_TEXTURE_2D, renderer->assets.palette.id);
    //glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv_palette, "palette16x16"), 1);

    //printf("\nused wall batches: %d\n",renderer->used_wall_batches);

    int texture_size = renderer->assets.sprite.width;
    int number_to_do = renderer->walls_layout.values_per_thing;

    for (int wall_batch_index = 0; wall_batch_index < renderer->used_dynamic_block_batches; wall_batch_index++) {
        //printf("\nindex used wall batch: %d\n",wall_batch_index);

        DrawBuffer *batch = &renderer->dynamic_blocks[wall_batch_index];


        u32 count = batch->count; //permanent->actor_count;
        for (u32 i = 0;
             i < count * renderer->walls_layout.values_per_thing;
             i += renderer->walls_layout.values_per_thing)
            {
                int prepare_index = i / renderer->walls_layout.values_per_thing;
                prepare_index += (wall_batch_index * 2048);
                DynamicBlock data = permanent->dynamic_blocks[prepare_index];
                float scale = 1.0f;
                float wallX = data.frame.x_pos;// * 24;
                float wallY = data.frame.y_pos;// * 108;
                float wallDepth = data.y;
                float pivotY = 1.0f;

                float wallHeight = data.frame.height;

                float tempX = data.x;
                float tempY = (data.z+data.frame.y_off) - (data.y) / 2;

                if (data.is_floor) {
                    // TODO this offset is still bugging me
                    wallDepth = data.y - 20;
                    //pivotY = (108.0f) /(108.0f - 12.0f);
                    //tempY -= 12;
                }


                Rect2 uvs = get_uvs(texture_size, wallX, wallY, data.frame.width, wallHeight);
                //Rect2 verts = get_verts(renderer->view.width, renderer->view.height, x, y, 24.0f, wallHeight, scale, scale, 0.5, 1.0f);
                Rect2 verts = get_verts_mvp(tempX, tempY, data.frame.width*1.0f, wallHeight, scale, scale, 0.5, pivotY);

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





        UNUSED(batch);

#ifdef GLES
        bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_uv_palette, &renderer->actors_layout);
        CHECK();
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * number_to_do * sizeof(VERTEX_FLOAT_TYPE), batch->vertices);
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        glDisableVertexAttribArray(0);
#endif

#ifdef GL3
        glBindVertexArray(batch->VAO);
        glBindBuffer(GL_ARRAY_BUFFER, batch->VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * number_to_do * sizeof(VERTEX_FLOAT_TYPE), batch->vertices);
        glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);

#endif
        CHECK();
    }
}




void render_walls(PermanentState *permanent,  RenderState *renderer);
void render_walls(PermanentState *permanent, RenderState *renderer) {
    UNUSED(permanent);

    glUseProgram(renderer->assets.xyz_uv);

    GLint MatrixID = glGetUniformLocation(renderer->assets.xyz_uv, "MVP");
    ASSERT(MatrixID >= 0);
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &renderer->mvp.m[0]);

    // Bind Textures using texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->assets.sprite.id);
    glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv, "sprite_atlas"), 0);


    for (int wall_batch_index = 0; wall_batch_index < renderer->used_static_block_batches; wall_batch_index++) {
        DrawBuffer *batch = &renderer->static_blocks[wall_batch_index];
        UNUSED(batch);

#ifdef GLES
        bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_uv, &renderer->walls_layout );
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

void render_transparent_blocks(PermanentState *permanent,  RenderState *renderer);
void render_transparent_blocks(PermanentState *permanent, RenderState *renderer) {
    UNUSED(permanent);

    glUseProgram(renderer->assets.xyz_uv);

    GLint MatrixID = glGetUniformLocation(renderer->assets.xyz_uv, "MVP");
    ASSERT(MatrixID >= 0);
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &renderer->mvp.m[0]);

    // Bind Textures using texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->assets.sprite.id);
    glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv, "sprite_atlas"), 0);


    for (int wall_batch_index = 0; wall_batch_index < renderer->used_transparent_block_batches; wall_batch_index++) {
        DrawBuffer *batch = &renderer->transparent_blocks[wall_batch_index];
        UNUSED(batch);

#ifdef GLES
        bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_uv, &renderer->walls_layout );
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


        GLint MatrixID = glGetUniformLocation(renderer->assets.xy_uv, "MVP");
        ASSERT(MatrixID >= 0);
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &renderer->mvp.m[0]);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderer->assets.menlo.id);
        glUniform1i(glGetUniformLocation(renderer->assets.xy_uv, "sprite_atlas"), 0);
        CHECK();

        int texture_size = renderer->assets.menlo.width;
        int number_to_do = renderer->debug_text_layout.values_per_thing;
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
                r32 scale = 1.0f;
                //float x = -1.0f + (((float)(data.x) / (float)renderer->view.width) * 2.0f);
                //float y = -1.0f + (((float)(renderer->view.height - data.y) / (float)renderer->view.height) * 2.0f);
                float x2 = (float)(data.x);
                float y2 = (float)(renderer->view.height - data.y);

                // TODO save the UVs on loadtime then just read them
                Rect2 uvs = get_uvs(texture_size, (float)data.sx, (float)data.sy, (float)data.w, (float)data.h);
                //Rect2 verts = get_verts(renderer->view.width, renderer->view.height, x, y, (float)data.w, (float)data.h, scale, scale, 0.0, 0.0);
                Rect2 verts = get_verts_mvp(x2, y2, (float)data.w, (float)data.h, scale, scale, 0.0, 0.0);

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
            bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.xy_uv, &renderer->debug_text_layout);
            glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * number_to_do * sizeof(VERTEX_FLOAT_TYPE), batch->vertices);
            glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
            glDisableVertexAttribArray(0);
#endif
#ifdef GL3
            glBindBuffer(GL_ARRAY_BUFFER, batch->VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * number_to_do * sizeof(VERTEX_FLOAT_TYPE), batch->vertices);
            glDrawElements(GL_TRIANGLES, batch->count * 6, GL_UNSIGNED_SHORT, 0);
            glBindVertexArray(0);
#endif
            CHECK();
        }
    }
}

void render_lines(PermanentState *permanent, RenderState *renderer);
void render_lines(PermanentState *permanent, RenderState *renderer) {
    glUseProgram(renderer->assets.xyz_rgb);
    GLint MatrixID = glGetUniformLocation(renderer->assets.xyz_rgb, "MVP");
    ASSERT(MatrixID >= 0);
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &renderer->mvp.m[0]);

    for (int line_batch_index = 0; line_batch_index < renderer->used_colored_lines_batches; line_batch_index++) {
        DrawBuffer *batch = &renderer->colored_lines[line_batch_index];
        int count = batch->count;
#ifdef GL3
        glBindVertexArray(batch->VAO);
#endif

        int number_to_do = renderer->colored_lines_layout.values_per_thing;

        for (int i = 0; i < count * number_to_do; i += number_to_do) {
            int prepare_index = i / number_to_do;
            prepare_index += (line_batch_index * 2048);
            ColoredLine data = permanent->colored_lines[prepare_index];

            const float tempX1 = round(data.x1);
            const float tempY1 = round(((data.z1) - (data.y1) / 2.0f) );
            //const float x1 = (tempX1 / screenWidth) * 2.0f - 1.0f;
            //const float y1 = ((tempY1+6) / screenHeight) * 2.0f - 1.0f;

            const float tempX2 = round(data.x2 );
            const float tempY2 = round(((data.z2) - (data.y2) / 2.0f));
            //const float x2 = (tempX2 / screenWidth) * 2.0f - 1.0f;
            //const float y2 = ((tempY2+6) / screenHeight) * 2.0f - 1.0f;

            batch->vertices[i + 0] = tempX1;//x1;
            batch->vertices[i + 1] = tempY1+7;//y1;
            batch->vertices[i + 2] = 0.0f;
            batch->vertices[i + 3] = ABS(data.x1 - data.x2) > 0 ||  ABS(data.y1 - data.y2) > 0  ? 1.0f : 0.0f;
            batch->vertices[i + 4] = ABS(data.z1 - data.z2) > 0 ? 1.0f :0.0f;
            batch->vertices[i + 5] = data.b;
            batch->vertices[i + 6] = tempX2;//x2;
            batch->vertices[i + 7] = tempY2+7;//y2;
            batch->vertices[i + 8] = 0.0f;
            batch->vertices[i + 9] = ABS(data.x1 - data.x2) > 0 ||  ABS(data.y1 - data.y2) > 0  ? 1.0f : 0.0f;;//ABS(x1 - x2) > 0 ? 1.0f : 0.0f;
            batch->vertices[i + 10] = ABS(data.z1 - data.z2) > 0 ? 1.0f :0.0f;
            batch->vertices[i + 11] = data.b;
        }
#ifdef GLES
        bindBuffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_rgb, &renderer->colored_lines_layout);
        CHECK();

        glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * number_to_do * sizeof(VERTEX_FLOAT_TYPE), batch->vertices);
        glDrawElements(GL_LINES, batch->count * 4, GL_UNSIGNED_SHORT, 0);
        glDisableVertexAttribArray(0);
#endif
#ifdef GL3
        glBindBuffer(GL_ARRAY_BUFFER, batch->VBO);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(batch->vertices), batch->vertices, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch->count * number_to_do * sizeof(VERTEX_FLOAT_TYPE), batch->vertices);
        glDrawElements(GL_LINES, batch->count * 4, GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);
#endif
    }
}

void render(PermanentState *permanent, RenderState *renderer, DebugState *debug) {
    if (renderer->needs_prepare == 1) {
        prepare_renderer(permanent, renderer);
        renderer->needs_prepare = 0;
    }


    float identity[16] = { 1.0f, 0.0f, 0.0f, 0.0f,
                           0.0f, 1.0f, 0.0f, 0.0f,
                           0.0f, 0.0f, 1.0f, 0.0f,
                           0.0f, 0.0f, 0.0f, 1.0f};

    GLKMatrix4 model = GLKMatrix4MakeWithArray(identity);
    GLKMatrix4 projection = GLKMatrix4MakeOrtho(0.0f, 1.0f * (float)renderer->view.width,
                                                0.0f, 1.0f * (float)renderer->view.height,
                                                -1.0f  * (float)renderer->view.height, 1.0f  * (float)renderer->view.height);
    GLKMatrix4 view = GLKMatrix4MakeLookAt(0.0f, 0.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    renderer->mvp = GLKMatrix4Multiply(model, GLKMatrix4Multiply(projection, view));
    renderer->mvp = GLKMatrix4Translate(renderer->mvp,  permanent->x_view_offset,  permanent->y_view_offset, 0 );
    //renderer->mvp  = GLKMatrix4RotateZ(renderer->mvp, PI/2);


    BEGIN_PERFORMANCE_COUNTER(render_func);

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClearDepthf(1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    render_actors(permanent, renderer, debug);
    render_walls(permanent, renderer);
    render_dynamic_blocks(permanent, renderer);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    render_transparent_blocks(permanent, renderer);
    glDisable(GL_DEPTH_TEST);

    render_lines(permanent, renderer);
    renderer->mvp = GLKMatrix4Multiply(model, GLKMatrix4Multiply(projection, view)); // get rid of translation
    render_text(permanent, renderer);

    END_PERFORMANCE_COUNTER(render_func);

    BEGIN_PERFORMANCE_COUNTER(swap_window);
    SDL_GL_SwapWindow(renderer->window);
    END_PERFORMANCE_COUNTER(swap_window);
}
