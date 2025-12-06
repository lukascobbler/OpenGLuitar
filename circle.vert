#version 330 core

layout (location = 0) in vec2 aPos;

uniform float centerX;
uniform float centerY;
uniform float radius;
uniform float aspectRatio;

void main()
{
    vec2 scaled = vec2(aPos.x * radius / aspectRatio,
                       aPos.y * radius);
    vec2 center = vec2(centerX, centerY);
    gl_Position = vec4(center + scaled, 0.0, 1.0);
}