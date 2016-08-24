#version 330 core
in vec2 out_uv;

out vec4 color;
uniform sampler2D sprite_atlas;


void main()
{
	color = texture(sprite_atlas, out_uv);

    if (color.a == 0.0) {
        discard;
    }
}
