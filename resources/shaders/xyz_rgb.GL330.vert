#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
precision lowp float;

uniform mat4 MVP;

out vec3 out_color;

void main()
{
    gl_Position =  MVP * vec4(position.x, position.y, position.z, 1.0);
    out_color = color;
}