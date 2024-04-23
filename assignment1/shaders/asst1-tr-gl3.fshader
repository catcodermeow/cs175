// If you have shader compilation problems, try changing this to 130
#version 150

uniform float uBlender;
uniform sampler2D uTex0;

in vec2 vTexCoord;
in vec3 vColor;

out vec4 fragColor;

void main(void) {

    // The texture(..) calls always return a vec4. Data comes out of a texture
    // in RGBA format
    vec4 texColor = texture(uTex0, vTexCoord);
    // if the texture is white, turn colorful; else use texture color
    if (texColor.x == 1) {
        fragColor = vec4(vColor.x, vColor.y, vColor.z, 1);
    } else {
        fragColor = texColor;
    }
}
