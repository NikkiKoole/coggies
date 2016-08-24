precision lowp float;
varying vec2 out_uv;

uniform sampler2D sprite_atlas;


void main()
{
	gl_FragColor = texture2d(sprite_atlas, out_uv);

    if (gl_FragColor.a == 0.0) {
        discard;
    }
}
