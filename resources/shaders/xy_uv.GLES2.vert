attribute vec2 xy;
attribute vec2 uv;

uniform mat4 MVP;


varying vec2 out_uv;


void main() {
    //gl_Position = xy;
    gl_Position = MVP * vec4(xy.x, xy.y, -1.0, 1.0);
    //gl_Position = vec3(1.0, 1.0, 1.0)
    out_uv = uv;
}