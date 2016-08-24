precision lowp float;
uniform sampler2D sprite_atlas;
uniform sampler2D palette16x16;

varying vec2 out_uv;
varying float out_palette;

void main() {
     gl_FragColor = texture2D(sprite_atlas, out_uv);
     if (gl_FragColor.a == 0.0) {
        discard;
     }


    vec2 index = vec2(gl_FragColor.r + (0.5/16.0) , out_palette);
    vec4 indexedColor = texture2D(palette16x16, index);
    gl_FragColor = vec4(indexedColor.rgb, gl_FragColor.a);


}