attribute vec2 xy;
attribute vec2 uv;

varying vec2 out_uv;
void main()
{
	gl_Position = vec4(xy, 1.0f, 1.0f);
    out_uv = vec2(uv.x, uv.y);
}