#version 330 core
layout (location = 0) in vec3 xyz;
layout (location = 1) in vec2 uv;
layout (location = 2) in float palette;

uniform mat4 MVP;


out float out_palette;
out vec2 out_uv;


void main()
{
	gl_Position = MVP * vec4(xyz, 1.0f);
    out_uv = vec2(uv.x, uv.y);
    out_palette = palette;
}