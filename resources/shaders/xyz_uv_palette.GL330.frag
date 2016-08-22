#version 330 core
in vec2 out_uv;
in float out_palette;
out vec4 color;
// Texture samplers
uniform sampler2D sprite_atlas;
uniform sampler2D palette16x16;

void main()
{

	color = texture(sprite_atlas, out_uv);
    if (color.a == 0.0) {
        discard;
    }
    vec2 index = vec2(color.r + (0.5/16.0) , out_palette);
    vec4 indexedColor = texture(palette16x16, index);
    color = vec4(indexedColor.rgb, color.a);
}
