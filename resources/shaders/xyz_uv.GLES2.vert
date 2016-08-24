attribute vec4 xyz;
attribute vec2 uv;

varying vec2 out_uv;



void main()
{
    gl_Position = xyz;//vec4(xyz, 1.0f);
    out_uv = uv;//vec2(uv.x, uv.y);
}