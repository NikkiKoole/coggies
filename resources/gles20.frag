precision lowp float;
uniform sampler2D ourTexture1;
uniform sampler2D ourTexture2;

varying vec2 v_TexCoord;
varying float v_Palette;

void main() {
     gl_FragColor = texture2D(ourTexture1, v_TexCoord);
     //gl_FragColor = texture2D(ourTexture1, v_TexCoord) + vec4(0.1,0.,0.,0);
     if (gl_FragColor.a == 0.0) {
        discard;
     }

    
    vec2 index = vec2(gl_FragColor.r + (0.5/16.0) , v_Palette);
    vec4 indexedColor = texture2D(ourTexture2, index);
    gl_FragColor = vec4(indexedColor.rgb, gl_FragColor.a);


}