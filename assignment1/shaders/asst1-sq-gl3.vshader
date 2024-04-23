// If you have shader compilation problems, try changing this to 130
#version 150

uniform float uSquareShift;
uniform float uWindowWidthScale;
uniform float uWindowHeightScale;

in vec2 aPosition;
in vec2 aTexCoord;

out vec2 vTexCoord;

void main() {
    gl_Position = vec4(aPosition.x*uWindowWidthScale + uSquareShift*uWindowWidthScale, aPosition.y * uWindowHeightScale, 0, 1);
    vTexCoord = aTexCoord;
}
