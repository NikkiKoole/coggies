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

//float last_frame = 999;

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
    result.tl.y = y - ((2.0f - pivotY * 2) * (height / viewportHeight) * scaleY);
    result.br.x = x + ((2.0f - pivotX * 2) * (width / viewportWidth) * scaleX);
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
internal void make_buffer_RPI(VERTEX_FLOAT_TYPE vertices[], GLushort indices[], int size, GLuint *VBO, GLuint *EBO, GLenum usage, ShaderLayout *layout) {
    glGenBuffers(1, VBO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * size * layout->values_per_thing, vertices, usage);

    glGenBuffers(1, EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * size * 6, indices, GL_STATIC_DRAW);


}


internal void bind_buffer(GLuint *VBO, GLuint *EBO, GLuint *program,  ShaderLayout *layout) {
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

internal void make_buffer(VERTEX_FLOAT_TYPE vertices[], GLushort indices[], int size, GLuint *VAO, GLuint *VBO, GLuint *EBO, GLenum usage, ShaderLayout *layout) {
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

internal void general_drawloop(DrawBuffer *batch, u32 batch_count, u32 batch_index, u32 batch_size, u32 values_per_thing, StaticBlock *buf, float texWidth, float texHeight) {
    float actor_texture_width = texWidth;
    float actor_texture_height = texHeight;

    for (u32 i = 0; i < batch_count * values_per_thing; i+= values_per_thing) {
        int prepare_index = i / values_per_thing;
        prepare_index += (batch_index * batch_size);
        StaticBlock data = buf[prepare_index];
        BlockTextureAtlasPosition frame = data.frame;

        float scale = 1.0f;
        float wallX = data.frame.x_pos;
        float wallY = data.frame.y_pos;
        float wallDepth = data.y;
        const float pivotX = (float)data.frame.pivotX / (float)data.frame.width;
        const float pivotY = (float)data.frame.pivotY / (float)data.frame.height;
        float wallHeight = data.frame.height;
        float tempX = data.x + data.frame.x_internal_off;
        float tempY =  (data.z  - data.y/2 + data.frame.y_off) +  data.frame.y_internal_off + data.frame.height - data.frame.ssH;
        const float UV_TL_X = wallX / actor_texture_width;
        const float UV_TL_Y = (wallY + data.frame.height) / actor_texture_height;
        const float UV_BR_X = (wallX + data.frame.width) / actor_texture_width;
        const float UV_BR_Y = wallY / actor_texture_height;

        //Rect2 uvs = get_uvs(texture_size, wallX, wallY, data.frame.width*1.0f, wallHeight);
        Rect2 verts = get_verts_mvp(tempX, tempY, data.frame.width*1.0f, wallHeight, scale, scale, pivotX, pivotY);

        // bottomright
        batch->vertices[i + 0] = verts.br.x;
        batch->vertices[i + 1] = verts.br.y;
        batch->vertices[i + 2] = wallDepth;
        batch->vertices[i + 3] = UV_BR_X;
        batch->vertices[i + 4] = UV_BR_Y;
        //batch->vertices[i + 5] = paletteIndex;
        //topright
        batch->vertices[i + 5] = verts.br.x;
        batch->vertices[i + 6] = verts.tl.y;
        batch->vertices[i + 7] = wallDepth;
        batch->vertices[i + 8] = UV_BR_X;
        batch->vertices[i + 9] = UV_TL_Y;
        //batch->vertices[i + 11] = paletteIndex;
        // top left
        batch->vertices[i + 10] = verts.tl.x;
        batch->vertices[i + 11] = verts.tl.y;
        batch->vertices[i + 12] = wallDepth;
        batch->vertices[i + 13] = UV_TL_X;
        batch->vertices[i + 14] = UV_TL_Y;
        // batch->vertices[i + 17] = paletteIndex;
        // bottomleft
        batch->vertices[i + 15] = verts.tl.x;
        batch->vertices[i + 16] = verts.br.y;
        batch->vertices[i + 17] = wallDepth;
        batch->vertices[i + 18] = UV_TL_X;
        batch->vertices[i + 19] = UV_BR_Y;
    }
}



void prepare_renderer(PermanentState *permanent, RenderState *renderer) {
    //ASSERT(renderer->walls.count * VALUES_PER_ELEM < MAX_IN_BUFFER * 24);
    glViewport(0, 0, renderer->view.width, renderer->view.height);

    for (int dynamic_block_batch_index = 0; dynamic_block_batch_index < DYNAMIC_BLOCK_BATCH_COUNT; dynamic_block_batch_index++) {
        DrawBuffer *batch = &renderer->dynamic_blocks[dynamic_block_batch_index];

        for (u32 i = 0; i < MAX_IN_BUFFER * 6; i += 6) {
            int j = (i / 6) * 4;
            batch->indices[i + 0] = j + 0;
            batch->indices[i + 1] = j + 1;
            batch->indices[i + 2] = j + 2;
            batch->indices[i + 3] = j + 0;
            batch->indices[i + 4] = j + 2;
            batch->indices[i + 5] = j + 3;
        }
#ifdef GLES
        make_buffer_RPI(batch->vertices, batch->indices, MAX_IN_BUFFER, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->walls_layout);
#endif
#ifdef GL3
        make_buffer(batch->vertices, batch->indices, MAX_IN_BUFFER, &batch->VAO, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->walls_layout);
#endif
    }
    {
        // TRANSPARANT BLOCKS
    for (int wall_batch_index = 0; wall_batch_index < TRANSPARENT_BLOCK_BATCH_COUNT; wall_batch_index++) {
        DrawBuffer *batch = &renderer->transparent_blocks[wall_batch_index];
        u32 count = batch->count; //permanent->actor_count;

        general_drawloop(batch, batch->count, wall_batch_index, TRANSPARENT_BLOCK_BATCH_COUNT, renderer->walls_layout.values_per_thing, permanent->transparent_blocks,
                         renderer->assets.blocks.width, renderer->assets.blocks.height);

        for (u32 i = 0; i < MAX_IN_BUFFER * 6; i += 6) {
            int j = (i / 6) * 4;
            batch->indices[i + 0] = j + 0;
            batch->indices[i + 1] = j + 1;
            batch->indices[i + 2] = j + 2;
            batch->indices[i + 3] = j + 0;
            batch->indices[i + 4] = j + 2;
            batch->indices[i + 5] = j + 3;
        }

#ifdef GLES
        make_buffer_RPI(batch->vertices, batch->indices, batch->count, &batch->VBO, &batch->EBO, GL_STATIC_DRAW, &renderer->walls_layout);
#endif
#ifdef GL3
        make_buffer(batch->vertices, batch->indices, batch->count, &batch->VAO, &batch->VBO, &batch->EBO, GL_STATIC_DRAW, &renderer->walls_layout);
#endif
    }

    }


    // STATIC BLOCKS
    for (int wall_batch_index = 0; wall_batch_index < STATIC_BLOCK_BATCH_COUNT; wall_batch_index++) {
        DrawBuffer *batch = &renderer->static_blocks[wall_batch_index];
        u32 count = batch->count; //permanent->actor_count;

        general_drawloop(batch, batch->count, wall_batch_index, STATIC_BLOCK_BATCH_COUNT, renderer->walls_layout.values_per_thing, permanent->static_blocks,
                         renderer->assets.blocks.width, renderer->assets.blocks.height);

        for (u32 i = 0; i < MAX_IN_BUFFER * 6; i += 6) {
            int j = (i / 6) * 4;
            batch->indices[i + 0] = j + 0;
            batch->indices[i + 1] = j + 1;
            batch->indices[i + 2] = j + 2;
            batch->indices[i + 3] = j + 0;
            batch->indices[i + 4] = j + 2;
            batch->indices[i + 5] = j + 3;
        }

#ifdef GLES
        make_buffer_RPI(batch->vertices, batch->indices, batch->count, &batch->VBO, &batch->EBO, GL_STATIC_DRAW, &renderer->walls_layout);
#endif
#ifdef GL3
        make_buffer(batch->vertices, batch->indices, batch->count, &batch->VAO, &batch->VBO, &batch->EBO, GL_STATIC_DRAW, &renderer->walls_layout);
#endif
    }



    for (int actor_batch_index = 0; actor_batch_index < ACTOR_BATCH_COUNT; actor_batch_index++) {
        DrawBuffer *batch = &renderer->actors[actor_batch_index];

        for (u32 i = 0; i < MAX_IN_BUFFER * 6; i += 6) {
            int j = (i / 6) * 4;
            batch->indices[i + 0] = j + 0;
            batch->indices[i + 1] = j + 1;
            batch->indices[i + 2] = j + 2;
            batch->indices[i + 3] = j + 0;
            batch->indices[i + 4] = j + 2;
            batch->indices[i + 5] = j + 3;
        }
#ifdef GLES
        make_buffer_RPI(batch->vertices, batch->indices, MAX_IN_BUFFER, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->actors_layout);
#endif
#ifdef GL3
        make_buffer(batch->vertices, batch->indices, MAX_IN_BUFFER, &batch->VAO, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->actors_layout);
#endif
    }


    // prepare buffers for FONT drawing
    {
        for (int glyph_batch_index = 0; glyph_batch_index < GLYPH_BATCH_COUNT; glyph_batch_index++) {
            DrawBuffer *batch = &renderer->glyphs[glyph_batch_index];

            for (u32 i = 0; i < MAX_IN_BUFFER * 6; i += 6) {
                int j = (i / 6) * 4;
                batch->indices[i + 0] = j + 0;
                batch->indices[i + 1] = j + 1;
                batch->indices[i + 2] = j + 2;
                batch->indices[i + 3] = j + 0;
                batch->indices[i + 4] = j + 2;
                batch->indices[i + 5] = j + 3;
            }
#ifdef GLES
            make_buffer_RPI(batch->vertices, batch->indices, MAX_IN_BUFFER, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->debug_text_layout);
#endif
#ifdef GL3
            make_buffer(batch->vertices, batch->indices, MAX_IN_BUFFER, &batch->VAO, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->debug_text_layout);
#endif
        }
    }



    // prepare buffers for LINE drawing
    {
        for (int line_batch_index = 0; line_batch_index < LINE_BATCH_COUNT; line_batch_index++) {
            DrawBuffer *batch = &renderer->colored_lines[line_batch_index];
            for (u32 i = 0; i < MAX_IN_BUFFER * 6; i += 6) {
                int j = (i / 6) * 4;
                batch->indices[i + 0] = j + 0;
                batch->indices[i + 1] = j + 1;
                batch->indices[i + 2] = j + 2;
                batch->indices[i + 3] = j + 3;
                batch->indices[i + 4] = j + 4;
                batch->indices[i + 5] = j + 5;
            }
#ifdef GLES
            make_buffer_RPI(batch->vertices, batch->indices, MAX_IN_BUFFER, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->colored_lines_layout);
#endif
#ifdef GL3
            make_buffer(batch->vertices, batch->indices, MAX_IN_BUFFER, &batch->VAO, &batch->VBO, &batch->EBO, GL_DYNAMIC_DRAW, &renderer->colored_lines_layout);
#endif
        }
    }
}

internal void update_and_draw_actor_vertices(PermanentState *permanent, RenderState *renderer, DebugState *debug){

    float actor_texture_width = renderer->assets.character.width;
    float actor_texture_height = renderer->assets.character.height;
    int number_to_do = renderer->actors_layout.values_per_thing;

    ASSERT(number_to_do > 0);

    for (int actor_batch_index = 0; actor_batch_index < renderer->used_actor_batches; actor_batch_index++) {
        DrawBuffer *batch = &renderer->actors[actor_batch_index];
        int count = batch->count;


        for (int i = 0; i < count * number_to_do; i += number_to_do) {
            BEGIN_PERFORMANCE_COUNTER(render_actors_batches);
            int prepare_index = i / number_to_do;
            prepare_index += (actor_batch_index * MAX_IN_BUFFER);
            Actor data = permanent->actors[prepare_index];

            FrameWithPivotAnchor complex = *data.complex;

            const float scale = 1.0f;
            const float guyFrameX = complex.frameX;

            const Vector3 location = data._location;
            const float guyDepth = location.y;

            float y_internal_off = complex.sssY;
            float x_internal_off = complex.sssX;

            const float x2 = round(location.x ) + x_internal_off - 12.f + data.x_off;
            const float y2 = round((location.z - y_internal_off) - (location.y) / 2.0f) + 12.f + data.y_off;


            const float paletteIndex = data._palette_index;

            const float pivotX = (float)complex.pivotX / (float)complex.frameW;
            const float pivotY = (float)complex.pivotY / (float)complex.frameH;
            const float guyFrameY = complex.frameY;
            const float guyFrameHeight = complex.frameH;
            const float guyFrameWidth = complex.frameW;

            const float UV_TL_X = guyFrameX / actor_texture_width;
            const float UV_TL_Y = (guyFrameY + guyFrameHeight) / actor_texture_height;
            const float UV_BR_X = (guyFrameX + guyFrameWidth) / actor_texture_width;
            const float UV_BR_Y = guyFrameY / actor_texture_height;


            Rect2 verts = get_verts_mvp(x2, y2, guyFrameWidth, guyFrameHeight, scale, scale, pivotX, pivotY);


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
        bind_buffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_uv_palette, &renderer->actors_layout);
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

}





internal void render_actors(PermanentState *permanent, RenderState *renderer, DebugState *debug) {

    // this part needs to be repeated in all render loops (To use specific shader programs)
    glUseProgram(renderer->assets.xyz_uv_palette);

     GLint MatrixID = glGetUniformLocation(renderer->assets.xyz_uv_palette, "MVP");
     ASSERT(MatrixID >= 0);
     glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &renderer->mvp.m[0]);

    // Bind Textures using texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->assets.character.id);
    glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv_palette, "sprite_atlas"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->assets.palette.id);
    glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv_palette, "palette16x16"), 1);
    // end this part needs to be repeated

    update_and_draw_actor_vertices(permanent, renderer, debug);
}


internal void render_dynamic_blocks(PermanentState *permanent, RenderState *renderer) {
    glUseProgram(renderer->assets.xyz_uv);

    GLint MatrixID = glGetUniformLocation(renderer->assets.xyz_uv, "MVP");
    ASSERT(MatrixID >= 0);
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &renderer->mvp.m[0]);

    // Bind Textures using texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->assets.blocks.id);
    glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv, "sprite_atlas"), 0);

    int texture_size = renderer->assets.blocks.width;
    int number_to_do = renderer->walls_layout.values_per_thing;

    for (int wall_batch_index = 0; wall_batch_index < renderer->used_dynamic_block_batches; wall_batch_index++) {
        DrawBuffer *batch = &renderer->dynamic_blocks[wall_batch_index];
        u32 count = batch->count;

        general_drawloop(batch, batch->count, wall_batch_index, DYNAMIC_BLOCK_BATCH_COUNT, renderer->walls_layout.values_per_thing, permanent->dynamic_blocks,
                         renderer->assets.blocks.width, renderer->assets.blocks.height);


#ifdef GLES
        bind_buffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_uv_palette, &renderer->actors_layout);
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





internal void render_walls(RenderState *renderer) {
    glUseProgram(renderer->assets.xyz_uv);

    GLint MatrixID = glGetUniformLocation(renderer->assets.xyz_uv, "MVP");
    ASSERT(MatrixID >= 0);
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &renderer->mvp.m[0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->assets.blocks.id);
    glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv, "sprite_atlas"), 0);


    for (int wall_batch_index = 0; wall_batch_index < renderer->used_static_block_batches; wall_batch_index++) {
        DrawBuffer *batch = &renderer->static_blocks[wall_batch_index];

#ifdef GLES
        bind_buffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_uv, &renderer->walls_layout );
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


internal void render_transparent_blocks(RenderState *renderer) {

    glUseProgram(renderer->assets.xyz_uv);

    GLint MatrixID = glGetUniformLocation(renderer->assets.xyz_uv, "MVP");
    ASSERT(MatrixID >= 0);
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &renderer->mvp.m[0]);

    // Bind Textures using texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->assets.blocks.id);
    glUniform1i(glGetUniformLocation(renderer->assets.xyz_uv, "sprite_atlas"), 0);


    for (int wall_batch_index = 0; wall_batch_index < renderer->used_transparent_block_batches; wall_batch_index++) {
        DrawBuffer *batch = &renderer->transparent_blocks[wall_batch_index];

#ifdef GLES
        bind_buffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_uv, &renderer->walls_layout );
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


internal void render_text(PermanentState *permanent, RenderState *renderer) {
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
                prepare_index += (glyph_batch_index * MAX_IN_BUFFER);
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
            bind_buffer(&batch->VBO, &batch->EBO, &renderer->assets.xy_uv, &renderer->debug_text_layout);
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


internal void render_lines(PermanentState *permanent, RenderState *renderer) {
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
            prepare_index += (line_batch_index * MAX_IN_BUFFER);
            ColoredLine data = permanent->colored_lines[prepare_index];

            const float tempX1 = round(data.x1);
            const float tempY1 = round(((data.z1) - (data.y1) / 2.0f) );
            //const float x1 = (tempX1 / screenWidth) * 2.0f - 1.0f;
            //const float y1 = ((tempY1+6) / screenHeight) * 2.0f - 1.0f;

            const float tempX2 = round(data.x2 );
            const float tempY2 = round(((data.z2) - (data.y2) / 2.0f));
            //const float x2 = (tempX2 / screenWidth) * 2.0f - 1.0f;
            //const float y2 = ((tempY2+6) / screenHeight) * 2.0f - 1.0f;

            batch->vertices[i + 0] = tempX1 - 12;//x1;
            batch->vertices[i + 1] = tempY1 + 12;//y1;
            batch->vertices[i + 2] = 0.0f;
            batch->vertices[i + 3] = ABS(data.x1 - data.x2) > 0 ||  ABS(data.y1 - data.y2) > 0  ? 1.0f : 0.0f;
            batch->vertices[i + 4] = ABS(data.z1 - data.z2) > 0 ? 1.0f :0.0f;
            batch->vertices[i + 5] = data.b;
            batch->vertices[i + 6] = tempX2 - 12;//x2;
            batch->vertices[i + 7] = tempY2 + 12;//y2;
            batch->vertices[i + 8] = 0.0f;
            batch->vertices[i + 9] = ABS(data.x1 - data.x2) > 0 ||  ABS(data.y1 - data.y2) > 0  ? 1.0f : 0.0f;;//ABS(x1 - x2) > 0 ? 1.0f : 0.0f;
            batch->vertices[i + 10] = ABS(data.z1 - data.z2) > 0 ? 1.0f :0.0f;
            batch->vertices[i + 11] = data.b;
        }
#ifdef GLES
        bind_buffer(&batch->VBO, &batch->EBO, &renderer->assets.xyz_rgb, &renderer->colored_lines_layout);
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

    Matrix4 model = Matrix4MakeWithArray(identity);
    Matrix4 projection = Matrix4MakeOrtho(0.0f, 1.0f * (float)renderer->view.width,
                                                0.0f, 1.0f * (float)renderer->view.height,
                                                -1.0f  * (float)renderer->view.height, 1.0f  * (float)renderer->view.height);
    Matrix4 view = Matrix4MakeLookAt(0.0f, 0.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    renderer->mvp = Matrix4Multiply(model, Matrix4Multiply(projection, view));
    renderer->mvp = Matrix4Translate(renderer->mvp,  permanent->x_view_offset,  permanent->y_view_offset, 0 );
    //renderer->mvp  = Matrix4RotateZ(renderer->mvp, PI/2);


    BEGIN_PERFORMANCE_COUNTER(render_func);

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClearDepthf(1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    render_actors(permanent, renderer, debug);
    render_walls(renderer);
    render_dynamic_blocks(permanent, renderer);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    render_transparent_blocks(renderer);
    glDisable(GL_DEPTH_TEST);

    render_lines(permanent, renderer);
    renderer->mvp = Matrix4Multiply(model, Matrix4Multiply(projection, view)); // get rid of translation
    render_text(permanent, renderer);

    END_PERFORMANCE_COUNTER(render_func);

    BEGIN_PERFORMANCE_COUNTER(swap_window);
    SDL_GL_SwapWindow(renderer->window);
    END_PERFORMANCE_COUNTER(swap_window);
}
