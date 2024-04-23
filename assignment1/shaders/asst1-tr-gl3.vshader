// If you have shader compilation problems, try changing this to 130
#version 150

uniform float uWindowWidthScale;
uniform float uWindowHeightScale;
uniform float uTriangleShiftX;
uniform float uTriangleShiftY;

in vec2 aPosition;
in vec2 aTexCoord;
in vec3 aColor;

out vec2 vTexCoord;
out vec3 vColor;

void main() {
    gl_Position = vec4(aPosition.x*uWindowWidthScale + uTriangleShiftX * uWindowWidthScale, aPosition.y * uWindowHeightScale + uTriangleShiftY * uWindowHeightScale, 0, 1);
    vTexCoord = aTexCoord;
    vColor = aColor;
}
