#version 330 core
layout (location = 0) in vec2 xy;
layout (location = 1) in vec2 uv;

uniform mat4 MVP;

out vec2 out_uv;

void main()
{
	gl_Position = MVP * vec4(xy, 1.0f, 1.0f);
    out_uv = vec2(uv.x, uv.y);
}