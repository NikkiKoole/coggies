attribute vec4 xyz;
attribute vec2 uv;
attribute float palette;

uniform mat4 MVP;

varying vec2 out_uv;
varying float out_palette;

void main() {
     gl_Position = MVP * xyz;
     //gl_Position = vec4(xyz.x, xyz.y, xyz.zw);
     out_uv = uv;
     out_palette = palette;
}