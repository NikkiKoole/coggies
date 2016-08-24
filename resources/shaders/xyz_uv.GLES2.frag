precision lowp float;

uniform sampler2D sprite_atlas;
varying vec2 out_uv;




void main() {
    gl_FragColor = texture2D(sprite_atlas, out_uv);

    if (gl_FragColor.a == 0.0) {
        discard;
    }
}
