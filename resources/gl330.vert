#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in float palette;

out float Palette;
out vec2 TexCoord;


void main()
{
	gl_Position = vec4(position, 1.0f);
    TexCoord = vec2(texCoord.x, texCoord.y);
    Palette = palette;
}