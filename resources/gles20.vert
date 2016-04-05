
attribute vec4 a_Position;
attribute vec2 a_TexCoord;
attribute float a_Palette;
varying vec2 v_TexCoord;
varying float v_Palette;
void main() {
     gl_Position = a_Position;
     //gl_Position = vec4(a_Position.x, a_Position.y, a_Position.zw);
     v_TexCoord = a_TexCoord;
     v_Palette = a_Palette;
}