#version 330 core
in vec2 TexCoord;
in float Palette;
out vec4 color;
// Texture samplers
uniform sampler2D ourTexture1;
uniform sampler2D ourTexture2;

void main()
{

	color = texture(ourTexture1, TexCoord);
    if (color.a == 0.0) {
        discard;
    }
    vec2 index = vec2(color.r + (0.5/16.0) , Palette);
    vec4 indexedColor = texture(ourTexture2, index);
    color = vec4(indexedColor.rgb, color.a);
}
