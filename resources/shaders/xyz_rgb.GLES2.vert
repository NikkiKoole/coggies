attribute vec3 xyz;
attribute vec3 rgb;


varying vec3 out_color;

void main()
{
    gl_Position = vec4(xyz.x, xyz.y, xyz.z, 1.0);
    out_color = rgb;
}