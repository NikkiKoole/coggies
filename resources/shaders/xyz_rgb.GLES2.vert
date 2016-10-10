attribute vec3 xyz;
attribute vec3 rgb;

uniform mat4 MVP;


varying vec3 out_color;

void main()
{
    gl_Position = MVP * vec4(xyz.x, xyz.y, xyz.z, 1.0);
    out_color = rgb;
}